#ifndef HOLYHTTP_CONFIG_H
#define HOLYHTTP_CONFIG_H

#define SERVER_PORT     80
#define TEMPLATE_PATH   "templates"
#define STATIC_PATH     "/root/v/Coverity"
#define STATIC_URI_PREFIX   "/static/" // MUST starts and ends tiwh '/'
#define STATIC_FILE_AGE (3600 * 24 * 7)
#define BIND_ADDRESS    "*"
#define SOCK_TIMEOUT    60
#define MAX_PATH_LEN    500
#define MAX_URL_LEN     1000
#define MAX_CONTENT_LENGTH  (1024*1024*10)//10M
#define SESSION_ID_AGE  (3600*24*7)
#define SESSION_AGE     3600

#define SERVER_NAME     "HolyHttp"
#define SERVER_VERSION  "0.1"
#define SESSION_ID_NAME SERVER_NAME"-sessionid"

#endif // HOLYHTTP_CONFIG_H
