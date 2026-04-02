#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include <signal.h>
#include "event_callbacks.h"
#include "server_config.h"
#include "server_stats.h"

// 定时器回调函数，输出服务器状态
void stats_timer_cb(evutil_socket_t fd, short what, void *arg)
{
	(void)fd;
	(void)what;
	(void)arg;
	print_server_stats();
}

int main(int argc, char *argv[])
{
	// 目前没有使用到argc和argv
	(void)argc;
	(void)argv;

	// 忽略SIGPIPE信号，防止向已经关闭的socket写入数据时发生SIGPIPE信号导致进程退出
	signal(SIGPIPE, SIG_IGN);

	// 获取当前工作目录
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		perror("getcwd error");
		return -1;
	}

	// 初始化服务器统计信息
	init_server_stats();

	// 加载配置文件
	load_config("server.conf");

	// 如果日志文件路径不是绝对路径，则拼接当前工作目录和日志文件路径，放到根目录下
	if (config.log_file[0] != '/')
	{
		char full_log_path[PATH_MAX];
		snprintf(full_log_path, sizeof(full_log_path), "%s/%s", cwd, config.log_file);
		strncpy(config.log_file, full_log_path, sizeof(config.log_file) - 1);
		config.log_file[sizeof(config.log_file) - 1] = '\0';
	}

	// 切换工作目录到配置文件中的根目录
	if (chdir(config.root_dir) == -1)
	{
		perror("chdir error");
		return -1;
	}

	// 创建服务器地址
	struct sockaddr_in server_addr;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(config.port);
	// 创建event_base
	struct event_base *p_evbase = event_base_new();
	// 创建统计定时器
	struct timeval tv_stats_timer = {config.stats_timer, 0};
	// 创建统计定时器事件
	struct event *p_ev_stats_timer = event_new(p_evbase, -1, EV_PERSIST, stats_timer_cb, NULL);
	// 添加统计定时器事件到event_base
	event_add(p_ev_stats_timer, &tv_stats_timer);
	// 创建监听器
	struct evconnlistener *p_ev_listener = evconnlistener_new_bind(p_evbase, cb_listener, (void *)p_evbase, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, (const struct sockaddr *)&server_addr, sizeof(server_addr));
	// 启动事件循环
	event_base_dispatch(p_evbase);
	// 释放监听器
	evconnlistener_free(p_ev_listener);
	// 释放event_base
	event_base_free(p_evbase);
	return 0;
}
