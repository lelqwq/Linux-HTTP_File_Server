#include "server_config.h"

struct server_config config;

void load_config(const char *config_file)
{
    FILE *file = fopen(config_file, "r");
    if (!file)
    {
        perror("Failed to open config file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        char key[128], value[128];
        if (sscanf(line, "%127[^=]=%127s", key, value) == 2)
        {
            if (strcmp(key, "root_dir") == 0)
            {
                strncpy(config.root_dir, value, sizeof(config.root_dir) - 1);
            }
            else if (strcmp(key, "port") == 0)
            {
                config.port = atoi(value);
            }
            else if (strcmp(key, "max_connections") == 0)
            {
                config.max_connections = atoi(value);
            }
            else if (strcmp(key, "timeout_seconds") == 0)
            {
                config.timeout_seconds = atoi(value);
            }
            else if (strcmp(key, "enable_directory_listing") == 0)
            {
                config.enable_directory_listing = atoi(value);
            }
            else if (strcmp(key, "enable_stats_logging") == 0)
            {
                config.enable_stats_logging = atoi(value);
            }
            else if (strcmp(key, "log_file") == 0)
            {
                strncpy(config.log_file, value, sizeof(config.log_file) - 1);
            }
            else if (strcmp(key, "stats_logging_interval") == 0)
            {
                config.stats_logging_interval = atoi(value);
            }
            else if (strcmp(key, "enable_connection_info") == 0)
            {
                config.enable_connection_info = atoi(value);
            }
            else
            {
                fprintf(stderr, "Unknown configuration key: %s\n", key);
            }
        }
    }
    fclose(file);
}