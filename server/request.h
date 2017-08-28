#ifndef HOLYHTTP_REQUEST_H
#define HOLYHTTP_REQUEST_H

#include <utils/types.h>
#include "template.h"
#include "packet.h"
#include "session.h"
#include "connection.h"

typedef struct request {
    int inited;
    u32 ip;
    u16 port;
    req_pkt_t *pkt;
    server_t *server;
    connection_t *conn;
    method_t method;
    version_t version;
    dict_t cookies; // to be set/del, response to client
    dict_t *headers;
    session_t *session;

    int (*send_file)(struct request *self, char *filename);
    int (*send_html)(struct request *self, char *html);
    int (*send_status)(struct request *self, status_code_t code);
    int (*send_srender)(struct request *self, char *s, char *fmt, ...);
    int (*send_frender)(struct request *self, char *f, char *fmt, ...);
    int (*send_srender_by)(struct request *self, char *s, dataset_t *ds);
    int (*send_frender_by)(struct request *self, char *f, dataset_t *ds);
    str_t (*get_cookie)(struct request *self, char *name);
    int (*set_cookie)(struct request *self, char *name, char *value, int age);
    int (*del_cookie)(struct request *self, char *name);
    int (*get_arg)(struct request *self, char *name, void **value, u32 *vlen);
    int (*response)(struct request *self, status_code_t code,
            void *content, u32 len, char *type, int age,
            char *location, char *encoding,
            char *filename);
    int (*redirect)(struct request *self, char *location);
    int (*redirect_top)(struct request *self, char *location);
    int (*redirect_forever)(struct request *self, char *location);
    void (*free)(struct request *self);
} request_t;

int request_init(request_t *self, connection_t *conn, req_pkt_t *pkt, status_code_t *status);

#endif // HOLYHTTP_REQUEST_H

