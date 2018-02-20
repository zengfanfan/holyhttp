#ifndef HOLYHTTP_SERVER_H
#define HOLYHTTP_SERVER_H

#include <utils/dict.h>
#include <utils/string.h>
#include <holyhttp.h>
#include "timer.h"

#define SERVER_NAME     "HolyHttp"
#define SERVER_VERSION  "0.1"

#define MAX_PATH_LEN    250
#define ARGS_BUF_LEN    (10*1024)

typedef struct server_s {
    holycfg_t cfg;
    int fd;
    int epfd;
    int inited;
    
    dict_t *routes;
    dict_t *whiteroutes;
    dict_t *conns;
    dict_t *sessions;

    char *common_separator;
    char common_args[ARGS_BUF_LEN];
    
    int (*run)(struct server_s *self);
    int (*set_route)(struct server_s *self, char *uri, void *handler);
    int (*set_whiteroute)(struct server_s *self, char *uri, void *handler);
    void *prerouting;

    time_t start;
    char start_time[GMT_TIME_STR_LEN + 1];
} server_t;

extern server_t holyserver;

int holy_epoll_add_fd(int epfd, int fd, int read);
int holy_epoll_mdf_fd(int epfd, int fd, int read);
int holy_epoll_del_fd(int epfd, int fd, int read);
int holy_server_init(server_t *self, holycfg_t *cfg);

#endif // HOLYHTTP_SERVER_H
