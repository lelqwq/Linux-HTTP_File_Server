#ifndef EVENT_CALLBACKS_H
#define EVENT_CALLBACKS_H

#include <arpa/inet.h>
#include <event2/event.h>
#include <stdlib.h>
#include <string.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include "config/server_config.h"
#include "http/http_dispatch.h"

void cb_read_browser(struct bufferevent *bev, void *arg);
void cb_client_close(struct bufferevent *bev, short events, void *arg);
void cb_listener(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg);

#endif
