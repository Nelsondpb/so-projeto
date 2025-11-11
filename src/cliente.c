#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "config.h"
#include "games.h"
#include "logs.h"
#include "verify.h"

#define BLUE   "\033[1;34m"
#define GREEN  "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RED    "\033[1;31m"
#define RESET  "\033[0m"

static ClientConfig g_cfg;
static SudokuGame *g_games = NULL;
static int g_n_games = 0;

void cleanup_and_exit(int code) {
    if (g_games) free_games(g_games);
    log_event(g_cfg.log_file, g_cfg.client_id, "CLIENT_STOP", "Cliente encerrado");
    printf(GREEN "Cliente: encerrado.\n" RESET);
    exit(code);
}

void handle_sigint(int sig) {
    (void)sig;
    log_event(g_cfg.log_file, g_cfg.client_id, "SIGINT", "Encerramento por SIGINT");
    cleanup_and_exit(0);
}

int main(int argc, char **argv) {
    const char *conf = "cliente.conf";
    if (argc > 1) conf = argv[1];

    signal(SIGINT, handle_sigint);

    if (load_client_config(conf, &g_cfg) != 0) {
        fprintf(stderr, RED "Erro: nao foi possivel ler config '%s'\n" RESET, conf);
        return 1;
    }

    printf(BLUE "========================================\n" RESET);
    printf(BLUE "  Cliente Sudoku - 1ª Fase             \n" RESET);
    printf(BLUE "========================================\n" RESET);
    printf(YELLOW "Config: server=%s:%d id=%d log=%s\n" RESET,
           g_cfg.server_ip[0] ? g_cfg.server_ip : "(nao definido)",
           g_cfg.server_port, g_cfg.client_id,
           g_cfg.log_file[0] ? g_cfg.log_file : "(nao definido)");

    log_event(g_cfg.log_file, g_cfg.client_id, "CLIENT_START", "Cliente iniciado e config carregada");

    if (load_games_csv("jogos.csv", &g_games, &g_n_games) != 0) {
        log_event(g_cfg.log_file, g_cfg.client_id, "LOAD_GAMES_ERROR", "jogos.csv");
        fprintf(stderr, RED "Erro: nao foi possivel carregar 'jogos.csv'\n" RESET);
        cleanup_and_exit(2);
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "Client: jogos carregados: %d", g_n_games);
    printf(GREEN "%s\n" RESET, buf);
    log_event(g_cfg.log_file, g_cfg.client_id, "LOAD_GAMES", buf);

    if (g_n_games > 0) {
        int errs = verifica_solucao(g_games[0].solution, g_games[0].solution);
        if (errs == 0) {
            log_event(g_cfg.log_file, g_cfg.client_id, "SEND_SOLUTION", "solucao correta (simulada)");
            printf(GREEN "Envio simulado: solucao correta\n" RESET);
        } else if (errs == -1) {
            log_event(g_cfg.log_file, g_cfg.client_id, "SEND_SOLUTION", "solucao invalida (simulada)");
            printf(RED "Envio simulado: grelha invalida\n" RESET);
        } else {
            snprintf(buf, sizeof(buf), "Envio simulado: solucao errada em %d", errs);
            log_event(g_cfg.log_file, g_cfg.client_id, "SEND_SOLUTION", buf);
            printf(RED "%s\n" RESET, buf);
        }
    }

    printf(YELLOW "Pressiona Ctrl+C para encerrar o cliente...\n" RESET);
    while (1) {
        pause();
    }

    cleanup_and_exit(0);
    return 0;
}
