#include "http_dispatch.h"
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http_line_reader.h"
#include "http_headers.h"
#include "http_handler.h"
#include "path_guard.h"
#include "url_decode.h"
#include "server_stats.h"

void http_dispatch_on_read(struct bufferevent *bev)
{
	char http_line[1024];
	int ret = http_read_line(bev, http_line, sizeof(http_line));
	if (ret == -1)
	{
		printf("no http\n");
		return;
	}
	char method[16], temp_path[256], protocol[16];
	sscanf(http_line, "%15s %255s %15s", method, temp_path, protocol);

	char *query = strchr(temp_path, '?');
	if (query)
		*query = '\0';

	char path[256];
	char url_path[256];
	char video_path[256] = {0};
	if (strstr(temp_path, "video_player.html") && query)
	{
		strcpy(path, "video/video_player.html");
		strcpy(url_path, temp_path);
		query++;
		if (strncmp(query, "video=", 6) == 0)
			url_decode(video_path, query + 6);
	}
	else if (strcmp(temp_path, "/") == 0)
	{
		strcpy(path, ".");
		strcpy(url_path, "/");
	}
	else
	{
		char decoded_path[256];
		url_decode(decoded_path, temp_path + 1);
		strcpy(path, decoded_path);
		strcpy(url_path, temp_path);
	}

	char server_root[] = "./";
	char server_real[PATH_MAX];
	if (realpath(server_root, server_real) == NULL)
	{
		send_error(bev, protocol, 403, "Invalid path");
		return;
	}

	char real_path[PATH_MAX];
	ret = path_guard_resolve_under_root(path, server_real, real_path, sizeof(real_path));
	if (ret == -1)
	{
		send_error(bev, protocol, 403, "Invalid path");
		return;
	}
	if (ret == -2)
	{
		send_error(bev, protocol, 403, "Forbidden");
		return;
	}

	if (video_path[0] != '\0')
	{
		char video_real_path[PATH_MAX];
		ret = path_guard_resolve_under_root(video_path, server_real, video_real_path, sizeof(video_real_path));
		if (ret == -1)
		{
			send_error(bev, protocol, 404, "Video file not found");
			return;
		}
		if (ret == -2)
		{
			send_error(bev, protocol, 403, "Forbidden video path");
			return;
		}
	}

	http_request_t req;
	parse_http_headers(bev, &req);
	if (strcmp(method, "GET") == 0)
	{
		atomic_fetch_add(&stats.total_requests, 1);
		http_request(method, path, url_path, protocol, bev, &req);
	}
}
