#ifndef HTTP_ROUTER_H
#define HTTP_ROUTER_H

#include <event2/bufferevent.h>
#include "http/http_headers.h"

void http_handle_request(char *method, const char *fs_path, const char *url_path, char *protocol, struct bufferevent *bev, http_request_t *req);

#endif
