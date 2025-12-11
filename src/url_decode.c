#include "url_decode.h"

// URL解码函数
void url_decode(char *dst, const char *src)
{
	while (*src)
	{
		if (*src == '%' && isxdigit(*(src + 1)) && isxdigit(*(src + 2)))
		{
			char hex[3] = { *(src + 1), *(src + 2), '\0' };
			*dst++ = (char) strtol(hex, NULL, 16);
			src += 3;
		}
		else if (*src == '+')  // URL 中 + 表示空格
		{
			*dst++ = ' ';
			src++;
		}
		else
		{
			*dst++ = *src++;
		}
	}
	*dst = '\0';
}