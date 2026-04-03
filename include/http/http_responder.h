#ifndef HTTP_RESPONDER_H
#define HTTP_RESPONDER_H

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <sys/types.h>
#include <stddef.h>

void http_send_response_headers(struct bufferevent *bev, char *protocol, int code, char *description, char *content_type, int length, off_t start, off_t end, off_t total_size);
void http_send_range_file(struct bufferevent *bev, const char *path, off_t offset, size_t length, const char *protocol);
void http_send_error_response(struct bufferevent *bev, const char *protocol, int code, char *description);

#endif
