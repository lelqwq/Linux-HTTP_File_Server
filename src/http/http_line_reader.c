#include "http/http_line_reader.h"

/*
 * bufferevent_read_crlf_line — 从 bufferevent 读取一行HTTP请求
 *
 * 功能：从 bufferevent 中读取数据，直到遇到 \r\n 结尾。
 *
 * 参数：p_bev 对应该客户端连接的 bufferevent；buffer 缓冲区；buffer_size 缓冲区大小。
 *
 * 返回值：成功读到以 \r\n 结尾的一行返回 0，否则返回 -1。
 */
int bufferevent_read_crlf_line(struct bufferevent *p_bev, char *buffer, size_t buffer_size)
{
	size_t buffer_pos = 0;
	while (buffer_pos < buffer_size - 2)
	{
		char ch;

		// 读取一个字符
		int ret = bufferevent_read(p_bev, &ch, 1);
		if (ret <= 0)
		{
			break;
		}

		// 判断是否是HTTP行结束符。HTTP行结束符是\r\n，所以读取到\r后，需要再读取到\n，才说明是行结束符
		if (ch == '\r')
		{
			// 继续读取一个字符，如果读取到\n，则说明是行结束符
			ret = bufferevent_read(p_bev, &ch, 1);
			if (ret > 0 && ch == '\n')
			{
				// 设置缓冲区结束符
				buffer[buffer_pos] = '\0';
				return 0;
			}
			else
			{
				// 不是行结束符，将\r和当前字符保存到缓冲区
				buffer[buffer_pos] = '\r';
				buffer_pos++;
				buffer[buffer_pos] = ch;
				buffer_pos++;
			}
		}
		else
		{
			buffer[buffer_pos] = ch;
			buffer_pos++;
		}
	}
	return -1;
}
