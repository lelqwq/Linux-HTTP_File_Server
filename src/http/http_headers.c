#include "http_headers.h"
#include "http_line_reader.h"
#include <string.h>
#include <strings.h>

void parse_http_headers(struct bufferevent *bev, http_request_t *req)
{
	req->header_count = 0;
	while (1)
	{
		char temp[1024] = {0};
		int ret = http_read_line(bev, temp, sizeof(temp));
		if (ret == -1 || strcmp(temp, "\r\n") == 0 || strlen(temp) == 0)
			break;
		if (req->header_count < MAX_HEADERS)
		{
			char *colon = strchr(temp, ':');
			if (colon)
			{
				int name_len = colon - temp;
				if (name_len < MAX_HEADER_NAME_LEN)
				{
					strncpy(req->headers[req->header_count].name, temp, name_len);
					req->headers[req->header_count].name[name_len] = '\0';
					char *value = colon + 1;
					while (*value == ' ')
						value++;
					strncpy(req->headers[req->header_count].value, value, MAX_HEADER_VALUE_LEN - 1);
					req->headers[req->header_count].value[MAX_HEADER_VALUE_LEN - 1] = '\0';
					char *end = strchr(req->headers[req->header_count].value, '\r');
					if (end)
						*end = '\0';
					req->header_count++;
				}
			}
		}
	}
}

const char *get_header_value(http_request_t *req, const char *key)
{
	for (int i = 0; i < req->header_count; i++)
	{
		if (strcasecmp(req->headers[i].name, key) == 0)
			return req->headers[i].value;
	}
	return NULL;
}
