#ifndef HOLYHTTP_PACKET_H
#define HOLYHTTP_PACKET_H

#include <utils/dict.h>

#define MAX_METHOD_LEN          10
#define MAX_URI_LEN             250
#define MAX_FIELD_NAME_LEN      50
#define MAX_FIELD_VALUE_LEN     250
#define MAX_CONTENT_MAJOR_TYPE_LEN  30
#define MAX_CONTENT_SUB_TYPE_LEN    30
#define MAX_CONTENT_TYPE_PARAM_LEN  100
#define MAX_BOUNDARY_LEN        100

#define MAX_HEADER_LEN  1024

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

typedef struct {
    method_t method;
    char mth_str[MAX_METHOD_LEN + 1];
    char uri[MAX_URI_LEN + 1];
    version_t version;
    dict_t *fields; // case-insensitive
    dict_t *cookies; // case-sensitive
    dict_t *args;
    u32 content_length;
    struct {
        char type[MAX_CONTENT_MAJOR_TYPE_LEN + 1];
        char subtype[MAX_CONTENT_SUB_TYPE_LEN + 1];
        char param[MAX_CONTENT_TYPE_PARAM_LEN + 1];
    } content_type;
    char content[0];
} req_pkt_t;

char *strstatus(status_code_t code);
req_pkt_t *new_req_pkt(char *data, u32 len, status_code_t *status);
void free_req_pkt(req_pkt_t *self);

#endif // HOLYHTTP_PACKET_H
