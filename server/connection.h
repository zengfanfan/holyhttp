#ifndef HOLYHTTP_CONNECTION_H
#define HOLYHTTP_CONNECTION_H

#include "packet.h"
#include "server.h"

typedef struct {
    list_head_t link;
    u32 offset;
    u32 left;
    u8 data[0];
} buffer_t;

#define BUFFER_OFFSET(buf) ((buf)->data + (buf)->offset)

typedef struct connection {
    int fd;
    u32 ip;
    u16 port;
    server_t *server;
    long last_active;
    u16 refer;
    u16 try_close;
    void *timer;

    u32 send_buf_len;
    list_head_t send_buf;
    buffer_t *recv_buf;

    void (*recv)(struct connection *self, int (*handler)(struct connection *, void *, u32));
    void (*send)(struct connection *self);
    int (*write)(struct connection *self, void *data, u32 len);
    void (*close)(struct connection *self);
    int (*send_status)(struct connection *self, status_code_t code);
} connection_t;

typedef int (*data_handler_t)(struct connection *conn, void *data, u32 len);

connection_t *holy_new_conn(server_t *server, int fd, u32 ip, u16 port);

#endif // HOLYHTTP_CONNECTION_H

