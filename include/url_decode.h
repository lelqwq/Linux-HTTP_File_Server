#ifndef URL_DECODE_H
#define URL_DECODE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <event2/bufferevent.h>
#include <unistd.h>
#include <limits.h>

void url_decode(char *dst, const char *src);

#endif