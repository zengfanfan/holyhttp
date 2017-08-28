#ifndef HOLYHTTP_TIMER_H
#define HOLYHTTP_TIMER_H

#include <utils/time.h>

typedef void (*timer_handler_t)(void *);

#define FREE_LATER(ptr) if (ptr) set_timeout(0, free, ptr)

void explode_timers(void);
int get_min_timeout(void);
void show_timers(void);
void *set_interval(u32 seconds, timer_handler_t handler, void *arg);
void *set_timeout(u32 seconds, timer_handler_t handler, void *arg);
void kill_timer(void *timer);

#endif // HOLYHTTP_TIMER_H
