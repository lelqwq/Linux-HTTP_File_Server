#include "ev_call_back_functions.h"

void cb_read_browser(struct bufferevent *bev, void *arg)
{
	// get the first line of request
	// 如果浏览器点击的超链接带有中文，那么浏览器会将中文编码为字母符号发送请求。
	// 因此解析路径时需要解码为中文原始路径
	char http_line[1024];
	int ret = read_http_line(bev, http_line, sizeof(http_line));
	if (ret == -1)
	{
		printf("no http\n");
		return;
	}
	printf("%s\n", http_line);
	// clear the rest of bufferevent readbuffer
	while (1)
	{
		char temp[1024] = {0};
		int ret = read_http_line(bev, temp, sizeof(temp));
		// printf("%s\n",temp);
		if (ret == -1)
		{
			break;
		}
	}
	// split the first HTTP line
	char method[16], temp_path[256], protocol[16];
	sscanf(http_line, "%15s %255s %15s", method, temp_path, protocol);

	char path[256];
	char url_path[256];
	if (strcmp(temp_path, "/") == 0)
	{
		strcpy(path, ".");
		strcpy(url_path, "/");
	}
	else
	{
		// 解码 URL，生成文件系统路径
		char decoded_path[256];
		url_decode(decoded_path, temp_path + 1); // 去掉开头的 /
		strcpy(path, decoded_path);              // 本地访问路径
		strcpy(url_path, temp_path);             // 原始 URL 路径
	}
	// 简单防御路径穿越攻击
	char real_path[1024];
	if (realpath(path, real_path) == NULL)
	{
		send_error(bev, protocol, 403, "Invalid path");
		return;
	}
	// 设置服务器根目录
	char server_root[] = "./";
	char server_real[1024];
	realpath(server_root, server_real);

	size_t root_len = strlen(server_real);
	if (strncmp(real_path, server_real, root_len) != 0 ||
		(real_path[root_len] != '/' && real_path[root_len] != '\0'))
	{
		send_error(bev, protocol, 403, "Forbidden");
		return;
	}
	// adjust what kind of request(as though this server was designed for only GET request)
	if (strcmp(method, "GET") == 0) // GET request
	{
		http_request(method, path, url_path, protocol, bev);
	}
}

void cb_client_close(struct bufferevent *bev, short events, void *arg)
{
	struct sockaddr_in *client_addr = (struct sockaddr_in *)arg;
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_addr->sin_addr, ip, sizeof(ip));
	if (BEV_EVENT_EOF & events)
	{
		printf("Connection %s:%d closed.\n", ip, ntohs(client_addr->sin_port));
		free(client_addr);
		bufferevent_free(bev);
	}
}

void cb_listener(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg)
{
	struct event_base *evbase = (struct event_base *)arg;
	// sockaddr_in of client_addr
	struct sockaddr_in *client_addr = (struct sockaddr_in *)sa;
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_addr->sin_addr, ip, sizeof(ip));
	printf("New connection from %s:%d\n", ip, ntohs(client_addr->sin_port));
	// copy client_addr for call_back function
	struct sockaddr_in *client_addr_copy = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	memcpy(client_addr_copy, client_addr, sizeof(struct sockaddr_in));
	// new client bufferevent
	struct bufferevent *bev = bufferevent_socket_new(evbase, fd, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, cb_read_browser, NULL, cb_client_close, (void *)client_addr_copy);
	bufferevent_enable(bev, EV_READ);
}