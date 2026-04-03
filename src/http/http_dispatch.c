#include "http/http_dispatch.h"
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http/http_line_reader.h"
#include "http/http_headers.h"
#include "http/http_router.h"
#include "http/http_responder.h"
#include "util/path_guard.h"
#include "util/url_decode.h"
#include "stats/server_stats.h"

/*
 * http_dispatch_on_read — bufferevent 读回调（有数据可读时由 libevent 调用）。
 *
 * 功能：读取请求行、解析方法、路径、协议；处理查询串；调用 path_guard_resolve_under_root 校验路径；
 *       解析请求头并交给 http_handle_request；累加请求数。
 *
 * 参数：p_bev 对应该客户端连接的 bufferevent。
 *
 * 返回值：无。
 */
void http_dispatch_on_read(struct bufferevent *p_bev)
{
	// 读取请求数据
	char http_line[4096];
	int ret = bufferevent_read_crlf_line(p_bev, http_line, sizeof(http_line));
	if (ret == -1)
	{
		printf("No HTTP request\n");
		return;
	}
	// 解析请求行，获取方法、路径、协议
	// todo: 当前解析不够安全，需要使用更安全的解析方法
	char method[16], request_target[256], protocol[16];
	sscanf(http_line, "%15s %255s %15s", method, request_target, protocol);

	// 定位查询参数分隔符指针
	char *p_query_delim = strchr(request_target, '?');
	if (p_query_delim != NULL)
	{
		*p_query_delim = '\0';
	}

	char fs_path[256];// 文件系统路径
	char video_fs_path[256] = {0};// 视频文件的文件系统路径
	char url_path[256];// URL路径

	// 根据请求路径类型处理文件系统路径和URL路径
	if (strcmp(request_target, "/") == 0)// 请求的是根路径
	{
		strcpy(fs_path, ".");// 根路径的文件系统路径是当前目录
		strcpy(url_path, "/");// 根路径的URL路径是 /
	}
	else if (strstr(request_target, "video_player.html") && p_query_delim)// 请求的是视频播放页面且有查询参数
	{
		strcpy(fs_path, "video/video_player.html");// 视频播放页面的文件系统路径是 video/video_player.html
		strcpy(url_path, request_target);// 视频播放页面的URL路径是请求路径
		// 获取查询参数
		p_query_delim++;
		if (strncmp(p_query_delim, "video=", 6) == 0)// 查询参数是 video=
		{
			url_decode(video_fs_path, p_query_delim + 6);// 解码视频路径
		}
	}
	else// 请求的是其他路径
	{
		// 请求路径需要进行URL解码，把 %20、中文编码等还原为正常字符
		// 将请求路径进行URL解码放到path中
		url_decode(fs_path, request_target + 1);// 加 1 是把 URL 路径映射为相对资源根目录的本地路径
		strcpy(url_path, request_target);// 将请求路径放到url_path中
	}

	// 获取服务器根目录绝对路径
	char server_root_abs_path[PATH_MAX];
	if (realpath(".", server_root_abs_path) == NULL)
	{
		http_send_error_response(p_bev, protocol, 403, "Invalid fs_path");
		return;
	}
	
	// 检测是否存在路径穿越，防止请求路径穿越到其他目录
	ret = path_guard_resolve_under_root(fs_path, server_root_abs_path);
	if (ret == -1)
	{
		http_send_error_response(p_bev, protocol, 403, "Invalid fs_path");
		return;
	}
	if (ret == -2)
	{
		http_send_error_response(p_bev, protocol, 403, "Forbidden");
		return;
	}

	// 处理视频播放页面
	if (video_fs_path[0] != '\0')
	{
		ret = path_guard_resolve_under_root(video_fs_path, server_root_abs_path);
		if (ret == -1)
		{
			http_send_error_response(p_bev, protocol, 404, "Video file not found");
			return;
		}
		if (ret == -2)
		{
			http_send_error_response(p_bev, protocol, 403, "Forbidden");
			return;
		}
	}

	http_request_t req;
	parse_http_headers(p_bev, &req);
	if (strcmp(method, "GET") == 0)
	{
		atomic_fetch_add(&stats.total_requests, 1);
		http_handle_request(method, fs_path, url_path, protocol, p_bev, &req);
	}
}
