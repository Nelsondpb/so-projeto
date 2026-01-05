#ifndef CONFIG_H
#define CONFIG_H

/*
 * Tamanho máximo para caminhos de ficheiros.
 * Usado para ficheiros de jogos e ficheiros de log.
 */
#define MAX_PATH 512

/*
 * Tamanho máximo de uma linha lida dos ficheiros
 * de configuração.
 */
#define MAX_LINE 1024

/*
 * Modos de funcionamento do servidor.
 *
 * MODE_NORMAL : clientes jogam individualmente
 * MODE_DUEL   : clientes jogam em modo competitivo (duelo)
 */
#define MODE_NORMAL 0
#define MODE_DUEL   1

/*
 * Estrutura que guarda a configuração do servidor.
 *
 * Contém informação lida do ficheiro servidor.conf,
 * como ficheiros usados, porta e modo de funcionamento.
 */
typedef struct {
    char jogos_file[MAX_PATH];  /* ficheiro CSV com jogos e soluções */
    char log_file[MAX_PATH];    /* ficheiro de log do servidor */
    int port;                   /* porta TCP onde o servidor escuta */
    int max_clients;            /* número máximo de clientes */
    int mode;                   /* MODE_NORMAL ou MODE_DUEL */
} ServerConfig;

/*
 * Estrutura que guarda a configuração do cliente.
 *
 * Contém informação lida do ficheiro cliente.conf,
 * como IP do servidor, porta e identificação do cliente.
 */
typedef struct {
    char server_ip[128];        /* endereço IP do servidor */
    int server_port;            /* porta do servidor */
    int client_id;              /* identificador lógico do cliente */
    char log_file[MAX_PATH];    /* ficheiro de log do cliente */
} ClientConfig;

/*
 * Lê e carrega a configuração do servidor a partir de um ficheiro.
 *
 * Parâmetros:
 *  - path : caminho para o ficheiro de configuração
 *  - cfg  : estrutura ServerConfig a preencher
 *
 * Retorna:
 *  - 0  em caso de sucesso
 *  - -1 em caso de erro
 */
int load_server_config(const char *path, ServerConfig *cfg);

/*
 * Lê e carrega a configuração do cliente a partir de um ficheiro.
 *
 * Parâmetros:
 *  - path : caminho para o ficheiro de configuração
 *  - cfg  : estrutura ClientConfig a preencher
 *
 * Retorna:
 *  - 0  em caso de sucesso
 *  - -1 em caso de erro
 */
int load_client_config(const char *path, ClientConfig *cfg);

#endif
