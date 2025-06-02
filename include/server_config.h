#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct server_config
{
    char root_dir[256];           // 服务器根目录
    int port;                     // 监听端口
    int max_connections;          // 最大连接数
    int timeout_seconds;          // 超时时间
    int enable_directory_listing; // 是否启用目录列表功能
};

extern struct server_config config;

void load_config(const char *config_file);

#endif