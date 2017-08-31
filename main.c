#include <sys/resource.h>
#include <utils/print.h>
#include <server/server.h>

#define MAX_FD_LIMIT    (1UL << 30)

extern void init_cgi(void);

rlim_t g_rlim_cur, g_rlim_max;

static void set_fd_limit(void)
{
    struct rlimit r;
    int i, limit, step;

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
}


int main(int argc, char *argv[])
{
    set_fd_limit();

    if (!server_init(&holyserver)) {
        FATAL("Failed to init holyserver.");
        return -1;
    }

    init_cgi();

    holyserver.run(&holyserver);
    
    return 0;
}

