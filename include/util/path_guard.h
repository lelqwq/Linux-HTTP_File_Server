#ifndef PATH_GUARD_H
#define PATH_GUARD_H

#include <stddef.h>

/*
 * 将 fs_path 经 realpath 解析后，校验是否落在 server_root_abs_path 目录下（前缀 + 边界）。
 * 返回 0 成功；-1 realpath 解析失败；-2 路径越出根目录。
 */
int path_guard_resolve_under_root(const char *fs_path, const char *server_root_abs_path);

#endif
