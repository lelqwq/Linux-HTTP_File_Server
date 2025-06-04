#include "http_functions.h"

void send_head(struct bufferevent *bev, char *protocol, int code, char *description, char *content_type, int length, off_t start, off_t end, off_t total_size)
{
	struct evbuffer *output = bufferevent_get_output(bev);
	if (!output)
		return;
	evbuffer_add_printf(output, "%s %d %s\r\n", protocol, code, description);
	evbuffer_add_printf(output, "Content-Type: %s\r\n", content_type);
	// 对视频文件总是添加Accept-Ranges
	if (strncmp(content_type, "video/", 6) == 0)
	{
		evbuffer_add_printf(output, "Accept-Ranges: bytes\r\n");
		// 为视频文件添加 Content-Disposition: inline
		evbuffer_add_printf(output, "Content-Disposition: inline\r\n");
		// 添加缓存控制头，优化视频播放体验
		evbuffer_add_printf(output, "Cache-Control: public, max-age=3600\r\n");
	}
	// 如果是范围请求响应，添加Content-Range头
	if (code == 206)
	{
		evbuffer_add_printf(output, "Content-Range: bytes %lld-%lld/%lld\r\n", (long long)start, (long long)end, (long long)total_size);
	}
	// 如果 length 为负，不输出 Content-Length 头
    if (length >= 0)
    {
        evbuffer_add_printf(output, "Content-Length: %d\r\n", length);
    }
	evbuffer_add_printf(output, "Connection: close\r\n");
	evbuffer_add(output, "\r\n", 2);
}

void send_file_range(struct bufferevent *bev, const char *path, off_t offset, size_t length, const char *protocol)
{
	int fd = open(path, O_RDONLY);
	if (fd == -1)
	{
		perror("open error");
		send_error(bev, protocol, HTTP_INTERNAL_ERROR, "Internal Server Error");
		return;
	}
	struct stat st;
	if (fstat(fd, &st) == -1)
	{
		perror("fstat error");
		close(fd);
		send_error(bev, protocol, HTTP_INTERNAL_ERROR, "Internal Server Error");
		return;
	}
	// 检查范围合法性
	if (offset < 0 || offset >= st.st_size || offset + length > st.st_size)
	{
		fprintf(stderr, "Invalid range offset/length\n");
		close(fd);
		send_error(bev, protocol, 416, "Requested Range Not Satisfiable");
		return;
	}
	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_add_file(output, fd, offset, length) == -1)
	{
		perror("evbuffer_add_file error");
		close(fd);
		send_error(bev, protocol, HTTP_INTERNAL_ERROR, "Internal Server Error");
		return;
	}
	// fd 由 libevent 接管，会在发送完成后自动关闭，无需调用 close(fd)
	// 更新统计
	atomic_fetch_add(&stats.total_bytes_sent, length);
}

void send_dir(struct bufferevent *bev, const char *fs_path, const char *url_path, char *protocol)
{
	DIR *dp = opendir(fs_path);
	if (!dp)
	{
		perror("opendir error");
		send_error(bev, protocol, HTTP_INTERNAL_ERROR, "Internal Server Error");
		return;
	}
	struct evbuffer *out = bufferevent_get_output(bev);
	if (!out)
	{
		closedir(dp);
		return;
	}
	// HTML 开头
	evbuffer_add_printf(out,
						"<html><head><title>Index of %s</title></head>"
						"<body bgcolor=\"#ffffff\">"
						"<h1>Index of %s</h1><hr><table>"
						"<tr><th>Type</th><th>Name</th><th>Size</th><th>Modified</th></tr>",
						fs_path, fs_path);

	struct dirent *entry;
	char full_path[1024];
	struct stat st;
	char modified_time[64];
	char href[1024];
	char clean_href[1024];
	// 优先处理 "." 和 ".."
	const char *special_entries[] = {".", ".."};
	for (int i = 0; i < 2; ++i)
	{
		// 根目录不显示 ".."
		if (strcmp(url_path, "/") == 0 && strcmp(special_entries[i], "..") == 0)
			continue;
		// 生成完整主机路径
		snprintf(full_path, sizeof(full_path), "%s/%s", fs_path, special_entries[i]);
		if (stat(full_path, &st) == -1)
			continue;
		// 获取修改时间
		struct tm *mt = localtime(&st.st_mtime);
		strftime(modified_time, sizeof(modified_time), "%Y-%m-%d %H:%M:%S", mt);
		// 生成 href 链接
		snprintf(href, sizeof(href), "%s/%s", url_path, special_entries[i]);
		int j = 0;
		for (int k = 0; href[k] && j < sizeof(clean_href) - 1; ++k)
		{
			if (href[k] == '/' && href[k + 1] == '/')
				continue;
			clean_href[j++] = href[k];
		}
		clean_href[j] = '\0';
		evbuffer_add_printf(out,
							"<tr><td>D</td><td><a href=\"%s\">%s</a></td>"
							"<td>%lld</td><td>%s</td></tr>",
							clean_href, special_entries[i],
							(long long)st.st_size, modified_time);
	}
	// 处理其他目录项
	while ((entry = readdir(dp)) != NULL)
	{
		// 跳过当前目录和上级目录
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;
		// 跳过video_player.html
		if (strcmp(entry->d_name, "video_player.html") == 0)
			continue;
		// 生成完整主机路径，这里的 fs_path 应该是主机文件系统的路径
		/*
		待添加功能
		若路径长度过长要进行处理
		*/
		snprintf(full_path, sizeof(full_path), "%s/%s", fs_path, entry->d_name);
		if (stat(full_path, &st) == -1)
		{
			perror("stat error");
			continue;
		}
		// 判断文件类型
		char type_flag = S_ISDIR(st.st_mode) ? 'D' : S_ISREG(st.st_mode) ? '-'
																		 : '?';
		// 获取修改时间
		struct tm *mt = localtime(&st.st_mtime);
		strftime(modified_time, sizeof(modified_time), "%Y-%m-%d %H:%M:%S", mt);
		// 生成 href 链接，这里的 url_path 应该是相对于服务器根目录的路径
		// 对于后缀为 .mp4 的文件，使用视频模板链接，其它文件则使用常规链接
        const char *ext = strrchr(entry->d_name, '.');
        if (ext && strcasecmp(ext, ".mp4") == 0)
        {
            // 使用完整的视频路径作为参数
			char video_path[2048];
			if (url_path[0] == '/')
				snprintf(video_path, sizeof(video_path), "%s/%s", url_path + 1, entry->d_name);
			else
				snprintf(video_path, sizeof(video_path), "%s/%s", url_path, entry->d_name);
			
			// 生成视频播放页面链接
			snprintf(href, sizeof(href), "/video_player.html?video=%s", video_path);
        }
        else
        {
            snprintf(href, sizeof(href), "%s/%s", url_path, entry->d_name);
        }
		// 规范化路径（去除重复 /）
		int j = 0;
		for (int i = 0; href[i] && j < sizeof(clean_href) - 1; ++i)
		{
			if (href[i] == '/' && href[i + 1] == '/')
				continue;
			clean_href[j++] = href[i];
		}
		clean_href[j] = '\0';

		evbuffer_add_printf(out,
							"<tr><td>%c</td><td><a href=\"%s\">%s</a></td>"
							"<td>%lld</td><td>%s</td></tr>",
							type_flag, clean_href, entry->d_name,
							(long long)st.st_size, modified_time);
	}
	evbuffer_add(out, "</table><hr></body></html>", strlen("</table><hr></body></html>"));
	closedir(dp);
}

void http_request(char *method, const char *fs_path, const char *url_path, char *protocol, struct bufferevent *bev, http_request_t *req)
{
	if (strcasecmp(method, "GET") != 0)
	{
		send_error(bev, protocol, HTTP_METHOD_NOT_ALLOWED, "Method Not Allowed");
		return;
	}
	struct stat st;
	if (stat(fs_path, &st) == -1)
	{
		send_error(bev, protocol, HTTP_NOT_FOUND, "Not Found");
		return;
	}
	if (S_ISREG(st.st_mode)) // 文件
	{
		char *file_type = get_file_type(fs_path);
		const char *range_header = get_header_value(req, "Range");
		off_t start = 0;
		off_t end = st.st_size - 1;
		int is_range = 0;
		// 处理Range请求
		if (range_header && strncmp(range_header, "bytes=", 6) == 0)
		{
			is_range = 1;
			// 用临时变量拷贝字符串，避免破坏原始请求头
			char range_copy[128];
			strncpy(range_copy, range_header + 6, sizeof(range_copy) - 1);
			range_copy[sizeof(range_copy) - 1] = '\0';
			char *dash = strchr(range_copy, '-');
			if (dash)
			{
				*dash = '\0';
				if (strlen(range_copy) > 0)
					start = atoll(range_copy);
				if (strlen(dash + 1) > 0)
					end = atoll(dash + 1);
				// 验证范围有效性
				if (start > end || start >= st.st_size)
				{
					send_error(bev, protocol, 416, "Requested Range Not Satisfiable");
					return;
				}
				// 确保end不超过文件大小
				if (end >= st.st_size)
					end = st.st_size - 1;
			}
		}
		int content_length = end - start + 1;
		// 发送适当的响应头
		if (is_range)
			send_head(bev, protocol, 206, "Partial Content", file_type, content_length, start, end, st.st_size);
		else
			send_head(bev, protocol, HTTP_OK, "OK", file_type, st.st_size, 0, st.st_size - 1, st.st_size);
		// 发送文件内容
		send_file_range(bev, fs_path, start, content_length, protocol);
	}
	else if (S_ISDIR(st.st_mode)) // 文件夹
	{
		// send HTTP head info
		send_head(bev, protocol, HTTP_OK, "OK", "text/html; charset=UTF-8", -1, 0, 0, 0);
		// send dir data
		send_dir(bev, fs_path, url_path, protocol);
	}
	else
	{
		send_error(bev, protocol, HTTP_FORBIDDEN, "Forbidden");
	}
}

void send_error(struct bufferevent *bev, const char *protocol, int code, char *description)
{
	if (!bev)
		return;
	struct evbuffer *output = bufferevent_get_output(bev);
	if (!output)
		return;

	// 生成 HTML 错误页面内容
	char html[1024];
	int html_len = snprintf(html, sizeof(html),
							"<html><head><title>%d %s</title></head>"
							"<body bgcolor=\"#cc99cc\"><h4 align=\"center\">%d %s</h4>"
							"Error occurred<hr></body></html>",
							code, description, code, description);
	if (html_len < 0 || html_len >= (int)sizeof(html))
		return;

	// 写 HTTP 响应头
	evbuffer_add_printf(output,
						"%s %d %s\r\n"
						"Content-Type: text/html; charset=UTF-8\r\n"
						"Content-Length: %d\r\n"
						"Connection: close\r\n\r\n",
						protocol, code, description, html_len);

	// 写 HTML 内容
	evbuffer_add(output, html, html_len);
}

void parse_http_headers(struct bufferevent *bev, http_request_t *req)
{
	req->header_count = 0;
	while (1)
	{
		char temp[1024] = {0};
		int ret = read_http_line(bev, temp, sizeof(temp));
		if (ret == -1 || strcmp(temp, "\r\n") == 0 || strlen(temp) == 0)
			break;
		// 解析 key:value（防止超过最大数量）
		if (req->header_count < MAX_HEADERS)
		{
			char *colon = strchr(temp, ':');
			if (colon)
			{
				int name_len = colon - temp;
				if (name_len < MAX_HEADER_NAME_LEN)
				{
					strncpy(req->headers[req->header_count].name, temp, name_len);
					req->headers[req->header_count].name[name_len] = '\0';
					// 跳过冒号和空格
					char *value = colon + 1;
					while (*value == ' ')
						value++;
					strncpy(req->headers[req->header_count].value, value, MAX_HEADER_VALUE_LEN - 1);
					req->headers[req->header_count].value[MAX_HEADER_VALUE_LEN - 1] = '\0';
					// 去除末尾的 \r\n
					char *end = strchr(req->headers[req->header_count].value, '\r');
					if (end)
						*end = '\0';
					req->header_count++;
				}
			}
		}
	}
}

const char *get_header_value(http_request_t *req, const char *key)
{
	for (int i = 0; i < req->header_count; i++)
	{
		if (strcasecmp(req->headers[i].name, key) == 0)
			return req->headers[i].value;
	}
	return NULL;
}
