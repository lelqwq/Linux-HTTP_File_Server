#ifndef EV_CALL_BACK_FUNCTIONS_H
#define EV_CALL_BACK_FUNCTIONS_H

#include <arpa/inet.h>
#include <sys/stat.h>
#include <event2/event.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include "read_http_line.h"
#include "http_functions.h"
#include "url_decode.h"
#include "server_config.h"

void cb_read_browser(struct bufferevent *bev, void *arg);
void cb_client_close(struct bufferevent *bev, short events, void *arg);
void cb_listener(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg);

#endif