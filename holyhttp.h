//
// Created by zengfanfan on 8-31 031.
//

#ifndef HOLYHTTP_H
#define HOLYHTTP_H

#define MAX_CACHED_FILE_SIZE (1024*1024/10240) // 1M, larger file will not be cached
#define HOLY_HASH_BITS  8// the bigger, the faster and larger(memory)
#define MAX_METHOD_LEN              10
#define MAX_URL_LEN                 250
#define MAX_FIELD_NAME_LEN          50// http header field, such as "User-agent: "
#define MAX_FIELD_VALUE_LEN         250
#define MAX_BOUNDARY_LEN            100

#define MAX_HEADER_LEN  1024

typedef enum {
    HOLY_DBG_FATAL = 1,
    HOLY_DBG_ERROR,
    HOLY_DBG_WARN,
    HOLY_DBG_DEBUG,
    HOLY_DBG_DETAIL,
} holy_dbg_lvl_t;

typedef struct {
    // ipv4 address of server, in host byte order
    unsigned ip;// default to [0=all]
    // tcp port of server, in host byte order
    unsigned short port;// default to [80]
    // where the template files locate
    char *template_path;// default to ["template"]
    // where the static files locate
    char *static_path;// default to ["static"]
    // uri that starts with this will be treated as request for static file
    char *static_uri_prefix;// default to ["/static/"]
    // the "max-age" for static file in seconds
    unsigned static_file_age;// default to [3600*24*7=7days]
    // max seconds for a socket(tcp connection) to keep idle
    unsigned socket_timeout;// default to [60=1minute]
    // exceed packets will be treated as illegal
    unsigned max_content_len;// default to [1024*1024*10=10MB]
    // max seconds for a session to keep idle
    unsigned session_timeout;// default to [3600=1hour]
} holycfg_t;

typedef enum {
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2_0,
} version_t;

typedef enum {
    HEAD_METHOD,
    GET_METHOD,
    POST_METHOD,
} method_t;

typedef enum {
    CONTINUE = 100,
    SWICTHING_PROTOCOLS,
    PROCESSING,

    OK = 200,
    CREATED,
    ACCEPTED,
    NON_AUTHORITATIVE_INFOMATION,
    NO_CONTENT,
    RESET_CONTENT,
    PARTIAL_CONTENT,
    MULTI_STATUS,

    MULTI_CHOICES = 300,
    MOVED_PERMANENTLY,
    MOVED_TEMPORARILY,
    SEE_OTHER,
    NOT_MODIFIED,
    USE_PROXY,
    SWITCH_PROXY,
    TEMPORARY_REDIRECT,

    BAD_REQUEST = 400,
    UNAUTHORIZED,
    PAYMENT_REQUIRED,
    FORBIDDEN,
    NOT_FOUND,
    METHOD_NOT_ALLOWED,
    NOT_ACCEPTABLE,
    PROXY_AUTHENTICATION_REQUIRED,
    REQUEST_TIMEOUT,
    CONFLICT,
    GONE,
    LENGTH_REQURED,
    PRECONDITION_FAILED,
    REQUEST_ENTITY_TOO_LARGE,
    REQUEST_URI_TOO_LONG,
    UNSUPPORTED_MEDIA_TYPE,
    REQUESTED_RANGE_NOT_SATISFIABLE,
    EXPECTATION_FAILED,
    TOO_MANY_CONNECTIONS_FROM_YOUR_IP = 421,
    UNPROCESSABLE_ENTITY,
    LOCKED,
    FAILED_DEPENDENCY,
    UNORDERED_COLLECTION,
    UPGRADE_REQUIRED,
    RETRY_WITH = 449,
    UNAVAILABLE_FOR_LEGAL_REASONS = 451,

    INTERNAL_ERROR = 500,
    NOT_IMPLEMENTED,
    BAD_GATEWAY,
    SERVICE_UNAVAILABLE,
    GATEWAY_TIMEOUT,
    VERSION_NOT_SUPPORTED,
    VARIANT_ALSO_NEGOTIATES,
    INSUFFICIENT_STORAGE,
    LOOP_DETECTED,
    BANDWITH_LIMIT_EXCEEDED,
    NOT_EXTENDED,

    UNPARSEABLE_RESPONSE_HEADERS = 600,
} status_code_t;

typedef struct holyreq {
    unsigned ip;
    unsigned short port;
    method_t method;
    version_t version;
    char *url, *uri;
    char *render_separator;// default to ","
     // @incomplete: set to 1 if you dont want req be freed,
     // and you MUST do it yourself with: req->free(req)
    char incomplete;

    int (*send_file)(struct holyreq *self, char *filename);
    int (*send_html)(struct holyreq *self, char *fmt, ...);
    int (*send_status)(struct holyreq *self, status_code_t code);
    int (*send_srender)(struct holyreq *self, char *string, char *fmt, ...);
    int (*send_frender)(struct holyreq *self, char *filename, char *fmt, ...);
    int (*send_srender_by)(struct holyreq *self, char *s, char *args);
    int (*send_frender_by)(struct holyreq *self, char *f, char *args);
    char *(*get_header)(struct holyreq *self, char *name);
    char *(*get_cookie)(struct holyreq *self, char *name);
    int (*set_cookie)(struct holyreq *self, char *name, char *value, int age);
    int (*del_cookie)(struct holyreq *self, char *name);
    char *(*get_session)(struct holyreq *self, char *name);
    int (*set_session)(struct holyreq *self, char *name, char *value);
    char *(*get_arg)(struct holyreq *self, char *name);
    int (*get_bin_arg)(struct holyreq *self, char *name, void **value, unsigned *vlen);
    int (*response)(struct holyreq *self, status_code_t code,
                    void *content, unsigned len, char *type, int age, int chunked,
                    char *location, char *encoding, char *filename);
    int (*redirect)(struct holyreq *self, char *location);
    int (*redirect_top)(struct holyreq *self, char *location);
    int (*redirect_forever)(struct holyreq *self, char *location);
    void (*free)(struct holyreq *self);
    struct holyreq *(*clone)(struct holyreq *self);
} holyreq_t;

typedef void (*holyreq_handler_t)(holyreq_t *req);
typedef int (*prerouting_t)(holyreq_t *req);

/*
    holyhttp_init - initialize the server
    @cfg: configure parameters, see holycfg_t

    This must be done before any other usage of holyhttp

    return: 1-ok, 0-fail
*/
int holyhttp_init(holycfg_t *cfg);

/*
    holyhttp_run - run the server

    This will block forever, like for(;;){}.
*/
void holyhttp_run();

/*
    holyhttp_run_async - run the server asynchronously

    This will not block, and actually create a thread to run the server loop.
*/
//void holyhttp_run_async();

/*
    holyhttp_set_route - set handler of uri
    @uri:
    @handler: see holyreq_handler_t

    If a request to @uri is detected, @handler will be called.

    return: 1-ok, 0-fail
*/
int holyhttp_set_route(char *uri, holyreq_handler_t handler);

/*
    holyhttp_set_debug_level - set the debug level
    @level: see holy_dbg_lvl_t
*/
void holyhttp_set_debug_level(holy_dbg_lvl_t level);

/*
    holyhttp_set_prerouting - set prerouting handler
    @handler: see prerouting_t

    If the request is not for static files or whiteroutes,
    @handler will be proceed.
    If @handler return 1, the request is allowed to proceed forward,
    otherwise it will be ignored.
*/
void holyhttp_set_prerouting(prerouting_t handler);

/*
    holyhttp_set_white_route - set handler of uri
    @uri:
    @handler: see holyreq_handler_t

    This is almost the same with holyhttp_set_route,
    except that it will not proceed prerouting.

    return: 1-ok, 0-fail
*/
int holyhttp_set_white_route(char *uri, holyreq_handler_t handler);

/*
    holyhttp_set_common_render_args - set common args for render
    @separator: separate common args with this
    @fmt: format as printf
    @...: like printf

    When req->send_frender/send_srender, these args are included automaticly.
*/
void holyhttp_set_common_render_args(char *separator, char *fmt, ...);

#endif //HOLYHTTP_H
