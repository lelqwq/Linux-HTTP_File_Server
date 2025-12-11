#include "read_http_line.h"

// 读取HTTP数据
int read_http_line(struct bufferevent *bev, char *buffer, size_t bufferSize)
{
	size_t buffer_pos = 0;
	// 每次调用此回调时尝试读取更多数据
	while (buffer_pos < bufferSize - 2) // 留出至少两个字符的空间给\r\n
	{
		char ch;
		if (bufferevent_read(bev, &ch, 1) <= 0) // 没有更多的数据可以读取了
			break;
		if (ch == '\r')
		{
			// 尝试读取下一个字符看是否为 \n
			if (bufferevent_read(bev, &ch, 1) > 0 && ch == '\n')
			{
				// 找到了 \r\n 结束符
				buffer[buffer_pos] = '\0'; // 终止字符串
				return 0;
			}
			else
			{
				// 如果不是 \n，则将之前的 \r 和当前字符加入缓冲区
				buffer[buffer_pos++] = '\r';
				buffer[buffer_pos++] = ch;
			}
		}
		else
		{
			buffer[buffer_pos++] = ch;
		}
	}
	// 没有找到 \r\n
	return -1;
}