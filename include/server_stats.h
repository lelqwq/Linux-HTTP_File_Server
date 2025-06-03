#ifndef SERVER_STATS_H
#define SERVER_STATS_H

#include <stdatomic.h>
#include <stdio.h>

struct server_stats
{
    atomic_int current_connections; // 当前连接数
    atomic_long total_requests;     // 总请求数
    atomic_long total_bytes_sent;   // 总发送字节数
    atomic_long total_errors;       // 总错误数
};

extern struct server_stats stats;

void init_server_stats(void);
void print_server_stats(void);

#endif