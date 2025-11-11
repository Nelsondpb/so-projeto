#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "config.h"
#include "games.h"
#include "logs.h"
#include "verify.h"

#define GREEN  "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RED    "\033[1;31m"
#define RESET  "\033[0m"

static ServerConfig g_cfg;
static SudokuGame *g_games = NULL;
static int g_n_games = 0;

void cleanup_and_exit(int code) {
    if (g_games) free_games(g_games);
    log_event(g_cfg.log_file, 0, "SERVER_STOP", "Servidor encerrado");
    printf(GREEN "Servidor: encerrado.\n" RESET);
    exit(code);
}

void handle_sigint(int sig) {
    (void)sig;
    log_event(g_cfg.log_file, 0, "SIGINT", "Encerramento por SIGINT");
    cleanup_and_exit(0);
}

int main(int argc, char **argv) {
    const char *conf = "servidor.conf";
    if (argc > 1) conf = argv[1];

    signal(SIGINT, handle_sigint);

    if (load_server_config(conf, &g_cfg) != 0) {
        fprintf(stderr, RED "Erro: nao foi possivel ler config '%s'\n" RESET, conf);
        return 1;
    }

    printf(GREEN "========================================\n" RESET);
    printf(GREEN "  Servidor Sudoku - 1ª Fase            \n" RESET);
    printf(GREEN "========================================\n" RESET);
    printf(YELLOW "Config: jogos=%s log=%s port=%d max_clients=%d\n" RESET,
           g_cfg.jogos_file[0] ? g_cfg.jogos_file : "(nao definido)",
           g_cfg.log_file[0] ? g_cfg.log_file : "(nao definido)",
           g_cfg.port, g_cfg.max_clients);

    log_event(g_cfg.log_file, 0, "SERVER_START", "Servidor iniciado e config carregada");

    if (g_cfg.jogos_file[0] == 0) {
        log_event(g_cfg.log_file, 0, "ERROR", "Ficheiro de jogos nao especificado");
        fprintf(stderr, RED "Erro: JOGOS_FILE nao especificado no config\n" RESET);
        cleanup_and_exit(2);
    }

    if (load_games_csv(g_cfg.jogos_file, &g_games, &g_n_games) != 0) {
        log_event(g_cfg.log_file, 0, "LOAD_GAMES_ERROR", g_cfg.jogos_file);
        fprintf(stderr, RED "Erro: nao foi possivel carregar jogos '%s'\n" RESET, g_cfg.jogos_file);
        cleanup_and_exit(3);
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "Jogos carregados: %d", g_n_games);
    printf(GREEN "%s\n" RESET, buf);
    log_event(g_cfg.log_file, 0, "LOAD_GAMES", buf);

    if (g_n_games > 0) {
        int errs = verifica_solucao(g_games[0].solution, g_games[0].solution);
        if (errs == 0) {
            log_event(g_cfg.log_file, 0, "VERIFY", "Solucao jogo 1 correta (interno)");
            printf(GREEN "Verificacao interna: solucao correta\n" RESET);
        } else if (errs == -1) {
            log_event(g_cfg.log_file, 0, "VERIFY", "Solucao jogo 1 invalida (interno)");
            printf(RED "Verificacao interna: grelha invalida\n" RESET);
        } else {
            snprintf(buf, sizeof(buf), "Solucao jogo 1 errada em %d", errs);
            log_event(g_cfg.log_file, 0, "VERIFY", buf);
            printf(RED "%s\n" RESET, buf);
        }
    }

    printf(YELLOW "Pressiona Ctrl+C para encerrar o servidor...\n" RESET);
    while (1) {
        pause(); 
    }

    cleanup_and_exit(0);
    return 0;
}
