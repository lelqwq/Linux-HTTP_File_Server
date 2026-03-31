#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include <event2/buffer.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/bufferevent.h>
#include <dirent.h>
#include "file_mime.h"
#include "http_error.h"
#include "http_headers.h"
#include "server_stats.h"

void http_request(char *method, const char *fs_path, const char *url_path, char *protocol, struct bufferevent *bev, http_request_t *req);
void send_head(struct bufferevent *bev, char *protocol, int code, char *description, char *content_type, int length, off_t start, off_t end, off_t total_size);
void send_file_range(struct bufferevent *bev, const char *path, off_t offset, size_t length, const char *protocol);
void send_dir(struct bufferevent *bev, const char *fs_path, const char *url_path, char *protocol);
void send_error(struct bufferevent *bev, const char *protocal, int code, char *discription);

#endif
