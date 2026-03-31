#ifndef PATH_GUARD_H
#define PATH_GUARD_H

#include <stddef.h>

/*
 * 将 logical_path 经 realpath 解析后，校验是否落在 root_real 目录下（前缀 + 边界）。
 * real_out 需至少 PATH_MAX 字节。
 * 返回 0 成功；-1 realpath 失败；-2 路径越出根目录。
 */
int path_guard_resolve_under_root(const char *logical_path, const char *root_real, char *real_out, size_t out_len);

#endif
