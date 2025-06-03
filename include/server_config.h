#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct server_config
{
    char root_dir[256];             // 服务器根目录
    int port;                       // 监听端口
    int max_connections;            // 最大连接数
    int timeout_seconds;            // 超时时间
    int enable_directory_listing;   // 是否启用目录列表功能
    int enable_stats_logging;       // 是否启用统计日志
    char log_file[256];             // 日志文件路径
    int stats_logging_interval;     // 统计日志记录间隔（秒）
    int enable_connection_info;     // 是否输出连接信息到控制台
};

extern struct server_config config;

void load_config(const char *config_file);

#endif