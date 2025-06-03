#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

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
#include "get_file_type.h"
#include "http_error.h"
#include "server_stats.h"

void http_request(char *method, const char *fs_path, const char *url_path, char *protocol, struct bufferevent *bev);
void send_head(struct bufferevent *bev, char *protocal, int code, char *discription, char *content_type, int length);
void send_file(struct bufferevent *bev, const char *path, char *protocol);
void send_dir(struct bufferevent *bev, const char *fs_path, const char *url_path, char *protocol);
void send_error(struct bufferevent *bev, char *protocal, int code, char *discription);

#endif