#include "http_line_reader.h"

int http_read_line(struct bufferevent *bev, char *buffer, size_t buffer_size)
{
	size_t buffer_pos = 0;
	while (buffer_pos < buffer_size - 2)
	{
		char ch;
		if (bufferevent_read(bev, &ch, 1) <= 0)
			break;
		if (ch == '\r')
		{
			if (bufferevent_read(bev, &ch, 1) > 0 && ch == '\n')
			{
				buffer[buffer_pos] = '\0';
				return 0;
			}
			else
			{
				buffer[buffer_pos++] = '\r';
				buffer[buffer_pos++] = ch;
			}
		}
		else
		{
			buffer[buffer_pos++] = ch;
		}
	}
	return -1;
}
