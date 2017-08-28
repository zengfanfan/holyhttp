#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <config.h>
#include <utils/print.h>
#include <utils/string.h>
#include <utils/file.h>
#include <utils/address.h>
#include "request.h"

static char *get_cookie(request_t *self, char *name)
{
    if (!self || !self->pkt || !name) {
        return "";
    }
    return self->pkt->cookies->get_ss(self->pkt->cookies, name);
}

static int set_cookie(request_t *self, char *name, char *value, int age)
{
    char cookie[MAX_FIELD_VALUE_LEN + 1] = {0};
    const u32 size = sizeof cookie;
    if (!self || !name || !value) {
        return 0;
    }
    STR_APPEND(cookie, size, "%s; HttpOnly", value ? value : "");
    if (age >= 0) {
        STR_APPEND(cookie, size, "; Max-Age=%d", age);
    }
    return self->cookies.set_ss(&self->cookies, name, cookie);
}

static int del_cookie(request_t *self, char *name)
{
    return set_cookie(self, name, "", 0);
}

static void cookie2str(tlv_t *key, tlv_t *value, void *args)
{
    char *buf = ((char **)args)[0];
    u32 len = *(((u32 **)args)[1]);
    STR_APPEND(buf, len, "Set-Cookie: %s=%s\r\n", key->v, value->v);
}

static void cookies_to_str(request_t *self, char *buf, u32 len)
{
    void *args[] = {buf, &len};
    self->cookies.foreach(&self->cookies, cookie2str, args);
}

static int get_arg(request_t *self, char *name, void **value, u32 *vlen)
{
    if (!self || !self->pkt) {
        return 0;
    }
    return self->pkt->args->get_sb(self->pkt->args, name, value, vlen);
}

static void gen_session_id(request_t *self, char *id, u32 size)
{
    // pid[0-8.hex] + bkdr(UA+port+ip)[8.hex] + rand()[0-16.hex]
    char upi[MAX_FIELD_VALUE_LEN * 2];
    char *ua = self->headers->get_ss(self->headers, "user-agent");
    snprintf(upi, sizeof upi, "%s%u%u", ua ? ua : "", self->port, self->ip);
    
    id[0] = 0;
    STR_APPEND(id, size, uint_to_s64(get_now_us()));
    STR_APPEND(id, size, uint_to_s64(getpid()));
    STR_APPEND(id, size, uint_to_s64(bkdr_hash(upi)));
}

static void check_session_timeout(void *args)
{
    session_t *ss = (session_t *)args;
    long now = get_sys_uptime();
    long interval = ss->last_req - now;
    if (interval < SESSION_AGE) {
        set_timeout(SESSION_AGE - interval, check_session_timeout, args);
        return;
    }
    // timeout
    free_dict(ss->dict);
    ss->parent->del_s(ss->parent, ss->id);
}

static int get_or_new_session(request_t *self)
{
    dict_t *sessions = self->server->sessions;
    session_t ss = {0};
    u32 ret, len = 0;
    char *sid;

    sid = get_cookie(self, SESSION_ID_NAME);
    if (!sid || !sid[0]) {
        goto _new;
    }

    ret = sessions->get_sb(sessions, sid, (void **)&self->session, NULL);
    if (!ret) {
        goto _new;
    }

    return 1;
    
_new:
    gen_session_id(self, ss.id, sizeof ss.id);

    ss.dict = new_dict();
    if (!ss.dict) {
        ERROR("Failed to create dict");
        return 0;
    }

    ss.parent = sessions;
    
    if (!sessions->set_sb(sessions, ss.id, &ss, sizeof ss)) {
        free_dict(ss.dict);
        return 0;
    }
    
    if (!sessions->get_sb(sessions, ss.id, (void **)&self->session, &len)) {
        free_dict(ss.dict);
        return 0;
    }
    
    if (!set_cookie(self, SESSION_ID_NAME, ss.id, SESSION_ID_AGE)) {
        free_dict(ss.dict);
        return 0;
    }

    set_timeout(SESSION_AGE, check_session_timeout, self->session);
    
    return 1;
}

static int response(request_t *self, status_code_t code,
            void *content, u32 len, char *type, int age,
            char *location, char *encoding,
            char *filename)
{
    u32 size;
    char *rsp, *now = gmtimestr(NOW);
    int ret;

    if (self->method == HEAD_METHOD || !content) {
        len = 0;
    }
    
    size = MAX_HEADER_LEN + len;
    rsp = (char *)malloc(size + 1);
    if (!rsp) {
        return 0;
    }

    rsp[0] = 0;
    STR_APPEND(rsp, size, "HTTP/1.1 %3d %s\r\n", code, strstatus(code));
    STR_APPEND(rsp, size, "Server: %s/%s\r\nConnection: keep-alive\r\n",
        SERVER_NAME, SERVER_VERSION);
    
    cookies_to_str(self, rsp, size);

    STR_APPEND(rsp, size, "Date: %s\r\n", now);
    STR_APPEND(rsp, size, "Cache-Control: max-age=%d\r\n", age);

    if (age || code == NOT_MODIFIED) {
        if (code == MOVED_PERMANENTLY) {
            STR_APPEND(rsp, size, "Last-Modified: %s\r\n", now);
        } else {
            STR_APPEND(rsp, size, "Last-Modified: %s\r\n", self->server->start_time);
        }
        STR_APPEND(rsp, size, "Expires: %s\r\n", gmtimestr(NOW + age));
    }

    if (code == METHOD_NOT_ALLOWED) {
        STR_APPEND(rsp, size, "Allow: HEAD,GET,POST\r\n");
    }

    if (type) {
        STR_APPEND(rsp, size, "Content-Type: %s; charset=utf-8\r\n", type);
    }

    if (location) {
        STR_APPEND(rsp, size, "Location: %s\r\n", location);
    }

    if (encoding) {
        STR_APPEND(rsp, size, "Content-Encoding: %s\r\n", encoding);
    }

    if (filename) {
        STR_APPEND(rsp, size, "Content-Disposition: attachment; filename=\"%s\"\r\n", filename);
    }

    STR_APPEND(rsp, size, "Content-Length: %u\r\n\r\n", len);

    // fill content
    size = strlen(rsp);
    if (len) {
        memcpy(rsp + size, content, len);
        size += len;
    }

    // print debug log
    if (location) {
        PRINT_CYAN("%15s:%-5u << %d %s\n",
            IPV4_BIN_TO_STR(self->ip), self->port, code, location);
    } else {
        PRINT_CYAN("%15s:%-5u << %d %s\n",
            IPV4_BIN_TO_STR(self->ip), self->port, code, strstatus(code));
    }

    ret = self->conn->write(self->conn, rsp, size);
    free(rsp);
    return ret;
}

static int redirect(request_t *self, char *location)
{
    if (!location) {
        return 0;
    }
    return response(self, MOVED_TEMPORARILY, NULL, 0, NULL, 0, location, NULL, NULL);
}

static int redirect_top(request_t *self, char *location)
{
    char content[MAX_URI_LEN * 2];
    if (!location) {
        return 0;
    }
    snprintf(content, sizeof content, "<script>top.location.href='%s'</script>", location);
    return response(self, OK, content, strlen(content), "text", 0, location, NULL, NULL);
}

static int redirect_forever(request_t *self, char *location)
{
    if (!location) {
        return 0;
    }
    return response(self, MOVED_PERMANENTLY, NULL, 0, NULL, SECPERYEAR, location, NULL, NULL);
}

static int send_status(request_t *self, status_code_t code)
{
    if (!self) {
        return 0;
    }
    return self->conn->send_status(self->conn, code);
}

static int send_file(request_t *self, char *filename)
{
    void *content;
    u32 len;
    char *since;
    time_t since_t;
    int ret;
    
    if (!self || !filename) {
        return 0;
    }

    since = self->headers->get_ss(self->headers, "if-modified-since");
    if (since) {
        since_t = gmtimeint(since);
        if (since_t >= self->server->start) {
            return send_status(self, NOT_MODIFIED);
        }
    }
    
    if (!get_file(filename, &content, &len)) {
        return send_status(self, NOT_FOUND);
    }

    ret = response(self, OK, content, len, guest_mime_type(filename),
                self->server->static_age, NULL, NULL, NULL);
    free(content);
    return ret;
}

static int send_html(request_t *self, char *html)
{
    if (!self || !html) {
        return 0;
    }
    return response(self, OK, html, strlen(html), "text/html", 0, NULL, NULL, NULL);
}

static int send_srender_by(request_t *self, char *s, dataset_t *ds)
{
    return send_html(self, srender_by(s, ds));
}

static int send_srender(request_t *self, char *s, char *fmt, ...)
{
    va_list ap;
    char *ret;

    va_start(ap, fmt);
    ret = vsrender(s, fmt, ap);
    va_end(ap);

    return send_html(self, ret);
}

static int send_frender_by(request_t *self, char *f, dataset_t *ds)
{
    static char abspath[MAX_URL_LEN];
    join_path(abspath, sizeof(abspath), self->server->template_path, f);
    return send_html(self, frender_by(abspath, ds));
}

static int send_frender(request_t *self, char *f, char *fmt, ...)
{
    va_list ap;
    char *ret;
    static char abspath[MAX_URL_LEN];
    
    join_path(abspath, sizeof(abspath), self->server->template_path, f);

    va_start(ap, fmt);
    ret = vfrender(abspath, fmt, ap);
    va_end(ap);

    return send_html(self, ret);
}

static void free_request(request_t *self)
{
    if (self) {
        --self->session->refer;
        free_req_pkt(self->pkt);
        self->cookies.clear(&self->cookies);
    }
}

int request_init(request_t *self, connection_t *conn, req_pkt_t *pkt, status_code_t *status)
{
    status_code_t code;

    if (!status) {
        status = &code;
    }
    
    if (!self || !conn || !pkt) {
        return 0;
    }
    
    if (self->inited) {
        return 1;
    }

    memset(self, 0, sizeof *self);

    // attibutes
    if (!dict_init(&self->cookies)) {
        return 0;
    }
    self->pkt = pkt;
    self->conn = conn;
    self->server = conn->server;
    self->ip = conn->ip;
    self->port = conn->port;
    self->method = pkt->method;
    self->headers = self->pkt->fields;
    self->version = pkt->version;

    // session
    if (!get_or_new_session(self)) {
        ERROR("Failed to create new session for %s:%u",
            IPV4_BIN_TO_STR(conn->ip), conn->port);
        *status = INSUFFICIENT_STORAGE;
        return 0;
    }

    ++self->session->refer;
    self->session->last_req = get_sys_uptime();

    // methods
    self->send_file = send_file;
    self->send_html = send_html;
    self->send_status = send_status;
    self->response = response;
    self->redirect = redirect;
    self->redirect_top = redirect_top;
    self->redirect_forever = redirect_forever;
    self->get_cookie = get_cookie;
    self->set_cookie = set_cookie;
    self->del_cookie = del_cookie;
    self->get_arg = get_arg;
    self->free = free_request;
    self->send_srender = send_srender;
    self->send_frender = send_frender;
    self->send_srender_by = send_srender_by;
    self->send_frender_by = send_frender_by;

    self->inited = 1;

    return 1;
}

