#include "path_guard.h"
#include <stdlib.h>
#include <string.h>

int path_guard_resolve_under_root(const char *logical_path, const char *root_real, char *real_out, size_t out_len)
{
	(void)out_len;
	if (realpath(logical_path, real_out) == NULL)
		return -1;
	size_t root_len = strlen(root_real);
	if (strncmp(real_out, root_real, root_len) != 0 ||
		(real_out[root_len] != '/' && real_out[root_len] != '\0'))
		return -2;
	return 0;
}
