#ifndef HOLYHTTP_PACKET_H
#define HOLYHTTP_PACKET_H

#include <utils/dict.h>
#include <holyhttp.h>

#define MAX_CONTENT_MAJOR_TYPE_LEN  30
#define MAX_CONTENT_SUB_TYPE_LEN    30
#define MAX_CONTENT_TYPE_PARAM_LEN  100

typedef struct {
    method_t method;
    char mth_str[MAX_METHOD_LEN + 1];
    char url[MAX_URL_LEN + 1];
    char uri[MAX_URL_LEN + 1];// without query string
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

char *holy_strstatus(status_code_t code);
req_pkt_t *holy_new_req_pkt(char *data, u32 len, status_code_t *status);
void holy_free_req_pkt(req_pkt_t *self);

#endif // HOLYHTTP_PACKET_H
