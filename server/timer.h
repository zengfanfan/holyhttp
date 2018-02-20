#ifndef HOLYHTTP_TIMER_H
#define HOLYHTTP_TIMER_H

#include <utils/time.h>

typedef void (*timer_handler_t)(void *);

#define FREE_LATER(ptr) if (ptr) holy_set_timeout(1, free, ptr)

void holy_explode_timers(void);
i64 holy_get_min_timeout(void);
void holy_show_timers(void);
void *holy_set_interval(u32 seconds, timer_handler_t handler, void *arg);
void *holy_set_timeout(u32 seconds, timer_handler_t handler, void *arg);
void holy_kill_timer(void *timer);

#endif // HOLYHTTP_TIMER_H
