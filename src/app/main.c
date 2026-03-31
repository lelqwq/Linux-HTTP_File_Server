#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include <signal.h>
#include "event_callbacks.h"
#include "server_config.h"
#include "server_stats.h"

struct event_base *evbase;
struct evconnlistener *ev_listener;

void stats_timer_cb(evutil_socket_t fd, short what, void *arg)
{
	(void)fd;
	(void)what;
	(void)arg;
	print_server_stats();
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	signal(SIGPIPE, SIG_IGN);
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		perror("getcwd error");
		return -1;
	}
	init_server_stats();
	load_config("server.conf");
	if (config.log_file[0] != '/')
	{
		char full_log_path[PATH_MAX];
		snprintf(full_log_path, sizeof(full_log_path), "%s/%s", cwd, config.log_file);
		strncpy(config.log_file, full_log_path, sizeof(config.log_file) - 1);
		config.log_file[sizeof(config.log_file) - 1] = '\0';
	}
	if (chdir(config.root_dir) == -1)
	{
		perror("chdir error");
		return -1;
	}
	struct sockaddr_in server_addr;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(config.port);
	evbase = event_base_new();
	struct timeval interval = {config.stats_logging_interval, 0};
	struct event *stats_timer = event_new(evbase, -1, EV_PERSIST, stats_timer_cb, NULL);
	event_add(stats_timer, &interval);
	ev_listener = evconnlistener_new_bind(evbase, cb_listener, (void *)evbase, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, (const struct sockaddr *)&server_addr, sizeof(server_addr));
	event_base_dispatch(evbase);
	evconnlistener_free(ev_listener);
	event_base_free(evbase);
	return 0;
}
