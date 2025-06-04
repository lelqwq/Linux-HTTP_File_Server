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
#include "read_http_line.h"

#define MAX_HEADERS 50
#define MAX_HEADER_NAME_LEN 64
#define MAX_HEADER_VALUE_LEN 512

typedef struct
{
    char name[MAX_HEADER_NAME_LEN];
    char value[MAX_HEADER_VALUE_LEN];
} http_header_t;

typedef struct
{
    http_header_t headers[MAX_HEADERS];
    int header_count;
} http_request_t;

void http_request(char *method, const char *fs_path, const char *url_path, char *protocol, struct bufferevent *bev, http_request_t *req);
void send_head(struct bufferevent *bev, char *protocol, int code, char *description, char *content_type, int length, off_t start, off_t end, off_t total_size);
void send_file(struct bufferevent *bev, const char *path, off_t offset, char *protocol);
void send_file_range(struct bufferevent *bev, const char *path, off_t offset, size_t length, const char *protocol);
void send_dir(struct bufferevent *bev, const char *fs_path, const char *url_path, char *protocol);
void send_error(struct bufferevent *bev, const char *protocal, int code, char *discription);
void parse_http_headers(struct bufferevent *bev, http_request_t *req);
const char *get_header_value(http_request_t *req, const char *key);

#endif