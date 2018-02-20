#include <sys/sysinfo.h>
#include "print.h"
#include "time.h"

u64 holy_get_now_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (u64)(tv.tv_sec) * 1000 * 1000 + (u64)(tv.tv_usec);
}

char *holy_gmtimestr(time_t t)
{
    static char ts[GMT_TIME_STR_LEN + 1] = {0};
    const u32 tsz = sizeof ts;
    static time_t t0 = 0;
    struct tm utc_tm = {0};
    
    if (t == t0) {
        return ts;
    }
    
    t0 = t;
    
    //Sat, 12 Aug 2017 16:28:26 GMT
    strftime(ts, tsz-1, "%a, %2d %b %Y %T GMT", gmtime_r(&t0, &utc_tm));
    return ts;
}

time_t holy_gmtimeint(char *s)
{
    static time_t t = 0;
    static char *s0 = 0;
    struct tm tm;
    
    if (s == s0) {
        return t;
    }
    
    s0 = s;
    
    //Sat, 12 Aug 2017 16:28:26 GMT
    strptime(s, "%a, %d %b %Y %T GMT", &tm);
    t = timegm(&tm);
    return t;
}

long holy_get_sys_uptime()
{
    struct sysinfo info;
    if (sysinfo(&info) < 0) {
        ERROR_NO("get sysinfo");
        return 0L;
    }
    return info.uptime;
}

