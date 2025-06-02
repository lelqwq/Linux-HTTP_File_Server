#include "http_functions.h"

void send_head(struct bufferevent *bev, char *protocol, int code, char *description, char *content_type, int length)
{
	struct evbuffer *output = bufferevent_get_output(bev);
    if (!output) return;

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
		send_error(bev, protocol, 500, "Internal Server Error");
		return;
	}
	struct stat st;
	if (fstat(fd, &st) == -1)
	{
		perror("fstat error");
		close(fd);
		send_error(bev, protocol, 500, "Internal Server Error");
		return;
	}
	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_add_file(output, fd, 0, st.st_size) == -1)
	{
		perror("evbuffer_add_file error");
		close(fd);
		send_error(bev, protocol, 500, "Internal Server Error");
		return;
	}
}

void send_dir(struct bufferevent *bev, const char *fs_path, const char *url_path, char *protocol)
{
	DIR *dp = opendir(fs_path);
	if (dp == NULL)
	{
		perror("opendir error");
		send_error(bev, protocol, 500, "Internal Server Error");
		return;
	}

	char html_content[4096] = {0};
	size_t html_len = 0;
	// 表头
	int n = snprintf(html_content + html_len, sizeof(html_content) - html_len,
					 "<html><head><title>Index of %s</title></head>\n"
					 "<body bgcolor=\"#ffffff\">\n"
					 "<h1>Index of %s</h1><hr>\n"
					 "<table>\n"
					 "<tr><th>Type</th><th>Name</th><th>Size</th><th>Modified</th></tr>\n",
					 fs_path, fs_path);
	if (n < 0 || (size_t)n >= sizeof(html_content) - html_len)
	{
		closedir(dp);
		send_error(bev, protocol, 500, "Internal Server Error");
		return;
	}
	html_len += n;

	struct dirent *entry;
	while ((entry = readdir(dp)) != NULL)
	{
		// 处理文件类型
		struct stat file_stat;
		char full_path[1024];
		snprintf(full_path, sizeof(full_path), "%s/%s", fs_path, entry->d_name);
		if (stat(full_path, &file_stat) == -1)
		{
			perror("stat error");
			continue;
		}
		char type_flag = '?';
		if (S_ISDIR(file_stat.st_mode))
			type_flag = 'D';
		else if (S_ISREG(file_stat.st_mode))
			type_flag = '-';
		// 处理修改时间
		char modified_time[26];
		strncpy(modified_time, ctime(&file_stat.st_mtime), sizeof(modified_time) - 1);
		modified_time[sizeof(modified_time) - 1] = '\0';
		modified_time[strcspn(modified_time, "\n")] = 0;
		// 超链接处理
		char href_path[1024];
		snprintf(href_path, sizeof(href_path), "%s/%s", url_path, entry->d_name);
		char cleaned_href[1024] = {0};
		int j = 0;
		for (int i = 0; href_path[i] != '\0' && j < (int)sizeof(cleaned_href) - 1; i++)
		{
			if (href_path[i] == '/' && href_path[i + 1] == '/')
				continue;
			cleaned_href[j++] = href_path[i];
		}
		cleaned_href[j] = '\0';
		// 拼接一行，先计算剩余空间
		size_t remain = sizeof(html_content) - html_len;
		if (remain < 64) // 剩余空间太小，直接跳出
			break;
		n = snprintf(html_content + html_len, remain,
					 "<tr><td>%c</td><td><a href=\"%s\">%s</a></td><td>%lld</td><td>%s</td></tr>\n",
					 type_flag, cleaned_href, entry->d_name, (long long)file_stat.st_size, modified_time);
		if (n < 0 || (size_t)n >= remain) // 空间不足，提前结束
			break;
		html_len += n;
	}
	// 结尾
	size_t remain = sizeof(html_content) - html_len;
	n = snprintf(html_content + html_len, remain, "</table><hr></body></html>");
	if (n > 0 && (size_t)n < remain)
		html_len += n;
	else
		html_content[sizeof(html_content) - 1] = '\0';
	closedir(dp);
	if (bufferevent_write(bev, html_content, strlen(html_content)) == -1)
	{
		perror("bufferevent_write error in send_dir");
		send_error(bev, protocol, 500, "Internal Server Error");
	}
}

void http_request(char *method, const char *fs_path, const char *url_path, char *protocol, struct bufferevent *bev)
{
	if (strcasecmp(method, "GET") != 0)
	{
		send_error(bev, protocol, 405, "Method Not Allowed");
		return;
	}
	struct stat st;
	if (stat(fs_path, &st) == -1)
	{
		send_error(bev, protocol, 404, "Not Found");
		return;
	}
	if (S_ISREG(st.st_mode))
	{
		// send HTTP head info
		char *file_type = get_file_type(fs_path);
		send_head(bev, protocol, 200, "OK", file_type, st.st_size);
		// send file
		send_file(bev, fs_path, protocol);
	}
	else if (S_ISDIR(st.st_mode))
	{
		// send HTTP head info
		send_head(bev, protocol, 200, "OK", "Content-Type: text/html; charset=UTF-8", -1);
		// send dir data
		send_dir(bev, fs_path, url_path, protocol);
	}
	else
	{
		send_error(bev, protocol, 403, "Forbidden");
	}
}

void send_error(struct bufferevent *bev, char *protocol, int code, char *description)
{
	char respond[4096] = {0};
	char html_content[1024] = {0};

	sprintf(html_content,
			"<html><head><title>%d %s</title></head>\n"
			"<body bgcolor=\"#cc99cc\"><h4 align=\"center\">%d %s</h4>\n"
			"%s\n"
			"<hr>\n"
			"</body>\n"
			"</html>\n",
			code, description, code, description, "Error occurred");

	int content_length = strlen(html_content);

	sprintf(respond,
			"%s %d %s\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: %d\r\n"
			"Connection: close\r\n\r\n",
			protocol, code, description, content_length);

	strcat(respond, html_content);
	bufferevent_write(bev, respond, strlen(respond));
}