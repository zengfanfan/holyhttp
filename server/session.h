#ifndef HOLYHTTP_SESSION_H
#define HOLYHTTP_SESSION_H

#include <utils/dict.h>
#include "server.h"

#define SESSION_ID_NAME SERVER_NAME"-sessionid"
#define SESSION_ID_LEN  24 // pid[0-8.hex] + bkdr(UA+port+ip)[8.hex] + rand()[0-8.hex]

typedef struct session {
    char id[SESSION_ID_LEN + 1];
    dict_t *parent;
    dict_t *dict;
    long last_req;
    u32 timeout;
    u32 refer;
} session_t;

#endif // HOLYHTTP_SESSION_H
