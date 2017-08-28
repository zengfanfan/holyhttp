#ifndef HOLYHTTP_SERVER_H
#define HOLYHTTP_SERVER_H

#include <utils/dict.h>
#include <utils/string.h>
#include "timer.h"

#define PATH_LEN    200

typedef struct server_s {
    u16 port;
    int fd;
    int epfd;
    int ip;
    int inited;
    
    char template_path[PATH_LEN + 1];
    char static_path[PATH_LEN + 1];
    char static_route[PATH_LEN + 1];
    int static_age;
    u32 timeout;
    dict_t *routes;
    dict_t *conns;
    dict_t *sessions;
    
    int (*run)(struct server_s *self);
    int (*route)(struct server_s *self, char *route, void *handler);

    time_t start;
    char start_time[GMT_TIME_STR_LEN + 1];
} server_t;

extern server_t server;

int epoll_add_fd(int epfd, int fd, int read);
int epoll_mdf_fd(int epfd, int fd, int read);
int epoll_del_fd(int epfd, int fd, int read);
int server_init(server_t *self);

#endif // HOLYHTTP_SERVER_H
