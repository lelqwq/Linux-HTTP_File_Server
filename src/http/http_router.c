#include "http/http_router.h"
#include "http/http_directory_renderer.h"
#include "http/http_error.h"
#include "http/http_range.h"
#include "http/http_responder.h"
#include "util/file_mime.h"
#include <strings.h>
#include <sys/stat.h>

typedef void (*http_method_handler_t)(const char *fs_path, const char *url_path, char *protocol, struct bufferevent *bev, http_request_t *req);

static void handle_get_request(const char *fs_path, const char *url_path, char *protocol, struct bufferevent *bev, http_request_t *req)
{
	struct stat st;
	if (stat(fs_path, &st) == -1)
	{
		http_send_error_response(bev, protocol, HTTP_NOT_FOUND, "Not Found");
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
			http_send_error_response(bev, protocol, 416, "Requested Range Not Satisfiable");
			return;
		}
		int content_length = (int)(end - start + 1);
		if (is_range)
			http_send_response_headers(bev, protocol, 206, "Partial Content", file_type, content_length, start, end, st.st_size);
		else
			http_send_response_headers(bev, protocol, HTTP_OK, "OK", file_type, st.st_size, 0, st.st_size - 1, st.st_size);
		http_send_range_file(bev, fs_path, start, (size_t)content_length, protocol);
	}
	else if (S_ISDIR(st.st_mode))
	{
		http_send_response_headers(bev, protocol, HTTP_OK, "OK", "text/html; charset=UTF-8", -1, 0, 0, 0);
		http_send_directory_listing(bev, fs_path, url_path, protocol);
	}
	else
	{
		http_send_error_response(bev, protocol, HTTP_FORBIDDEN, "Forbidden");
	}
}

void http_handle_request(char *method, const char *fs_path, const char *url_path, char *protocol, struct bufferevent *bev, http_request_t *req)
{
	static const struct
	{
		const char *method;
		http_method_handler_t handler;
	} method_routes[] = {
		{"GET", handle_get_request},
	};

	size_t route_count = sizeof(method_routes) / sizeof(method_routes[0]);
	for (size_t i = 0; i < route_count; ++i)
	{
		if (strcasecmp(method, method_routes[i].method) == 0)
		{
			method_routes[i].handler(fs_path, url_path, protocol, bev, req);
			return;
		}
	}
	http_send_error_response(bev, protocol, HTTP_METHOD_NOT_ALLOWED, "Method Not Allowed");
}
