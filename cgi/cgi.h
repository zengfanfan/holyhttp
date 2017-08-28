#ifndef HOLYHTTP_CGI_H
#define HOLYHTTP_CGI_H

#include <server/request.h>
#include <server/template.h>

status_code_t cgi_test(request_t *req);
status_code_t cgi_test_string(request_t *req);
status_code_t cgi_test_file(request_t *req);

#endif // HOLYHTTP_CGI_H
