#ifndef __HTTP_ERROR_H__
#define __HTTP_ERROR_H__

enum http_status_code
{
    HTTP_OK = 200,
    HTTP_NOT_FOUND = 404,
    HTTP_FORBIDDEN = 403,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_INTERNAL_ERROR = 500
};

#endif