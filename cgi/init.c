#include "cgi.h"

void init_cgi(void)
{
    holyserver.route(&holyserver, "test", cgi_test);
    holyserver.route(&holyserver, "test/string", cgi_test_string);
    holyserver.route(&holyserver, "test/file", cgi_test_file);
}

