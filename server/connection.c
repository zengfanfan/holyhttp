#include <sys/socket.h>
#include <unistd.h>
#include <utils/print.h>
#include <utils/address.h>
#include "connection.h"

#define BUF_DEF_LEN 1024

static char buffer[BUF_DEF_LEN] = {0};
static const u32 buf_sz = sizeof buffer;

#define STATUS_FMT \
    "HTTP/1.1 %3d %s\r\n"\
    "Server: "SERVER_NAME"/"SERVER_VERSION"\r\n"\
    "Connection: close\r\n"\
    "Date: %s\r\n"\
    "Content-Length: 0\r\n\r\n"

static int send_status(connection_t *self, status_code_t code)
{
    if (!self) {
        return 0;
    }
    INFO("%15s:%-5u << %d %s", IPV4_BIN_TO_STR(self->ip), self->port, code, strstatus(code));
    snprintf(buffer, buf_sz, STATUS_FMT, code, strstatus(code), gmtimestr(NOW));
    return self->write(self, buffer, strlen(buffer));
}

static void clear_fd_buf(int fd)
{
    while (recv(fd, buffer, sizeof buffer, 0) > 0);
}

static void close_conn(connection_t *self)
{
    buffer_t *pos, *n;
    dict_t *conns = self->server->conns;

    close(self->fd);

    kill_timer(self->timer);

    list_for_each_entry_safe(pos, n, &self->send_buf, link) {
        list_del(&pos->link);
        free(pos);
    }

    conns->del_i(conns, self->fd);
    
    free(self);
}

static void check_timeout(void *args)
{
    connection_t *self = (connection_t *)args;
    u32 timeout = self->server->cfg.socket_timeout;
    long now = get_sys_uptime();
    long interval = self->last_active - now;
    if (interval < timeout) {
        self->timer = set_timeout(timeout - interval, check_timeout, self);
        return;
    }
    // timeout
    close_conn(self);
}

static int do_recv(connection_t *self, void *buf, u32 len)
{
    int received = (int)recv(self->fd, buf, len, 0);
    if (received < 0) {
        switch (errno) {
        case EWOULDBLOCK:// not ready
            break;// do it next time
        case ECONNRESET:
        case EPIPE:// peer is broken
            close_conn(self);
            break;
        default:
            ERROR_NO("recv from fd %d", self->fd);
            close_conn(self);
            break;
        }
    } else if (received == 0) {
        close_conn(self);
    }

    return received;
}

static void recv_from_peer(connection_t *self, data_handler_t handler)
{
    const char *header = buffer;
    int received;
    char *tmp, *end;
    buffer_t *buf = self->recv_buf;
    u32 clen = 0, hlen;

    if (!self || !handler) {
        return;
    }

    if (buf) {// continous
        received = do_recv(self, buf->data + buf->offset, buf->left);
        if (received <= 0) {
            return;
        }
        
        if (buf->left == received) {
            goto done;
        }

        buf->offset += received;
        buf->left -= received;

        return;// continous
    }

    // first packet, including header

    received = do_recv(self, buffer, buf_sz - 1);
    if (received <= 0) {
        return;
    }
    buffer[received] = 0;
    self->last_active = get_sys_uptime();

    // header length
    end = strstr(header, "\r\n\r\n");
    if (!end) {
        send_status(self, BAD_REQUEST);
        return;
    }
    hlen = end - header + 4;

    // content length
    tmp = strcasestr(header, "Content-Length:");
    if (tmp) {
        sscanf(tmp, "%*[^0-9\r\n]%10u", &clen);
    }

    if (clen > self->server->cfg.max_content_len) {
        send_status(self, REQUEST_ENTITY_TOO_LARGE);
        return;
    }

    if (received >= hlen + clen) {
        received = hlen + clen;
    }

    // make buffer
    buf = self->recv_buf = (buffer_t *)malloc(sizeof(buffer_t) + hlen + clen + 1);
    if (!buf) {
        MEMFAIL();
        send_status(self, INSUFFICIENT_STORAGE);
        return;
    }
    
    buf->left = hlen + clen - received;
    buf->offset = received;
    memcpy(buf->data, header, received);
    buf->data[hlen + clen] = 0;// make sure it's null-terminated
    if (!buf->left) {
        goto done;
    }

    // get the rest
    for (; buf->left; buf->left -= received, buf->offset += received) {
        received = do_recv(self, buf->data + buf->offset, buf->left);
        if (received <= 0) {
            DEBUG("!!!");
            return;// closed or continous
        }
    }

done:
    // ok, handle the new req
    clear_fd_buf(self->fd);
    handler(self, buf->data, buf->offset);
    FREE_IF_NOT_NULL(self->recv_buf);
    self->recv_buf = NULL;
}

static void send_to_peer(connection_t *self)
{
    buffer_t *pos, *n;
    int sent;
    
    list_for_each_entry_safe(pos, n, &self->send_buf, link) {
        // MSG_NOSIGNAL: donot send SIGPIPE signal
        sent = send(self->fd, pos->data + pos->offset, pos->left, MSG_NOSIGNAL);
        if (sent < 0) {
            switch (errno) {
            case EWOULDBLOCK:// not ready
                // without EPOLLONESHOT, we need not to rearm
                return;
            case ECONNRESET:
            case EPIPE:// peer is broken
                close_conn(self);
                return;
            default:
                ERROR_NO("send to fd %d", self->fd);
                close_conn(self);
                return;
            }
        }

        self->send_buf_len -= sent;
        if (sent < pos->left) {// incomplete
            pos->offset += sent;
            pos->left -= sent;
            return;
        }
        
        list_del(&pos->link);
        free(pos);
    }

    if (self->send_buf_len == 0) {
        epoll_mdf_fd(self->server->epfd, self->fd, 1);
    }
}

static int write_to_buf(connection_t *self, void *data, u32 len)
{
    buffer_t *buf;

    if (!self || !data || !len) {
        return 0;
    }

    buf = (buffer_t *)malloc(sizeof *buf + len);
    if (!buf) {
        MEMFAIL();
        send_status(self, INSUFFICIENT_STORAGE);
        return 0;
    }

    memset(buf, 0, sizeof *buf + len);
    buf->left = len;
    memcpy(buf->data, data, len);
    list_add_tail(&buf->link, &self->send_buf);
    self->send_buf_len += len;

    epoll_mdf_fd(self->server->epfd, self->fd, 0);

    return 1;
}

connection_t *new_connection(server_t *server, int fd, u32 ip, u16 port)
{
    connection_t *self;
    
    if (!server) {
        return NULL;
    }

    self = (connection_t *)malloc(sizeof *self);
    if (!self) {
        MEMFAIL();
        return NULL;
    }

    memset(self, 0, sizeof *self);
    self->fd = fd;
    self->ip = ip;
    self->port = port;
    self->server = server;
    self->last_active = get_sys_uptime();
    self->timer = set_timeout(self->server->cfg.socket_timeout, check_timeout, self);
    INIT_LIST_HEAD(&self->send_buf);

    self->recv = recv_from_peer;
    self->send = send_to_peer;
    self->write = write_to_buf;
    self->close = close_conn;
    self->send_status = send_status;

    return self;
}

