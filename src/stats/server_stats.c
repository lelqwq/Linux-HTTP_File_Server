#include "server_stats.h"
#include "server_config.h"
#include <time.h>
#include <stdio.h>

struct server_stats stats;

// 初始化性能统计
void init_server_stats(void)
{
    atomic_init(&stats.current_connections, 0);
    atomic_init(&stats.total_requests, 0);
    atomic_init(&stats.total_bytes_sent, 0);
    atomic_init(&stats.total_errors, 0);
}

// 打印服务器状态
void print_server_stats(void)
{
    // 如果未启用统计日志，直接返回
    if (!config.enable_stats_logging) 
        return;
    // 打开日志文件
    FILE *log_file = fopen(config.log_file, "a");//追加模式
    if (!log_file) 
    {
        perror("Failed to open stats log file");
        return;
    }
    // 获取当前时间
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    // 写入统计信息到日志文件
    fprintf(log_file, "========================================\n");
    fprintf(log_file, "统计时间: %s\n", time_str);
    fprintf(log_file, "----------------------------------------\n");
    fprintf(log_file, "当前连接数: %d\n", atomic_load(&stats.current_connections));
    fprintf(log_file, "总请求数: %ld\n", atomic_load(&stats.total_requests));
    fprintf(log_file, "总发送字节数: %ld\n", atomic_load(&stats.total_bytes_sent));
    fprintf(log_file, "总错误数: %ld\n", atomic_load(&stats.total_errors));
    fprintf(log_file, "========================================\n");
    // 关闭日志文件
    fclose(log_file);
}