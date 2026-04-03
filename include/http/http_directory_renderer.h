#ifndef HTTP_DIRECTORY_RENDERER_H
#define HTTP_DIRECTORY_RENDERER_H

#include <event2/bufferevent.h>

void http_send_directory_listing(struct bufferevent *bev, const char *fs_path, const char *url_path, char *protocol);

#endif
