#include <errno.h>
#include <stdio.h>
#include "event/event_callbacks.h"
#include "stats/server_stats.h"

/*
 * cb_client_read — bufferevent 读回调（有数据可读时由 libevent 调用）。
 *
 * 功能：将连接交给 http_dispatch_on_read，完成请求行、路径校验、请求头解析与 GET 处理。
 *
 * 参数：
 *   p_bev  对应该客户端连接的 bufferevent；
 *   arg    与 cb_client_close 相同，为 cb_listener 里 malloc 的客户端地址副本；本回调未使用。
 *
 * 返回值：无。
 */
void cb_client_read(struct bufferevent *p_bev, void *arg)
{
	(void)arg;
	http_dispatch_on_read(p_bev);
}

/*
 * cb_client_close — bufferevent 事件回调（连接结束或异常时调用）。
 *
 * 功能：根据 events 区分对端关闭(EOF)、错误(ERROR，含写失败等)、读超时(TIMEOUT)；
 *       按需打印日志、累加 total_errors，释放 cb_listener 中 malloc 的客户端地址副本，
 *       释放 bufferevent，并将 current_connections 减一。
 *
 * 参数：
 *   p_bev  本连接的 bufferevent；
 *   events libevent 事件位，如 BEV_EVENT_EOF / BEV_EVENT_ERROR / BEV_EVENT_TIMEOUT；
 *   arg    必须为 (struct sockaddr_in *)，与 bufferevent_setcb 传入一致，由 cb_listener malloc。
 *
 * 返回值：无。
 */
void cb_client_close(struct bufferevent *p_bev, short events, void *arg)
{
	struct sockaddr_in *p_client_addr = (struct sockaddr_in *)arg;
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &p_client_addr->sin_addr, ip, sizeof(ip));
	if (events & BEV_EVENT_EOF)
	{
		if (config.enable_connection_info)
			printf("Connection %s:%d closed by peer.\n", ip, ntohs(p_client_addr->sin_port));
	}
	else if (events & BEV_EVENT_ERROR)
	{
		atomic_fetch_add(&stats.total_errors, 1);
		if (config.enable_connection_info)
			printf("Connection %s:%d error: %s\n", ip, ntohs(p_client_addr->sin_port), strerror(errno));
	}
	else if (events & BEV_EVENT_TIMEOUT)
	{
		if (config.enable_connection_info)
			printf("Connection %s:%d timeout.\n", ip, ntohs(p_client_addr->sin_port));
	}
	free(p_client_addr);
	bufferevent_free(p_bev);
	atomic_fetch_sub(&stats.current_connections, 1);
}

/*
 * cb_listener — evconnlistener 在 accept 到新 TCP 连接后的回调。
 *
 * 功能：从 p_arg 恢复 event_base；解析对端地址并可打印新连接信息；
 *       malloc 拷贝 sockaddr_in 供 cb_client_close 使用；为 fd 创建 bufferevent，
 *       注册读/事件回调、读超时与读高水位，启用读事件；current_connections 加一。
 *
 * 参数：
 *   p_evlistener  触发本次回调的监听器（本实现未使用）；
 *   fd            accept 得到的新连接套接字；
 *   p_sa          对端地址（实为 struct sockaddr_in *）；
 *   socklen       p_sa 长度（本实现未使用）；
 *   p_arg         与 evconnlistener_new_bind 传入一致，此处为 (struct event_base *)。
 *
 * 返回值：无。
 */
void cb_listener(struct evconnlistener *p_evlistener, evutil_socket_t fd, struct sockaddr *p_sa, int socklen, void *p_arg)
{
	(void)p_evlistener;
	(void)socklen;
	struct event_base *p_evbase = (struct event_base *)p_arg;
	struct sockaddr_in *p_client_addr = (struct sockaddr_in *)p_sa;

	// 获取客户端IP地址
	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &p_client_addr->sin_addr, client_ip, sizeof(client_ip));

	// 如果启用连接信息，则打印连接信息
	if (config.enable_connection_info != 0)
	{
		printf("New connection from %s:%d\n", client_ip, ntohs(p_client_addr->sin_port));
	}

	// 创建客户端地址拷贝
	// p_client_addr的内存是临时分配的，需要拷贝一份给cb_client_close使用
	struct sockaddr_in *p_client_addr_copy = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	memcpy(p_client_addr_copy, p_client_addr, sizeof(struct sockaddr_in));

	// 创建bufferevent，后续读写基于这个bufferevent进行
	struct bufferevent *p_bev = bufferevent_socket_new(p_evbase, fd, BEV_OPT_CLOSE_ON_FREE);
	// 设置bufferevent回调函数
	bufferevent_setcb(p_bev, cb_client_read, NULL, cb_client_close, (void *)p_client_addr_copy);
	// 设置读超时时间
	struct timeval tv_read_timeout = {config.timeout_seconds, 0};
	bufferevent_set_timeouts(p_bev, &tv_read_timeout, NULL);
	// 设置读缓冲区大小
	bufferevent_setwatermark(p_bev, EV_READ, 0, 4096);
	// 启用读事件
	bufferevent_enable(p_bev, EV_READ);
	// 增加当前连接数。使用原子操作，避免多线程竞争
	atomic_fetch_add(&stats.current_connections, 1);
}
