#include <utils/print.h>
#include <utils/string.h>
#include "packet.h"

char *holy_strstatus(status_code_t code)
{
    switch (code) {
    case CONTINUE:
        return "Continue";
    case SWICTHING_PROTOCOLS:
        return "Switching Protocols";
    case PROCESSING:
        return "Processing";

    case OK:
        return "OK";
    case CREATED:
        return "Created";
    case ACCEPTED:
        return "Accepted";
    case NON_AUTHORITATIVE_INFOMATION:
        return "Non-Authoritative Information";
    case NO_CONTENT:
        return "No Content";
    case RESET_CONTENT:
        return "Reset Content";
    case PARTIAL_CONTENT:
        return "Partial Content";
    case MULTI_STATUS:
        return "Multi-Status";

    case MULTI_CHOICES:
        return "Multiple Choices";
    case MOVED_PERMANENTLY:
        return "Moved Permanently";
    case MOVED_TEMPORARILY:
        return "Move temporarily";
    case SEE_OTHER:
        return "See Other";
    case NOT_MODIFIED:
        return "Not Modified";
    case USE_PROXY:
        return "Use Proxy";
    case SWITCH_PROXY:
        return "Switch Proxy";
    case TEMPORARY_REDIRECT:
        return "Temporary Redirect";

    case BAD_REQUEST:
        return "Bad Request";
    case UNAUTHORIZED:
        return "Unauthorized";
    case PAYMENT_REQUIRED:
        return "Payment Required";
    case FORBIDDEN:
        return "Forbidden";
    case NOT_FOUND:
        return "Not Found";
    case METHOD_NOT_ALLOWED:
        return "Method Not Allowed";
    case NOT_ACCEPTABLE:
        return "Not Acceptable";
    case PROXY_AUTHENTICATION_REQUIRED:
        return "Proxy Authentication Required";
    case REQUEST_TIMEOUT:
        return "Request Timeout";
    case CONFLICT:
        return "Conflict";
    case GONE:
        return "Gone";
    case LENGTH_REQURED:
        return "Length Required";
    case PRECONDITION_FAILED:
        return "Precondition Failed";
    case REQUEST_ENTITY_TOO_LARGE:
        return "Request Entity Too Large";
    case REQUEST_URI_TOO_LONG:
        return "Request-URI Too Long";
    case UNSUPPORTED_MEDIA_TYPE:
        return "Unsupported Media Type";
    case REQUESTED_RANGE_NOT_SATISFIABLE:
        return "Requested Range Not Satisfiable";
    case EXPECTATION_FAILED:
        return "Expectation Failed";
    case TOO_MANY_CONNECTIONS_FROM_YOUR_IP:
        return "There are too many connections from your internet address";
    case UNPROCESSABLE_ENTITY:
        return "Unprocessable Entity";
    case LOCKED:
        return "Locked";
    case FAILED_DEPENDENCY:
        return "Failed Dependency";
    case UNORDERED_COLLECTION:
        return "Unordered Collection";
    case UPGRADE_REQUIRED:
        return "Upgrade Required";
    case RETRY_WITH:
        return "Retry With";
    case UNAVAILABLE_FOR_LEGAL_REASONS:
        return "Unavailable For Legal Reasons";

    case INTERNAL_ERROR:
        return "Internal Server Error";
    case NOT_IMPLEMENTED:
        return "Not Implemented";
    case BAD_GATEWAY:
        return "Bad Gateway";
    case SERVICE_UNAVAILABLE:
        return "Service Unavailable";
    case GATEWAY_TIMEOUT:
        return "Gateway Timeout";
    case VERSION_NOT_SUPPORTED:
        return "HTTP Version Not Supported";
    case VARIANT_ALSO_NEGOTIATES:
        return "Variant Also Negotiates";
    case INSUFFICIENT_STORAGE:
        return "Insufficient Storage";
    case LOOP_DETECTED:
        return "Loop Detected";
    case BANDWITH_LIMIT_EXCEEDED:
        return "Bandwidth Limit Exceeded";
    case NOT_EXTENDED:
        return "Not Extended";

    case UNPARSEABLE_RESPONSE_HEADERS:
        return "Unparseable Response Headers";

    default:
        return "HOLY-XXX v_v";
    }
}

static char hex2char(char hex)
{
    if (hex >= '0' && hex <= '9') {
        return hex - '0';
    }
    if (hex >= 'A' && hex <= 'Z') {
        return hex - 'A' + 0xA;
    }
    if (hex >= 'a' && hex <= 'z') {
        return hex - 'a' + 0xa;
    }

    return hex;
}

static void url_decode(char *url)
{
    int i, j;
    int qlen = strlen(url);

    for (i = j = 0; i < qlen; ++i, ++j) {
        if (url[i] == '%' && (i+2) < qlen) {
            url[j] = (hex2char(url[i+1])<<4) | hex2char(url[i+2]);
            i += 2;
        } else if (url[i] == '+'){
            url[j] = ' ';
        } else {
            url[j] = url[i];
        }
    }

    url[j] = 0;
}

static int parse_method(req_pkt_t *self, char *method, status_code_t *status)
{
    if (!strcmp(method, "HEAD")) {
        self->method = HEAD_METHOD;
        return 1;
    } else  if (!strcmp(method, "GET")) {
        self->method = GET_METHOD;
        return 1;
    } else if (!strcmp(method, "POST")) {
        self->method = POST_METHOD;
        return 1;
    } else {
        *status = METHOD_NOT_ALLOWED;
        return 0;
    }
}

static int parse_requset_line(req_pkt_t *self, char *data, status_code_t *status)
{
    int ret;
    char fmt[MAX_URL_LEN];
    u32 major_ver, sub_ver;

    // GET /uri?query=1&key=value HTTP/1.1\r\n
    snprintf(fmt, sizeof fmt, "%%%ds %%%ds HTTP/%%u.%%u\r\n", MAX_METHOD_LEN, MAX_URL_LEN);
    ret = sscanf((char *)data, fmt, self->mth_str, self->url, &major_ver, &sub_ver);
    if (ret != 4) {
        *status = BAD_REQUEST;
        return 0;
    }

    if (!parse_method(self, self->mth_str, status)) {
        return 0;
    }

    if (strlen(self->url) >= MAX_URL_LEN) {
        *status = REQUEST_URI_TOO_LONG;
        return 0;
    }

    if (major_ver == 1 && sub_ver == 0) {
        self->version = HTTP_1_0;
    } else if (major_ver == 1 && sub_ver == 1) {
        self->version = HTTP_1_1;
    } else if (major_ver == 2 && sub_ver == 0) {
        self->version = HTTP_2_0;
    } else {
        *status = VERSION_NOT_SUPPORTED;
        return 0;
    }

    return 1;

}

static int parse_query(req_pkt_t *self, char *query, status_code_t *status)
{
    char *name, *value, *tmp, *pair = NULL;
    char empty[] = "";

    // name=value&name=value
    foreach_item_in_str(pair, query, "&") {
        name = pair;
        tmp = strchr(name, '=');
        if (!tmp) {
            value = empty;// value may be absent
        } else {
            *tmp = 0;
            value = tmp + 1;
        }

        url_decode(name);
        url_decode(value);

        self->args->set_ss(self->args, name, value);
    }

    return 1;
}

static int parse_headers(req_pkt_t *self, char *data, status_code_t *status, char **content)
{
    int ret;
    char fmt[MAX_URL_LEN];
    char name[MAX_FIELD_NAME_LEN + 1], value[MAX_FIELD_VALUE_LEN + 1];
    char *fields_start, *fields_end, *fields, *line = NULL, *tmp;
    int fields_len;

    fields_start = strstr(data, "\r\n");
    if (!fields_start) {
        *status = BAD_REQUEST;
        return 0;
    }

    fields_end = strstr(fields_start, "\r\n\r\n");
    if (!fields_end || fields_start == fields_end) {
        *status = BAD_REQUEST;
        return 0;
    }

    // includes \r\n\r\n, and null-terminated
    fields_len = fields_end - fields_start + 4;
    fields = (char *)malloc(fields_len + 1);
    if (!fields) {
        MEMFAIL();
        *status = INSUFFICIENT_STORAGE;
        return 0;
    }
    memcpy(fields, fields_start, fields_len);
    fields[fields_len] = 0;

    // name: value\r\n
    snprintf(fmt, sizeof fmt, "%%%d[^:]:%%*[ ]%%%d[^\r\n]",
        MAX_FIELD_NAME_LEN, MAX_FIELD_VALUE_LEN);

    foreach_item_in_str(line, fields, "\r\n") {
        ret = sscanf(line, fmt, name, value);
        if (ret != 2) {
            continue;
        }
        holy_str2lower(name);
        self->fields->set_ss(self->fields, name, value);
    }

    if (self->method == POST_METHOD) {
        tmp = self->fields->get_ss(self->fields, "content-length");
        if (tmp) {
            self->content_length = atoi(tmp);
        }
    }

    tmp = self->fields->get_ss(self->fields, "content-type");
    if (tmp) {
        // type/subtype; parameters
        snprintf(fmt, sizeof fmt, "%%%d[^/;]/%%%d[^;] %%%d[^\r\n]",
            MAX_CONTENT_MAJOR_TYPE_LEN,
            MAX_CONTENT_SUB_TYPE_LEN,
            MAX_CONTENT_TYPE_PARAM_LEN);
        sscanf(tmp, fmt,
            self->content_type.type,
            self->content_type.subtype,
            self->content_type.param);
    }

    *content = fields_end + 4;

    free(fields);
    return 1;
}

static int parse_content(req_pkt_t *self, char *content, status_code_t *status)
{
    int ret;
    char fmt[MAX_URL_LEN];
    char boundary[MAX_BOUNDARY_LEN + 1];
    char delimiter[MAX_BOUNDARY_LEN * 2 + 1];
    char *start = content, *end = content + self->content_length - 1;
    char *tmp, *pos, *next;
    u32 len;
    char name[MAX_FIELD_NAME_LEN + 1];

    if (self->method != POST_METHOD) {
        return 1;
    }

    if (!strcasecmp(self->content_type.type, "application")
            && !strcasecmp(self->content_type.subtype, "x-www-form-urlencoded")) {
        return parse_query(self, content, status);
    }

    if (strcasecmp(self->content_type.type, "multipart")
            || strcasecmp(self->content_type.subtype, "form-data")) {
        return 1;
    }

    tmp = strstr(self->content_type.param, "boundary=");
    if (!tmp) {
        return parse_query(self, content, status);
    }

    snprintf(fmt, sizeof fmt, "boundary=%%%d[^;\r\n]", MAX_BOUNDARY_LEN);
    ret = sscanf(tmp, fmt, boundary);
    if (ret != 1) {
        return parse_query(self, content, status);
    }

    // the last boundary
    ret = snprintf(delimiter, sizeof delimiter, "\r\n--%s--\r\n", boundary);
    len = strlen(delimiter);

    if (ret > len) {
        *status = INTERNAL_ERROR;
        ERROR("boundary too long(%d)", ret);
        return 0;
    }

    if (memcmp(end - len + 1, delimiter, len)) {
        *status = BAD_REQUEST;
        return 0;
    }

    end -= len;

    // the first boundary
    ret = snprintf(delimiter, sizeof delimiter, "--%s\r\n", boundary);
    if (!holy_str_starts_with(start, delimiter)) {
        *status = BAD_REQUEST;
        return 0;
    }
    start += ret;

    // the split boundary
    snprintf(delimiter, sizeof delimiter, "\r\n--%s\r\n", boundary);
    len = strlen(delimiter);
    for (pos = start; pos < end; pos = next + len) {
        next = holy_strnstr(pos, delimiter, end - pos + 1);
        if (!next) {
            next = end + 1;
        }

        // [now part is from pos to next]
        // content-disposition: form-data; name="xxxxx"\r\n\r\n<...data...>
        for (pos = holy_strnstr(pos, "name=\"", next - pos);
            (pos && pos < next) && isalnum(pos[-1]);
            pos = holy_strnstr(pos, "name=\"", next - pos));
        if (!(pos && pos < next)) {
            ERROR("Name not found in form-data");
            continue;
        }

        snprintf(fmt, sizeof fmt, "name=\"%%%d[^\"]", MAX_FIELD_NAME_LEN);
        ret = sscanf(pos, fmt, name);
        if (ret != 1) {
            ERROR("Name not found in form-data");
            continue;
        }

        pos = holy_strnstr(pos, "\r\n\r\n", next - pos);
        if (!pos) {
            ERROR("Data not found in form-data");
            continue;
        }

        pos += 4;// [now data is from pos to next]

        self->args->set_sb(self->args, name, pos, next - pos);
    }

    return 1;
}

static int parse_cookies(req_pkt_t *self, status_code_t *status)
{
    int ret;
    char fmt[MAX_URL_LEN], *pair = NULL;
    char name[MAX_FIELD_NAME_LEN + 1], value[MAX_FIELD_VALUE_LEN + 1];
    char *cookies = self->fields->get_ss(self->fields, "cookie");

    cookies = cookies ? strdup(cookies) : NULL;
    if (!cookies) {
        return 1;
    }

    // name=value; name=value
    snprintf(fmt, sizeof fmt, "%%%d[^=]=%%%d[^;\r\n]",
        MAX_FIELD_NAME_LEN, MAX_FIELD_VALUE_LEN);

    foreach_item_in_str(pair, cookies, ";") {
        value[0] = 0; // value may be absent
        ret = sscanf(pair, fmt, name, value);
        if (ret != 1 && ret != 2) {
            continue;
        }
        holy_str_trim(name, ' ');
        self->cookies->set_ss(self->cookies, name, value);
    }

    free(cookies);
    return 1;
}

req_pkt_t *holy_new_req_pkt(char *data, u32 len, status_code_t *status)
{
    req_pkt_t tmp = {0}, *pkt;
    status_code_t code;
    char *query, *content;

    if (!status) {
        status = &code;
    }

    if (!data || !len) {
        *status = BAD_REQUEST;
        return NULL;
    }

    tmp.args = holy_new_dict();
    tmp.fields = holy_new_dict();
    tmp.cookies = holy_new_dict();
    if (!tmp.args || !tmp.fields || !tmp.cookies) {
        *status = INSUFFICIENT_STORAGE;
        goto fail;
    }

    if (!parse_requset_line(&tmp, data, status)) {
        goto fail;
    }

    query = strchr(tmp.url, '?');
    if (query) {
        strncpy(tmp.uri, tmp.url, query - tmp.url);
        query = strdup(query+1);
        if (!query) {
            goto fail;
        }
        if (!parse_query(&tmp, query, status)) {
            free(query);
            goto fail;
        }
        free(query);
    } else {
        strcpy(tmp.uri, tmp.url);
    }

    if (!parse_headers(&tmp, data, status, &content)) {
        goto fail;
    }

    if (!parse_content(&tmp, content, status)) {
        goto fail;
    }

    if (!parse_cookies(&tmp, status)) {
        goto fail;
    }

    pkt = (req_pkt_t *)malloc(sizeof tmp + tmp.content_length);
    if (!pkt) {
        MEMFAIL();
        *status = INSUFFICIENT_STORAGE;
        goto fail;
    }

    memcpy(pkt, &tmp, sizeof tmp);
    if (tmp.content_length) {
        memcpy(pkt->content, content, tmp.content_length);
    }

    *status = OK;
    return pkt;

fail:
    holy_free_dict(tmp.args);
    holy_free_dict(tmp.fields);
    holy_free_dict(tmp.cookies);
    return NULL;
}

void holy_free_req_pkt(req_pkt_t *self)
{
    if (self) {
        holy_free_dict(self->args);
        holy_free_dict(self->fields);
        holy_free_dict(self->cookies);
        free(self);
    }
}

