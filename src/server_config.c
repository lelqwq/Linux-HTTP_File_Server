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
        }
    }
    fclose(file);
}