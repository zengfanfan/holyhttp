#include "cgi.h"

status_code_t cgi_test(request_t *req)
{
    req->send_html(req, "baby is ok!");
    return OK;
}

status_code_t cgi_test_string(request_t *req)
{
    req->send_srender(req,
        "Hello, @name! Are you @age?",
        "name=%s,age=%d",
        "baby", get_now_us() % 100);
    return OK;
}

status_code_t cgi_test_file(request_t *req)
{
    req->send_frender(req,
        "test.html",
        "name=%s,age=%d,a.1=%s,a.2=%s,a.b.1=A,a.b.2=B,a.b.3=C",
        "baby", get_now_us() % 100, "Zeng", "Fanfan");
    return OK;
}

