#ifndef HTTP_HEADERS_H
#define HTTP_HEADERS_H

#include <event2/bufferevent.h>

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

void parse_http_headers(struct bufferevent *bev, http_request_t *req);
const char *get_header_value(http_request_t *req, const char *key);

#endif
