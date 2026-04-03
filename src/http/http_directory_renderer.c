#include "http/http_directory_renderer.h"
#include "http/http_error.h"
#include "http/http_responder.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>

void http_send_directory_listing(struct bufferevent *bev, const char *fs_path, const char *url_path, char *protocol)
{
	DIR *dp = opendir(fs_path);
	if (!dp)
	{
		perror("opendir error");
		http_send_error_response(bev, protocol, HTTP_INTERNAL_ERROR, "Internal Server Error");
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
