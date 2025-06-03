#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "ev_call_back_functions.h"
#include "server_config.h"
#include "server_stats.h"
#include <linux/limits.h>

struct event_base *evbase;
struct evconnlistener *ev_listener;

void stats_timer_cb(evutil_socket_t fd, short what, void *arg)
{
	print_server_stats();
}

int main(int argc, char *argv[])
{
	// 获取当前工作目录
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		perror("getcwd error");
		return -1;
	}
	// 初始化性能统计
	init_server_stats();
	// 加载配置文件
	load_config("server.conf");
	// 检查日志文件路径是否为绝对路径
    if (config.log_file[0] != '/')// 相对路径
    {
        // 拼接日志文件路径
        char full_log_path[PATH_MAX];
        snprintf(full_log_path, sizeof(full_log_path), "%s/%s", cwd, config.log_file);
        strncpy(config.log_file, full_log_path, sizeof(config.log_file) - 1);
        config.log_file[sizeof(config.log_file) - 1] = '\0'; // 确保日志文件路径以 '\0' 结尾
    }
	// 设置当前工作目录
	if (chdir(config.root_dir) == -1)
	{
		perror("chdir error");
		return -1;
	}
	// sockaddr of server
	struct sockaddr_in server_addr;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(config.port);
	// 初始化event_base
	evbase = event_base_new();
	// 创建定时器事件
	struct timeval interval = {config.stats_logging_interval, 0}; // 每5秒打印一次性能统计
	struct event *stats_timer = event_new(evbase, -1, EV_PERSIST, stats_timer_cb, NULL);
	event_add(stats_timer, &interval);
	// 连接监听器
	ev_listener = evconnlistener_new_bind(evbase, cb_listener, (void *)evbase, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, (const struct sockaddr *)&server_addr, sizeof(server_addr));
	// 循环监听
	event_base_dispatch(evbase);
	// 释放资源
	evconnlistener_free(ev_listener);
	event_base_free(evbase);
	return 0;
}
