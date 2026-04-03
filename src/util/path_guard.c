#include "util/path_guard.h"
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>

/*
 * 将 fs_path 经 realpath 解析后，校验是否落在 server_root_abs_path 目录下（前缀 + 边界）。
 * 返回 0 成功；-1 realpath 解析失败；-2 路径越出根目录。
 */
int path_guard_resolve_under_root(const char *fs_path, const char *server_root_abs_path)
{
	char real_path[PATH_MAX];
	if (realpath(fs_path, real_path) == NULL)
	{
		return -1;
	}
	size_t root_len = strlen(server_root_abs_path);
	if (strncmp(real_path, server_root_abs_path, root_len) != 0 || (real_path[root_len] != '/' && real_path[root_len] != '\0'))
	{
		return -2;
	}
	return 0;
}
