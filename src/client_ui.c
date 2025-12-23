#include "client_ui.h"
#include <stdio.h>

/*
 * Códigos ANSI simples para cores.
 * Se quiseres, podes remover as cores e deixar texto simples.
 */
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_GREEN   "\033[1;32m"
#define C_YELLOW  "\033[1;33m"
#define C_CYAN    "\033[1;36m"
#define C_BLUE    "\033[1;34m"
#define C_RED     "\033[1;31m"
#define C_GRAY    "\033[1;90m"

void ui_print_banner(int client_id, int game_id) {
    printf("\n%s🧩 Cliente Sudoku%s\n", C_BOLD, C_RESET);
    printf("%s──────────────────────────────────────────────%s\n", C_CYAN, C_RESET);
    printf("  Cliente ID:   %s%d%s\n", C_GREEN, client_id, C_RESET);
    if (game_id >= 0) {
        printf("  Jogo ID:      %s%d%s\n", C_YELLOW, game_id, C_RESET);
    } else {
        printf("  Jogo ID:      (a aguardar atribuição)\n");
    }
    printf("%s──────────────────────────────────────────────%s\n\n", C_CYAN, C_RESET);
}

/*
 * Imprime o tabuleiro 9x9 com separadores de blocos 3x3.
 * As células iniciais (fixas) podem ser destacadas se "initial" != NULL.
 */
void ui_print_grid(const int grid[9][9], const int initial[9][9]) {
    printf("%sTabuleiro atual:%s\n", C_BOLD, C_RESET);
    printf("    ");
    for (int c = 0; c < 9; c++) {
        printf(" %d ", c + 1);
        if ((c + 1) % 3 == 0 && c < 8) printf(" ");
    }
    printf("\n");

    printf("   %s──────────────────────────────%s\n", C_BLUE, C_RESET);

    for (int r = 0; r < 9; r++) {
        printf(" %d %s│%s", r + 1, C_BLUE, C_RESET);
        for (int c = 0; c < 9; c++) {
            int v = grid[r][c];
            int fixed = (initial != NULL && initial[r][c] != 0);

            if (v == 0) {
                /* célula vazia */
                printf(" . ");
            } else if (fixed) {
                /* célula fixa do tabuleiro inicial */
                printf(" %s%d%s ", C_GRAY, v, C_RESET);
            } else {
                /* célula preenchida pelo cliente */
                printf(" %s%d%s ", C_GREEN, v, C_RESET);
            }

            if ((c + 1) % 3 == 0 && c < 8) {
                printf("%s│%s", C_BLUE, C_RESET);
            }
        }
        printf("%s│%s\n", C_BLUE, C_RESET);

        if ((r + 1) % 3 == 0 && r < 8) {
            printf("   %s├───────────┼───────────┼───────────┤%s\n", C_BLUE, C_RESET);
        }
    }

    printf("   %s──────────────────────────────%s\n\n", C_BLUE, C_RESET);
}

void ui_print_move_result(int row, int col, int value, const char *result_str) {
    const char *color = C_YELLOW;

    if (result_str == NULL) {
        result_str = "DESCONHECIDO";
        color = C_RED;
    } else if (result_str[0] == 'O') { /* OK, OCCUPIED */
        color = C_GREEN;
    } else if (result_str[0] == 'F') { /* FIXED */
        color = C_CYAN;
    } else if (result_str[0] == 'E') { /* ERR */
        color = C_RED;
    }

    printf("Jogada: (R=%d, C=%d, V=%d) -> %s%s%s\n",
           row + 1, col + 1, value, color, result_str, C_RESET);
}

void ui_print_summary(int client_id,
                      int game_id,
                      time_t inicio,
                      time_t fim,
                      int total_moves,
                      int correct,
                      int incorrect) {
    double duracao = difftime(fim, inicio);
    if (duracao < 0) duracao = 0;

    printf("\n%sResumo do jogo%s\n", C_BOLD, C_RESET);
    printf("%s──────────────────────────────────────────────%s\n", C_CYAN, C_RESET);
    printf("  Cliente ID:        %d\n", client_id);
    printf("  Jogo ID:           %d\n", game_id);
    printf("  Duração:           %.0f segundos\n", duracao);
    printf("  Jogadas totais:    %d\n", total_moves);
    printf("    Corretas:        %s%d%s\n", C_GREEN, correct, C_RESET);
    printf("    Incorretas:      %s%d%s\n", C_RED, incorrect, C_RESET);
    printf("%s──────────────────────────────────────────────%s\n\n", C_CYAN, C_RESET);
}
