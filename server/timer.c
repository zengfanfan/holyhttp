#include <stdlib.h>
#include <utils/print.h>
#include <utils/rbtree.h>
#include "timer.h"

typedef struct {
    rb_node_t link;
    u64 interval; // us
    u64 expiry;
    int oneshot;
    timer_handler_t handler;
    void *arg;
} holytimer_t;

static rb_root_t timers = RB_ROOT;

static void insert_timer(holytimer_t *t)
{
    rb_root_t *root = &timers;
    rb_node_t **tmp = &(root->rb_node), *parent = NULL;
    u64 key = t->expiry;
    holytimer_t *pos;

    /* Figure out where to put new node */
    while (*tmp) {
        pos = rb_entry(*tmp, holytimer_t, link);

        parent = *tmp;
        if (key < pos->expiry) {
            tmp = &((*tmp)->rb_left);
        } else {// same key is allowed
            tmp = &((*tmp)->rb_right);
        }
    }

    /* Add new node and rebalance tree. */
    rb_link_node(&t->link, parent, tmp);
    rb_insert_color(&t->link, root);
}

static void remove_timer(holytimer_t *t)
{
    rb_erase(&t->link, &timers);
}

void kill_timer(void *timer)
{
    holytimer_t *p = (holytimer_t *)timer;
    if (p) {
        remove_timer(p);
        free(p);
    }
}

void *set_timeout(u32 seconds, timer_handler_t handler, void *arg)
{
    holytimer_t *t = (holytimer_t *)malloc(sizeof *t);
    if (!t) {
        MEMFAIL();
        return NULL;
    }

    memset(t, 0, sizeof *t);
    t->handler = handler;
    t->arg = arg;
    t->oneshot = 1;
    t->interval = seconds * 1000 * 1000;
    t->expiry = get_now_us() + t->interval;

    insert_timer(t);
    return t;
}

void *set_interval(u32 seconds, timer_handler_t handler, void *arg)
{
    holytimer_t *t = (holytimer_t *)malloc(sizeof *t);
    if (!t) {
        MEMFAIL();
        return NULL;
    }

    memset(t, 0, sizeof *t);
    t->handler = handler;
    t->arg = arg;
    t->oneshot = 0;
    t->interval = seconds * 1000 * 1000;
    t->expiry = get_now_us() + t->interval;

    insert_timer(t);
    return t;
}

void show_timers(void)
{
    u64 now = get_now_us();
    rb_node_t *node;
    holytimer_t *t;
    int i;

    DEBUG("NOW: %llu", now);
    for (node = rb_first(&timers), i = 0; node; node = rb_next(node), ++i) {
        t = rb_entry(node, holytimer_t, link);
        DEBUG("[%d] %p %llu", i, t, t->expiry);
    }
}

void explode_timers(void)
{
    rb_node_t *node;
    holytimer_t *t;
    u64 now = get_now_us();

    for (node = rb_first(&timers); node; node = rb_first(&timers)) {
        t = rb_entry(node, holytimer_t, link);
        if (t->expiry > now) {
            break;
        }
        
        remove_timer(t);
        t->handler(t->arg);
        
        if (t->oneshot) {
            free(t);
        } else {
            t->expiry = now + t->interval;
            insert_timer(t);
        }
    }
}

i64 get_min_timeout(void)
{
    u64 now = get_now_us();
    rb_node_t *node = rb_first(&timers);
    holytimer_t *t;
    i64 timeout;

    if (!node) {
        return SECPERYEAR * 1000LL;
    }

    t = rb_entry(node, holytimer_t, link);
    timeout = t->expiry - now;
    timeout /= 1000; // us -> ms
    return MAX(timeout, 1000);
}

