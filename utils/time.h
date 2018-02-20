#ifndef HOLYHTTP_TIME_H
#define HOLYHTTP_TIME_H

#include <time.h>
#include <sys/time.h>
#include "types.h"

#define NOW time(NULL)
#define GMT_TIME_STR_LEN    30//Sat, 12 Aug 2017 16:28:26 GMT
#define SECPERYEAR (3600 * 24 * 366)

u64 holy_get_now_us();
char *holy_gmtimestr(time_t t);
time_t holy_gmtimeint(char *s);
long holy_get_sys_uptime();

#endif // HOLYHTTP_TIME_H
