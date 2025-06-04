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
	// split the first HTTP line
	char method[16], temp_path[256], protocol[16];
	sscanf(http_line, "%15s %255s %15s", method, temp_path, protocol);

	// 去除查询参数，例如处理 video_template.html?video=...
	char *query = strchr(temp_path, '?');
	if (query)
		*query = '\0';

	char path[256];
	char url_path[256];
	char video_path[256] = {0}; // 存储视频路径参数
	// 检查是否是视频模板页面请求
	if (strstr(temp_path, "video_player.html") && query)
	{
		strcpy(path, "video/video_player.html"); // 视频模板页面的实际路径
		strcpy(url_path, temp_path);
		// 解析视频路径参数
		query++; // 跳过 '?'
		if (strncmp(query, "video=", 6) == 0)
		{
			char *video_param = query + 6;
			url_decode(video_path, video_param); // 存储视频路径，供后续使用
		}
	}
	else if (strcmp(temp_path, "/") == 0)
	{
		strcpy(path, ".");
		strcpy(url_path, "/");
	}
	else
	{
		// 解码 URL，生成文件系统路径
		char decoded_path[256];
		url_decode(decoded_path, temp_path + 1); // 去掉开头的 /
		strcpy(path, decoded_path);				 // 本地访问路径
		strcpy(url_path, temp_path);			 // 原始 URL 路径
	}
	// 设置服务器根目录
	char server_root[] = "./";
	char server_real[1024];
	realpath(server_root, server_real);
	// 检查模板页面路径
	char real_path[1024];
	if (realpath(path, real_path) == NULL)
	{
		send_error(bev, protocol, 403, "Invalid path");
		return;
	}

	size_t root_len = strlen(server_real);
	if (strncmp(real_path, server_real, root_len) != 0 ||
		(real_path[root_len] != '/' && real_path[root_len] != '\0'))
	{
		send_error(bev, protocol, 403, "Forbidden");
		return;
	}
	// 如果是视频请求，检查视频文件路径
	if (video_path[0] != '\0')
	{
		char video_real_path[1024];
		if (realpath(video_path, video_real_path) == NULL)
		{
			send_error(bev, protocol, 404, "Video file not found");
			return;
		}
		// 检查视频文件是否在允许的目录内
		if (strncmp(video_real_path, server_real, strlen(server_real)) != 0)
		{
			send_error(bev, protocol, 403, "Forbidden video path");
			return;
		}
	}
	// 解析 HTTP 请求头
	http_request_t req;
	parse_http_headers(bev, &req);
	// adjust what kind of request(as though this server was designed for only GET request)
	if (strcmp(method, "GET") == 0) // GET request
	{
		// 增加总请求数
		atomic_fetch_add(&stats.total_requests, 1);
		http_request(method, path, url_path, protocol, bev, &req);
	}
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
		// 增加错误计数
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
	// 减少当前连接数
	atomic_fetch_sub(&stats.current_connections, 1);
}

void cb_listener(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg)
{
	struct event_base *evbase = (struct event_base *)arg;
	// sockaddr_in of client_addr
	struct sockaddr_in *client_addr = (struct sockaddr_in *)sa;
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_addr->sin_addr, ip, sizeof(ip));
	if (config.enable_connection_info != 0)
	{
		printf("New connection from %s:%d\n", ip, ntohs(client_addr->sin_port));
	}
	// copy client_addr for call_back function
	struct sockaddr_in *client_addr_copy = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	memcpy(client_addr_copy, client_addr, sizeof(struct sockaddr_in));
	// new client bufferevent
	struct bufferevent *bev = bufferevent_socket_new(evbase, fd, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, cb_read_browser, NULL, cb_client_close, (void *)client_addr_copy);
	// set read timeout
	struct timeval read_timeout = {config.timeout_seconds, 0};
	bufferevent_set_timeouts(bev, &read_timeout, NULL);
	// 设置读写上限，防止缓存无限制增长
	bufferevent_setwatermark(bev, EV_READ, 0, 4096);
	// enable read event
	bufferevent_enable(bev, EV_READ);
	// 增加当前连接数
	atomic_fetch_add(&stats.current_connections, 1);
}