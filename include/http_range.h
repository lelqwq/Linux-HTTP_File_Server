#ifndef HTTP_RANGE_H
#define HTTP_RANGE_H

#include <sys/types.h>

/*
 * 解析 Range: bytes=…，写入起止下标（闭区间）。无 Range 头时视为整文件。
 * 返回 0 成功；-1 范围非法（调用方应响应 416）。
 */
int http_parse_range_header(const char *range_header, off_t file_size, off_t *start, off_t *end, int *is_range);

#endif
