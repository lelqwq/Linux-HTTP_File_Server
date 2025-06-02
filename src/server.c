#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "ev_call_back_functions.h"
#include "server_config.h"

struct event_base *evbase;
struct evconnlistener *ev_listener;

int main(int argc, char *argv[])
{
	// 加载配置文件
    load_config("server.conf");
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
	// 连接监听器
	ev_listener = evconnlistener_new_bind(evbase, cb_listener, (void *)evbase, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, (const struct sockaddr *)&server_addr, sizeof(server_addr));
	// 循环监听
	event_base_dispatch(evbase);
	// 释放资源
	evconnlistener_free(ev_listener);
	event_base_free(evbase);
	return 0;
}
