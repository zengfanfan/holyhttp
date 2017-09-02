#ifndef HOLYHTTP_PACKET_H
#define HOLYHTTP_PACKET_H

#include <utils/dict.h>
#include <holyhttp.h>

#define MAX_METHOD_LEN          10
#define MAX_URI_LEN             250
#define MAX_FIELD_NAME_LEN      50
#define MAX_FIELD_VALUE_LEN     250
#define MAX_CONTENT_MAJOR_TYPE_LEN  30
#define MAX_CONTENT_SUB_TYPE_LEN    30
#define MAX_CONTENT_TYPE_PARAM_LEN  100
#define MAX_BOUNDARY_LEN        100

#define MAX_HEADER_LEN  1024

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
