#ifndef HTTP_DISPATCH_H
#define HTTP_DISPATCH_H

struct bufferevent;

/* 客户端可读：读请求行、解析路径与根目录校验、读请求头并交给 http_handle_request */
void http_dispatch_on_read(struct bufferevent *bev);

#endif
