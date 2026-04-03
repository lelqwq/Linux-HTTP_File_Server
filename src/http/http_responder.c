#include "http/http_responder.h"
#include "http/http_error.h"
#include "stats/server_stats.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void http_send_response_headers(struct bufferevent *bev, char *protocol, int code, char *description, char *content_type, int length, off_t start, off_t end, off_t total_size)
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

void http_send_range_file(struct bufferevent *bev, const char *path, off_t offset, size_t length, const char *protocol)
{
	int fd = open(path, O_RDONLY);
	if (fd == -1)
	{
		perror("open error");
		http_send_error_response(bev, protocol, HTTP_INTERNAL_ERROR, "Internal Server Error");
		return;
	}
	struct stat st;
	if (fstat(fd, &st) == -1)
	{
		perror("fstat error");
		close(fd);
		http_send_error_response(bev, protocol, HTTP_INTERNAL_ERROR, "Internal Server Error");
		return;
	}
	if (offset < 0 || offset >= st.st_size || offset + length > st.st_size)
	{
		fprintf(stderr, "Invalid range offset/length\n");
		close(fd);
		http_send_error_response(bev, protocol, 416, "Requested Range Not Satisfiable");
		return;
	}
	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_add_file(output, fd, offset, length) == -1)
	{
		perror("evbuffer_add_file error");
		close(fd);
		http_send_error_response(bev, protocol, HTTP_INTERNAL_ERROR, "Internal Server Error");
		return;
	}
	atomic_fetch_add(&stats.total_bytes_sent, length);
}

void http_send_error_response(struct bufferevent *bev, const char *protocol, int code, char *description)
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
