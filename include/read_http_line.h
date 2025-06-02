#ifndef __READ_HTTP_H__
#define __READ_HTTP_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <event2/bufferevent.h>

/*
return 0 for successfully found http line
return -1 for not found http line
*/
int read_http_line(struct bufferevent *bev,char* buffer,size_t bufferSize);

#endif