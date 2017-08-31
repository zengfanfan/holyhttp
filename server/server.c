#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <config.h>
#include <utils/print.h>
#include <utils/address.h>
#include "server.h"
#include "connection.h"
#include "request.h"

#define EPOLL_EVT_SZ    1024

#ifndef TCP_DEFER_ACCEPT
#define TCP_DEFER_ACCEPT 9
#endif

typedef status_code_t (*route_handler_t)(request_t *req);

server_t holyserver;

static int server_listen(server_t *self)
{
    int reuse = 1, flags = 0;
    struct sockaddr_in addr;    

    self->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (self->fd < 0) {
        FATAL("Failed(%d) to create socket, %s.", errno, strerror(errno));
        return 0;
    }
    setsockopt(self->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    setsockopt(self->fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    setsockopt(self->fd, IPPROTO_TCP, TCP_DEFER_ACCEPT,
        &self->timeout, sizeof(self->timeout));
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(self->port);
    if (bind(self->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        FATAL("Failed(%d) to bind %s:%d, %s.",
            errno, IPV4_BIN_TO_STR(self->ip), self->port, strerror(errno));
        close(self->fd);
        return 0;
    }

    if (listen(self->fd, EPOLL_EVT_SZ) < 0) {
        FATAL("Failed(%d) to listen socket, %s.", errno, strerror(errno));
        close(self->fd);
        return 0;
    }

    flags = fcntl(self->fd, F_GETFL, 0);
    if (fcntl(self->fd, F_SETFL, flags|O_NONBLOCK) < 0) {
        ERROR("Failed(%d) to set socket non-blocking, %s.", errno, strerror(errno));
        // fall through
    }
    
    return 1;
}

int epoll_add_fd(int epfd, int fd, int read)
{
    struct epoll_event evt;
    evt.data.fd = fd;
    evt.events = EPOLLET|EPOLLRDHUP;
    evt.events |= read ? EPOLLIN : EPOLLOUT;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &evt) < 0) {
        ERROR("Failed to set epoll %c%d failed, %s.", read?'r':'w', fd, strerror(errno));
        return 0;
    } else {
        return 1;
    }
}

int epoll_mdf_fd(int epfd, int fd, int read)
{
    struct epoll_event evt;
    evt.data.fd = fd;
    evt.events = EPOLLET|EPOLLRDHUP;
    evt.events |= read ? EPOLLIN : EPOLLOUT;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &evt) < 0) {
        ERROR("Failed to set epoll %c%d failed, %s.", read?'r':'w', fd, strerror(errno));
        return 0;
    } else {
        return 1;
    }
}

int epoll_del_fd(int epfd, int fd, int read)
{
    struct epoll_event evt;
    evt.data.fd = fd;
    evt.events = EPOLLET|EPOLLRDHUP;
    evt.events |= read ? EPOLLIN : EPOLLOUT;
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &evt) < 0) {
        ERROR("Failed to del epoll %c%d failed, %s.", read?'r':'w', fd, strerror(errno));
        return 0;
    } else {
        return 1;
    }
}

static int accept_connection(server_t *self)
{
    struct sockaddr_in src;
    int slen = sizeof(src);
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

    epoll_add_fd(self->epfd, fd, 1);

    conn = new_connection(self, fd, (u32)ntohl(src.sin_addr.s_addr), ntohs(src.sin_port));
    if (!conn) {
        close(fd);
        return 0;
    }

    self->conns->set_ip(self->conns, fd, conn);
    
    return 1;
}

static int handle_static(request_t *req)
{
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof path, "%s/%s",
        req->server->static_path, req->pkt->uri + strlen(req->server->static_route));
    return req->send_file(req, path);
}

static int handle_dynamic(request_t *req)
{
    route_handler_t handler;
    dict_t *routes = req->server->routes;
    char path[MAX_PATH_LEN];
    status_code_t status;
    char *uri;
    
    if (!strcasecmp(req->pkt->uri, "/favicon.ico")) {
        snprintf(path, sizeof path, "%sfavicon.ico", req->server->static_route);
        return req->redirect_forever(req, path);
    }
    
    uri = strdup(req->pkt->uri);
    if (!uri) {
        return req->send_status(req, INSUFFICIENT_STORAGE);
    }

    str_trim(uri, "/");
    handler = (route_handler_t)routes->get_sp(routes, uri);
    free(uri);

    if (!handler) {
        return req->send_status(req, NOT_FOUND);
    }

    status = handler(req);
    if (status == OK) {
        return 1;
    } else {
        return req->send_status(req, status);
    }
}

static int handle_data(connection_t *conn, void *data, u32 len)
{
    req_pkt_t *pkt;
    status_code_t status;
    request_t req = {.inited=0};
    int ret;

    pkt = new_req_pkt(data, len, &status);
    if (!pkt) {
        ERROR("data from %s:%d is malformed!", IPV4_BIN_TO_STR(conn->ip), conn->port);
        conn->send_status(conn, status);
        return 0;
    }

    PRINT_CYAN("%15s:%-5u >> %s %s\n",
        IPV4_BIN_TO_STR(conn->ip), conn->port, pkt->mth_str, pkt->uri);

    if (!request_init(&req, conn, pkt, &status)) {
        ERROR("Failed(%d) to init request for %s:%d, %s.",
            status, IPV4_BIN_TO_STR(conn->ip), conn->port, strstatus(status));
        conn->send_status(conn, status);
        free_req_pkt(pkt);
        return 0;
    }

    if (str_starts_with(pkt->uri, conn->server->static_route)) {
        ret = handle_static(&req);
    } else {
        ret = handle_dynamic(&req);
    }
    
    req.free(&req);

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
    int nfds, i, fd, timeout;
    
    self->epfd = epoll_create(EPOLL_EVT_SZ);
    if (self->epfd < 0) {
        FATAL("Failed(%d) to create epoll, %s.", errno, strerror(errno));
        close(self->fd);
        return 0;
    }

    if (!epoll_add_fd(self->epfd, self->fd, 1)) {
        close(self->fd);
        close(self->epfd);
        return 0;
    }
    
    memset(evts, 0, sizeof(evts));
    self->start = NOW;
    strcpy(self->start_time, gmtimestr(self->start));

    for (;;) {
        timeout = get_min_timeout();
        nfds = epoll_wait(self->epfd, evts, EPOLL_EVT_SZ, timeout * 1000);
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
            } else if (evts[i].events & EPOLLRDHUP) {
                close_conn_by_fd(self, fd);
            } else if (evts[i].events & EPOLLIN) {// readable
                read_data(self, fd);
            } else if (evts[i].events & EPOLLOUT) {// writable
                write_data(self, fd);
            } else {
                ERROR("Unexpected epoll event %x for fd %d", evts[i].events, fd);
                close_conn_by_fd(self, fd);
            }
        }

        // handle timers
        explode_timers();
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

    str_trim(uri_cp, "/");
    ret = self->routes->set_sp(self->routes, uri, handler);
    free(uri_cp);
    return ret;
}

int server_init(server_t *self)
{
    if (!self) {
        return 0;
    }

    if (self->inited == 0x9527) {
        return 1;
    }

    // set arguments
    self->port = SERVER_PORT;
    self->timeout = SOCK_TIMEOUT;
    strncpy(self->template_path, TEMPLATE_PATH, sizeof self->template_path - 1);
    strncpy(self->static_path, STATIC_PATH, sizeof self->static_path - 1);
    strncpy(self->static_route, STATIC_URI_PREFIX, sizeof self->static_route - 1);
    self->static_age = STATIC_FILE_AGE;
    if (!BIND_ADDRESS || !BIND_ADDRESS[0] || !strcmp(BIND_ADDRESS, "*")) {
        self->ip = 0;
    } else {
        self->ip = IPV4_STR_TO_BIN(BIND_ADDRESS);
    }

    if (!server_listen(self)) {
        return 0;
    }

    self->conns = new_dict();
    self->routes = new_dict();
    self->sessions = new_dict();
    if (!self->conns || !self->routes || !self->sessions) {
        FREE_IF_NOT_NULL(self->conns);
        FREE_IF_NOT_NULL(self->routes);
        FREE_IF_NOT_NULL(self->sessions);
        return 0;
    }

    self->run = run_server;
    self->route = set_route;

    self->inited = 0x9527;
    return 1;
}

