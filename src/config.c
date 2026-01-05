#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/*
 * Remove espaços em branco no início e no fim de uma string.
 *
 * Esta função é usada para limpar valores lidos dos ficheiros
 * de configuração, garantindo que não existem espaços indesejados.
 *
 * Parâmetros:
 *  - s : string a ser limpa
 */
static void trim(char *s) {
    if (!s) return;
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len-1])) {
        s[len-1] = 0;
        len--;
    }
}

/*
 * Remove comentários inline de uma linha.
 *
 * Tudo o que aparece após o caractere '#' é ignorado.
 * Permite comentários no mesmo linha dos parâmetros.
 *
 * Parâmetros:
 *  - s : linha de texto a ser processada
 */
static void strip_inline_comment(char *s) {
    if (!s) return;
    char *hash = strchr(s, '#');
    if (hash) {
        *hash = '\0';
    }
}

/*
 * Faz o parsing de uma linha no formato "CHAVE: valor".
 *
 * Separa a chave e o valor, remove espaços extra e
 * ignora comentários inline.
 *
 * Parâmetros:
 *  - line  : linha original lida do ficheiro
 *  - key   : buffer onde será colocada a chave
 *  - value : buffer onde será colocado o valor
 *
 * Retorna:
 *  - 0  em caso de sucesso
 *  - -1 em caso de erro de parsing
 */
static int parse_key_value(const char *line, char *key, char *value) {
    const char *sep = strchr(line, ':');
    if (!sep) return -1;

    size_t klen = sep - line;
    if (klen >= 256) klen = 255;
    strncpy(key, line, klen);
    key[klen] = 0;

    strncpy(value, sep + 1, 512 - 1);
    key[255] = 0;
    value[511] = 0;

    trim(key);
    strip_inline_comment(value);
    trim(value);

    return 0;
}

/*
 * Carrega a configuração do servidor a partir de um ficheiro.
 *
 * Lê parâmetros como:
 *  - ficheiro de jogos
 *  - ficheiro de logs
 *  - porta de escuta
 *  - número máximo de clientes
 *  - modo de funcionamento (NORMAL ou DUEL)
 *
 * Parâmetros:
 *  - path : caminho para o ficheiro de configuração
 *  - cfg  : estrutura ServerConfig a preencher
 *
 * Retorna:
 *  - 0  em caso de sucesso
 *  - -1 em caso de erro ao abrir o ficheiro
 */
int load_server_config(const char *path, ServerConfig *cfg) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    /* valores por defeito */
    cfg->jogos_file[0] = 0;
    cfg->log_file[0] = 0;
    cfg->port = 0;
    cfg->max_clients = 0;
    cfg->mode = MODE_NORMAL;

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || strlen(line) < 2) continue;

        char key[256] = {0}, value[512] = {0};
        if (parse_key_value(line, key, value) != 0) continue;

        /* normalizar chave para maiúsculas */
        for (char *p = key; *p; ++p)
            *p = (char)toupper((unsigned char)*p);

        if (strcmp(key, "JOGOS_FILE") == 0) {
            strncpy(cfg->jogos_file, value, MAX_PATH - 1);
        } else if (strcmp(key, "LOG_FILE") == 0) {
            strncpy(cfg->log_file, value, MAX_PATH - 1);
        } else if (strcmp(key, "PORT") == 0) {
            cfg->port = atoi(value);
        } else if (strcmp(key, "MAX_CLIENTS") == 0) {
            cfg->max_clients = atoi(value);
        } else if (strcmp(key, "MODE") == 0) {
            char up[512];
            strncpy(up, value, sizeof(up) - 1);
            up[sizeof(up) - 1] = 0;
            for (char *p = up; *p; ++p)
                *p = (char)toupper((unsigned char)*p);

            if (strcmp(up, "DUEL") == 0)
                cfg->mode = MODE_DUEL;
            else
                cfg->mode = MODE_NORMAL;
        }
    }

    fclose(f);
    return 0;
}

/*
 * Carrega a configuração do cliente a partir de um ficheiro.
 *
 * Lê parâmetros como:
 *  - IP do servidor
 *  - porta do servidor
 *  - ID do cliente
 *  - ficheiro de logs
 *
 * Parâmetros:
 *  - path : caminho para o ficheiro de configuração
 *  - cfg  : estrutura ClientConfig a preencher
 *
 * Retorna:
 *  - 0  em caso de sucesso
 *  - -1 em caso de erro ao abrir o ficheiro
 */
int load_client_config(const char *path, ClientConfig *cfg) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    /* valores por defeito */
    cfg->server_ip[0] = 0;
    cfg->server_port = 0;
    cfg->client_id = 0;
    cfg->log_file[0] = 0;

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || strlen(line) < 2) continue;

        char key[256] = {0}, value[512] = {0};
        if (parse_key_value(line, key, value) != 0) continue;

        /* normalizar chave para maiúsculas */
        for (char *p = key; *p; ++p)
            *p = (char)toupper((unsigned char)*p);

        if (strcmp(key, "SERVER_IP") == 0) {
            strncpy(cfg->server_ip, value, sizeof(cfg->server_ip) - 1);
        } else if (strcmp(key, "SERVER_PORT") == 0) {
            cfg->server_port = atoi(value);
        } else if (strcmp(key, "CLIENT_ID") == 0) {
            cfg->client_id = atoi(value);
        } else if (strcmp(key, "LOG_FILE") == 0) {
            strncpy(cfg->log_file, value, MAX_PATH - 1);
        }
    }

    fclose(f);
    return 0;
}
