#ifndef __URL_DECODE_H__
#define __URL_DECODE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <event2/bufferevent.h>
#include <unistd.h>
#include <limits.h>

void url_decode(char *dst, const char *src);

#endif