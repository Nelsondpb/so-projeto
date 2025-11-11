#ifndef CONFIG_H
#define CONFIG_H

#define MAX_PATH 512
#define MAX_LINE 1024

typedef struct {
    char jogos_file[MAX_PATH];
    char log_file[MAX_PATH];
    int port;
    int max_clients;
} ServerConfig;

typedef struct {
    char server_ip[128];
    int server_port;
    int client_id;
    char log_file[MAX_PATH];
} ClientConfig;

int load_server_config(const char *path, ServerConfig *cfg);
int load_client_config(const char *path, ClientConfig *cfg);

#endif
