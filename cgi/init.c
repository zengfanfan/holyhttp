#include "cgi.h"

void init_cgi(void)
{
    server.route(&server, "test", cgi_test);
    server.route(&server, "test/string", cgi_test_string);
    server.route(&server, "test/file", cgi_test_file);
}

