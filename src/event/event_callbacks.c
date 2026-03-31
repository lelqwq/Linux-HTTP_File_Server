#include <errno.h>
#include <stdio.h>
#include "event_callbacks.h"
#include "server_stats.h"

void cb_read_browser(struct bufferevent *bev, void *arg)
{
	(void)arg;
	http_dispatch_on_read(bev);
}

void cb_client_close(struct bufferevent *bev, short events, void *arg)
{
	struct sockaddr_in *client_addr = (struct sockaddr_in *)arg;
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_addr->sin_addr, ip, sizeof(ip));
	if (events & BEV_EVENT_EOF)
	{
		if (config.enable_connection_info)
			printf("Connection %s:%d closed by peer.\n", ip, ntohs(client_addr->sin_port));
	}
	else if (events & BEV_EVENT_ERROR)
	{
		atomic_fetch_add(&stats.total_errors, 1);
		if (config.enable_connection_info)
			printf("Connection %s:%d error: %s\n", ip, ntohs(client_addr->sin_port), strerror(errno));
	}
	else if (events & BEV_EVENT_TIMEOUT)
	{
		if (config.enable_connection_info)
			printf("Connection %s:%d timeout.\n", ip, ntohs(client_addr->sin_port));
	}
	free(client_addr);
	bufferevent_free(bev);
	atomic_fetch_sub(&stats.current_connections, 1);
}

void cb_listener(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg)
{
	(void)listener;
	(void)socklen;
	struct event_base *evbase = (struct event_base *)arg;
	struct sockaddr_in *client_addr = (struct sockaddr_in *)sa;
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_addr->sin_addr, ip, sizeof(ip));
	if (config.enable_connection_info != 0)
	{
		printf("New connection from %s:%d\n", ip, ntohs(client_addr->sin_port));
	}
	struct sockaddr_in *client_addr_copy = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	memcpy(client_addr_copy, client_addr, sizeof(struct sockaddr_in));
	struct bufferevent *bev = bufferevent_socket_new(evbase, fd, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, cb_read_browser, NULL, cb_client_close, (void *)client_addr_copy);
	struct timeval read_timeout = {config.timeout_seconds, 0};
	bufferevent_set_timeouts(bev, &read_timeout, NULL);
	bufferevent_setwatermark(bev, EV_READ, 0, 4096);
	bufferevent_enable(bev, EV_READ);
	atomic_fetch_add(&stats.current_connections, 1);
}
