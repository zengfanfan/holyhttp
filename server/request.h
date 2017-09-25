#ifndef HOLYHTTP_REQUEST_H
#define HOLYHTTP_REQUEST_H

#include <utils/types.h>
#include <holyhttp.h>
#include "template.h"
#include "packet.h"
#include "session.h"
#include "connection.h"

typedef struct request {
    holyreq_t base;
    int inited;
    req_pkt_t *pkt;
    server_t *server;
    connection_t *conn;
    dict_t cookies; // to be set/del, response to client
    dict_t *headers;
    session_t *session;

    void (*free)(struct request *self);
} request_t;

int request_init(request_t *self, connection_t *conn, req_pkt_t *pkt, status_code_t *status);

#endif // HOLYHTTP_REQUEST_H

