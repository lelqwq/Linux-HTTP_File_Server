#ifndef HTTP_LINE_READER_H
#define HTTP_LINE_READER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/bufferevent.h>

/* 成功读到以 \r\n 结尾的一行返回 0，否则返回 -1 */
int bufferevent_read_crlf_line(struct bufferevent *bev, char *buffer, size_t buffer_size);

#endif
