#include "get_file_type.h"

char *get_file_type(const char *filename)
{
    char *dot = strrchr(filename, '.');
    if (dot && dot != filename) // 存在后缀且不在开头位置
    {
        if (strcmp(dot, ".txt") == 0)
            return "Content-Type: text/plain; charset=UTF-8";
        else if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
            return "Content-Type: text/html; charset=UTF-8";
        else if (strcmp(dot, ".css") == 0)
            return "Content-Type: text/css; charset=UTF-8";
        else if (strcmp(dot, ".js") == 0)
            return "Content-Type: application/javascript";
        else if (strcmp(dot, ".json") == 0)
            return "Content-Type: application/json";
        else if (strcmp(dot, ".xml") == 0)
            return "Content-Type: application/xml";
        else if (strcmp(dot, ".pdf") == 0)
            return "Content-Type: application/pdf";
        else if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
            return "Content-Type: image/jpeg";
        else if (strcmp(dot, ".png") == 0)
            return "Content-Type: image/png";
        else if (strcmp(dot, ".gif") == 0)
            return "Content-Type: image/gif";
        else if (strcmp(dot, ".svg") == 0)
            return "Content-Type: image/svg+xml";
        else if (strcmp(dot, ".mp3") == 0)
            return "Content-Type: audio/mpeg";
        else if (strcmp(dot, ".wav") == 0)
            return "Content-Type: audio/wav";
        else if (strcmp(dot, ".mp4") == 0)
            return "Content-Type: video/mp4";
        else if (strcmp(dot, ".mov") == 0)
            return "Content-Type: video/quicktime";
        else if (strcmp(dot, ".zip") == 0)
            return "Content-Type: application/zip";
        else if (strcmp(dot, ".tar.gz") == 0)
            return "Content-Type: application/x-tar";
        else //没有匹配的后缀
            return "Content-Type: application/octet-stream";
    }
    else // 没有有效后缀
    {
        return "Content-Type: application/octet-stream";
    }
}