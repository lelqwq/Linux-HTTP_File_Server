#include "http/http_range.h"
#include <string.h>
#include <stdlib.h>

int http_parse_range_header(const char *range_header, off_t file_size, off_t *start, off_t *end, int *is_range)
{
	*start = 0;
	*end = file_size - 1;
	*is_range = 0;
	if (!range_header || strncmp(range_header, "bytes=", 6) != 0)
		return 0;

	*is_range = 1;
	char range_copy[128];
	strncpy(range_copy, range_header + 6, sizeof(range_copy) - 1);
	range_copy[sizeof(range_copy) - 1] = '\0';
	char *dash = strchr(range_copy, '-');
	if (!dash)
		return -1;
	*dash = '\0';
	if (strlen(range_copy) > 0)
		*start = (off_t)atoll(range_copy);
	if (strlen(dash + 1) > 0)
		*end = (off_t)atoll(dash + 1);
	if (*start > *end || *start >= file_size)
		return -1;
	if (*end >= file_size)
		*end = file_size - 1;
	return 0;
}
