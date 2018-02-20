#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <utils/print.h>
#include <utils/address.h>
#include "server.h"
#include "connection.h"
#include "request.h"

#define EPOLL_EVT_SZ    1024

#if 0
#define EPOLL_MODE EPOLLET
#else
#define EPOLL_MODE 0//LT
#endif

#ifndef TCP_DEFER_ACCEPT
#define TCP_DEFER_ACCEPT 9
#endif

typedef void (*route_handler_t)(holyreq_t *req);

server_t holyserver;

static int server_listen(server_t *self)
{
    int reuse = 1;
    struct sockaddr_in addr;    

    self->fd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, IPPROTO_TCP);
    if (self->fd < 0) {
        FATAL_NO("create socket");
        return 0;
    }
    setsockopt(self->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    //setsockopt(self->fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    
    setsockopt(self->fd, IPPROTO_TCP, TCP_DEFER_ACCEPT,
        &self->cfg.socket_timeout, sizeof(self->cfg.socket_timeout));
    
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(self->cfg.port);
    if (bind(self->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        FATAL_NO("bind %s:%d", IPV4_BIN_TO_STR(self->cfg.ip), self->cfg.port);
        close(self->fd);
        return 0;
    }

    if (listen(self->fd, EPOLL_EVT_SZ) < 0) {
        FATAL_NO("listen server socket");
        close(self->fd);
        return 0;
    }

    return 1;
}

int holy_epoll_add_fd(int epfd, int fd, int read)
{
    struct epoll_event evt;
    evt.data.fd = fd;
    evt.events = EPOLL_MODE|EPOLLRDHUP;
    evt.events |= read ? EPOLLIN : EPOLLOUT;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &evt) < 0) {
        ERROR_NO("add epoll %c%d", read?'r':'w', fd);
        return 0;
    } else {
        return 1;
    }
}

int holy_epoll_mdf_fd(int epfd, int fd, int read)
{
    struct epoll_event evt;
    evt.data.fd = fd;
    evt.events = EPOLL_MODE|EPOLLRDHUP;
    evt.events |= read ? EPOLLIN : EPOLLOUT;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &evt) < 0) {
        ERROR_NO("set epoll %c%d", read?'r':'w', fd);
        return 0;
    } else {
        return 1;
    }
}

int holy_epoll_del_fd(int epfd, int fd, int read)
{
    struct epoll_event evt;
    evt.data.fd = fd;
    evt.events = EPOLL_MODE|EPOLLRDHUP;
    evt.events |= read ? EPOLLIN : EPOLLOUT;
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &evt) < 0) {
        ERROR_NO("del epoll %c%d", read?'r':'w', fd);
        return 0;
    } else {
        return 1;
    }
}

static int accept_connection(server_t *self)
{
    struct sockaddr_in src;
    u32 slen = sizeof(src);
    int fd, flags;
    connection_t *conn;

    memset(&src, 0, slen);

    fd = accept(self->fd, (struct sockaddr *)&src, (socklen_t *)&slen);
    if (fd < 0) {
        ERROR_NO("accept socket");
        return 0;
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags|O_NONBLOCK) < 0) {
        ERROR_NO("set socket %d non-blocking", fd);
        // fall through
    }

    holy_epoll_add_fd(self->epfd, fd, 1);

    conn = holy_new_conn(self, fd, (u32)ntohl(src.sin_addr.s_addr), ntohs(src.sin_port));
    if (!conn) {
        ERROR("Failed to create connection for fd %d", fd);
        close(fd);
        return 0;
    }
    
    self->conns->set_ip(self->conns, fd, conn);
    
    return 1;
}

static int handle_static(holyreq_t *req)
{
    char path[MAX_PATH_LEN], real[MAX_PATH_LEN];
    char *static_uri_prefix = ((request_t *)req)->server->cfg.static_uri_prefix;
    char *static_path = ((request_t *)req)->server->cfg.static_path;
    char *relative_path = req->uri + strlen(static_uri_prefix);
    if (relative_path[0] == '/') {
        ++relative_path;
    }
    holy_get_real_path(relative_path, real, sizeof real);
    holy_join_path(path, sizeof path, static_path, real);
    return req->send_file(req, path);
}

static int handle_dynamic(holyreq_t *req)
{
    route_handler_t handler;
    prerouting_t prerouting = ((request_t *)req)->server->prerouting;
    dict_t *routes = ((request_t *)req)->server->routes;
    dict_t *whiteroutes = ((request_t *)req)->server->whiteroutes;
    char path[MAX_PATH_LEN], *uri;
    char *static_uri_prefix = ((request_t *)req)->server->cfg.static_uri_prefix;

    if (!strcasecmp(req->uri, "/favicon.ico")) {
        holy_join_path(path, sizeof path, static_uri_prefix, "favicon.ico");
        return req->redirect_forever(req, path);
    }
    
    uri = strdup(req->uri);
    if (!uri) {
        return req->send_status(req, INSUFFICIENT_STORAGE);
    }

    holy_str_trim(uri, '/');
    handler = whiteroutes->get_sp(whiteroutes, uri);
    if (handler) {
        free(uri);
        goto handle;
    }
    
    if (prerouting && !prerouting(req)) {
        free(uri);
        return 1;// ignore
    }

    
    handler = routes->get_sp(routes, uri);
    free(uri);

handle:

    if (!handler) {
        return req->send_status(req, NOT_FOUND);
    }

    handler(req);
    return 1;
}

static int handle_data(connection_t *conn, void *data, u32 len)
{
    req_pkt_t *pkt;
    status_code_t status;
    request_t req = {.inited=0};
    int ret;

    pkt = holy_new_req_pkt(data, len, &status);
    if (!pkt) {
        ERROR("data from %s:%d is malformed!", IPV4_BIN_TO_STR(conn->ip), conn->port);
        conn->send_status(conn, status);
        return 0;
    }

    INFO("%15s:%-5u >> %s %s",
        IPV4_BIN_TO_STR(conn->ip), conn->port, pkt->mth_str, pkt->uri);

    if (!holy_request_init(&req, conn, pkt, &status)) {
        ERROR("Failed(%d) to init request for %s:%d, %s.",
            status, IPV4_BIN_TO_STR(conn->ip), conn->port, holy_strstatus(status));
        conn->send_status(conn, status);
        holy_free_req_pkt(pkt);
        return 0;
    }

    if (holy_str_starts_with(pkt->uri, conn->server->cfg.static_uri_prefix)) {
        ret = handle_static((holyreq_t *)&req);
    } else {
        ret = handle_dynamic((holyreq_t *)&req);
    }

    if (req.base.incomplete) {
        INFO("request is not freed for incomplete");
    } else {
        req.base.free((holyreq_t *)&req);
    }

    return ret;
}

static void read_data(server_t *self, int fd)
{
    connection_t *conn = self->conns->get_ip(self->conns, (long)fd);
    if (!conn) {
        ERROR("Cannot find connection for fd %d", fd);
        return;
    }

    conn->recv(conn, handle_data);
}

static void write_data(server_t *self, int fd)
{
    connection_t *conn = self->conns->get_ip(self->conns, (long)fd);
    if (!conn) {
        ERROR("Cannot find connection for fd %d", fd);
        return;
    }

    conn->send(conn);
}

static void close_conn_by_fd(server_t *self, int fd)
{
    connection_t *conn = self->conns->get_ip(self->conns, (long)fd);
    if (!conn) {
        ERROR("Cannot find connection for fd %d", fd);
        return;
    }

    conn->close(conn);
}

static int run_server(server_t *self)
{
    struct epoll_event evts[EPOLL_EVT_SZ];
    int nfds, i, fd;
    i64 timeout;
    
    self->epfd = epoll_create(EPOLL_EVT_SZ);
    if (self->epfd < 0) {
        FATAL_NO("create epoll");
        close(self->fd);
        return 0;
    }

    if (!holy_epoll_add_fd(self->epfd, self->fd, 1)) {
        close(self->fd);
        close(self->epfd);
        return 0;
    }
    
    memset(evts, 0, sizeof(evts));
    self->start = NOW;
    strcpy(self->start_time, holy_gmtimestr(self->start));

    for (;;) {
        timeout = holy_get_min_timeout();
        nfds = epoll_wait(self->epfd, evts, EPOLL_EVT_SZ, timeout);
        if (nfds < 0) {
            ERROR_NO("wait epoll");
            sleep(1);
            continue;
        }

        // handle sockets
        for (i = 0; i < nfds; i++) {
            fd = evts[i].data.fd;
            
            if (fd == self->fd) {// acceptable
                accept_connection(self);
            } else if (evts[i].events & EPOLLIN) {// readable
                read_data(self, fd);
            } else if (evts[i].events & EPOLLOUT) {// writable
                write_data(self, fd);
            } else {
                if (!(evts[i].events & EPOLLRDHUP)) {
                    ERROR("Unexpected epoll event %x for fd %d", evts[i].events, fd);
                }
                close_conn_by_fd(self, fd);
            }
        }

        // handle timers
        holy_explode_timers();
    }

    return 1;
}

static int set_route(server_t *self, char *uri, void *handler)
{
    char *uri_cp;
    int ret;
    
    if (!self || !uri || !handler) {
        return 0;
    }

    uri_cp = strdup(uri);
    if (!uri_cp) {
        return 0;
    }

    holy_str_trim(uri_cp, '/');
    ret = self->routes->set_sp(self->routes, uri_cp, handler);
    free(uri_cp);
    return ret;
}

static int set_whiteroute(server_t *self, char *uri, void *handler)
{
    char *uri_cp;
    int ret;
    
    if (!self || !uri || !handler) {
        return 0;
    }

    uri_cp = strdup(uri);
    if (!uri_cp) {
        return 0;
    }

    holy_str_trim(uri_cp, '/');
    ret = self->whiteroutes->set_sp(self->whiteroutes, uri_cp, handler);
    free(uri_cp);
    return ret;
}

static int check_n_set_cfg(server_t *self, holycfg_t *cfg)
{
    self->cfg = *cfg;
    if (!cfg->port) {
        self->cfg.port = 80;
    }
    if (!cfg->static_file_age) {
        self->cfg.static_file_age = 7*24*3600;
    }
    if (!cfg->socket_timeout) {
        self->cfg.socket_timeout = 60;
    }
    if (!cfg->session_timeout) {
        self->cfg.session_timeout = 3600;
    }
    if (!cfg->max_content_len) {
        self->cfg.max_content_len = 10*1024*1024;
    }
    if (!cfg->template_path) {
        self->cfg.template_path = "template";
    }
    if (!cfg->static_path) {
        self->cfg.static_path = "static";
    }
    if (!cfg->static_uri_prefix) {
        self->cfg.static_uri_prefix = "/static/";
    }
    
    self->cfg.template_path = strdup(self->cfg.template_path);
    self->cfg.static_path = strdup(self->cfg.static_path);
    self->cfg.static_uri_prefix = strdup(self->cfg.static_uri_prefix);
    
    if (!self->cfg.template_path || !self->cfg.static_path || !self->cfg.static_uri_prefix) {
        FREE_IF_NOT_NULL(self->cfg.template_path);
        FREE_IF_NOT_NULL(self->cfg.static_path);
        FREE_IF_NOT_NULL(self->cfg.static_uri_prefix);
        return 0;
    }
    
    return 1;
}

int holy_server_init(server_t *self, holycfg_t *cfg)
{
    holycfg_t defcfg = {0};
    
    if (!self) {
        return 0;
    }

    if (self->inited == 0x9527) {
        return 1;
    }

    if (!self->common_separator) {
        self->common_separator = ",";
    }
    
    if (!cfg) {
        cfg = &defcfg;
    }

    if (!check_n_set_cfg(self, cfg)) {
        return 0;
    }

    if (!server_listen(self)) {
        FREE_IF_NOT_NULL(self->cfg.template_path);
        FREE_IF_NOT_NULL(self->cfg.static_path);
        FREE_IF_NOT_NULL(self->cfg.static_uri_prefix);
        return 0;
    }

    self->conns = holy_new_dict();
    self->routes = holy_new_dict();
    self->whiteroutes = holy_new_dict();
    self->sessions = holy_new_dict();
    if (!self->conns || !self->routes || !self->whiteroutes || !self->sessions) {
        FREE_IF_NOT_NULL(self->cfg.template_path);
        FREE_IF_NOT_NULL(self->cfg.static_path);
        FREE_IF_NOT_NULL(self->cfg.static_uri_prefix);
        FREE_IF_NOT_NULL(self->conns);
        FREE_IF_NOT_NULL(self->routes);
        FREE_IF_NOT_NULL(self->whiteroutes);
        FREE_IF_NOT_NULL(self->sessions);
        return 0;
    }

    self->run = run_server;
    self->set_route = set_route;
    self->set_whiteroute = set_whiteroute;

    self->inited = 0x9527;
    return 1;
}

