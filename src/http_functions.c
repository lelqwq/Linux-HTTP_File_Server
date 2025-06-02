#include "http_functions.h"

void send_head(struct bufferevent *bev, char *protocol, int code, char *description, char *content_type, int length)
{
	struct evbuffer *output = bufferevent_get_output(bev);
	if (!output)
		return;

	evbuffer_add_printf(output, "%s %d %s\r\n", protocol, code, description);
	evbuffer_add_printf(output, "%s\r\n", content_type);
	evbuffer_add_printf(output, "Content-Length: %d\r\n", length);
	evbuffer_add(output, "\r\n", 2);
}

void send_file(struct bufferevent *bev, const char *path, char *protocol)
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
	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_add_file(output, fd, 0, st.st_size) == -1)
	{
		perror("evbuffer_add_file error");
		close(fd);
		send_error(bev, protocol, HTTP_INTERNAL_ERROR, "Internal Server Error");
		return;
	}
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
		if(strcmp(url_path, "/") == 0 && strcmp(special_entries[i], "..") == 0)
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
		snprintf(href, sizeof(href), "%s/%s", url_path, entry->d_name);
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

void http_request(char *method, const char *fs_path, const char *url_path, char *protocol, struct bufferevent *bev)
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
		// send HTTP head info
		char *file_type = get_file_type(fs_path);
		send_head(bev, protocol, HTTP_OK, "OK", file_type, st.st_size);
		// send file
		send_file(bev, fs_path, protocol);
	}
	else if (S_ISDIR(st.st_mode)) // 文件夹
	{
		// send HTTP head info
		send_head(bev, protocol, HTTP_OK, "OK", "Content-Type: text/html; charset=UTF-8", -1);
		// send dir data
		send_dir(bev, fs_path, url_path, protocol);
	}
	else
	{
		send_error(bev, protocol, HTTP_FORBIDDEN, "Forbidden");
	}
}

void send_error(struct bufferevent *bev, char *protocol, int code, char *description)
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