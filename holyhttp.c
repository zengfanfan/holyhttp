#include <sys/resource.h>
#include <utils/print.h>
#include <server/server.h>
#include <holyhttp.h>

#define MAX_FD_LIMIT    (1UL << 30)

rlim_t g_rlim_cur, g_rlim_max;
holy_dbg_lvl_t holydebug = HOLY_DBG_ERROR;

static void set_fd_limit(void)
{
    struct rlimit r;
    u32 i, limit, step;

    for (limit = MAX_FD_LIMIT; limit > 0; limit >>= 1) {
        r.rlim_cur = r.rlim_max = limit;
        if (setrlimit(RLIMIT_NOFILE, &r) == 0) {// success
            step = limit / 10;// try 10 times at most
            for (i = limit; i > 0; i -= step) {
                r.rlim_max = r.rlim_cur = limit + i;
                if (setrlimit(RLIMIT_NOFILE, &r) == 0) {
                    break;
                }
            }
            break;
        }
    }

    getrlimit(RLIMIT_NOFILE, &r);
    g_rlim_cur = r.rlim_cur;
    g_rlim_max = r.rlim_max;
    DEBUG("fd limit = %lu %lu", g_rlim_cur, g_rlim_max);
}

int holyhttp_init(holycfg_t *cfg)
{
    static int inited = 0;

    if (inited) {
        return 1;
    }

    set_fd_limit();

    if (!server_init(&holyserver, cfg)) {
        FATAL("Failed to init server.");
        return 0;
    }

    return 1;
}

void holyhttp_run()
{
    holyserver.run(&holyserver);
}

int holyhttp_set_route(char *uri, holyreq_handler_t handler)
{
    return holyserver.route(&holyserver, uri, handler);
}

void holyhttp_set_debug_level(holy_dbg_lvl_t level)
{
    if (level <= 0) {
        holydebug = HOLY_DBG_ERROR;
    } else {
        holydebug = level;
    }
}

