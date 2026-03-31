#include "http_handler.h"
#include "http_range.h"
#include <strings.h>

void send_head(struct bufferevent *bev, char *protocol, int code, char *description, char *content_type, int length, off_t start, off_t end, off_t total_size)
{
	struct evbuffer *output = bufferevent_get_output(bev);
	if (!output)
		return;
	evbuffer_add_printf(output, "%s %d %s\r\n", protocol, code, description);
	evbuffer_add_printf(output, "Content-Type: %s\r\n", content_type);
	if (strncmp(content_type, "video/", 6) == 0)
	{
		evbuffer_add_printf(output, "Accept-Ranges: bytes\r\n");
		evbuffer_add_printf(output, "Content-Disposition: inline\r\n");
		evbuffer_add_printf(output, "Cache-Control: public, max-age=3600\r\n");
	}
	if (code == 206)
	{
		evbuffer_add_printf(output, "Content-Range: bytes %lld-%lld/%lld\r\n", (long long)start, (long long)end, (long long)total_size);
	}
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
	const char *special_entries[] = {".", ".."};
	for (int i = 0; i < 2; ++i)
	{
		if (strcmp(url_path, "/") == 0 && strcmp(special_entries[i], "..") == 0)
			continue;
		snprintf(full_path, sizeof(full_path), "%s/%s", fs_path, special_entries[i]);
		if (stat(full_path, &st) == -1)
			continue;
		struct tm *mt = localtime(&st.st_mtime);
		strftime(modified_time, sizeof(modified_time), "%Y-%m-%d %H:%M:%S", mt);
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
	while ((entry = readdir(dp)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;
		if (strcmp(entry->d_name, "video_player.html") == 0)
			continue;
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
		char type_flag = S_ISDIR(st.st_mode) ? 'D' : S_ISREG(st.st_mode) ? '-'
																		 : '?';
		struct tm *mt = localtime(&st.st_mtime);
		strftime(modified_time, sizeof(modified_time), "%Y-%m-%d %H:%M:%S", mt);
		const char *ext = strrchr(entry->d_name, '.');
		if (ext && strcasecmp(ext, ".mp4") == 0)
		{
			char video_path[2048];
			if (url_path[0] == '/')
				snprintf(video_path, sizeof(video_path), "%s/%s", url_path + 1, entry->d_name);
			else
				snprintf(video_path, sizeof(video_path), "%s/%s", url_path, entry->d_name);

			snprintf(href, sizeof(href), "/video_player.html?video=%s", video_path);
		}
		else
		{
			snprintf(href, sizeof(href), "%s/%s", url_path, entry->d_name);
		}
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
	if (S_ISREG(st.st_mode))
	{
		char *file_type = file_mime_lookup(fs_path);
		const char *range_header = get_header_value(req, "Range");
		off_t start = 0;
		off_t end = st.st_size - 1;
		int is_range = 0;
		if (http_parse_range_header(range_header, st.st_size, &start, &end, &is_range) != 0)
		{
			send_error(bev, protocol, 416, "Requested Range Not Satisfiable");
			return;
		}
		int content_length = (int)(end - start + 1);
		if (is_range)
			send_head(bev, protocol, 206, "Partial Content", file_type, content_length, start, end, st.st_size);
		else
			send_head(bev, protocol, HTTP_OK, "OK", file_type, st.st_size, 0, st.st_size - 1, st.st_size);
		send_file_range(bev, fs_path, start, (size_t)content_length, protocol);
	}
	else if (S_ISDIR(st.st_mode))
	{
		send_head(bev, protocol, HTTP_OK, "OK", "text/html; charset=UTF-8", -1, 0, 0, 0);
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

	char html[1024];
	int html_len = snprintf(html, sizeof(html),
							"<html><head><title>%d %s</title></head>"
							"<body bgcolor=\"#cc99cc\"><h4 align=\"center\">%d %s</h4>"
							"Error occurred<hr></body></html>",
							code, description, code, description);
	if (html_len < 0 || html_len >= (int)sizeof(html))
		return;

	evbuffer_add_printf(output,
						"%s %d %s\r\n"
						"Content-Type: text/html; charset=UTF-8\r\n"
						"Content-Length: %d\r\n"
						"Connection: close\r\n\r\n",
						protocol, code, description, html_len);

	evbuffer_add(output, html, html_len);
}
