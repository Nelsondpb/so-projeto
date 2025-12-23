#ifndef CLIENT_UI_H
#define CLIENT_UI_H

#include <time.h>

/*
 * Imprime um banner inicial com informações sobre o cliente e o jogo.
 */
void ui_print_banner(int client_id, int game_id);

/*
 * Imprime o tabuleiro atual de Sudoku.
 *
 * Parâmetros:
 *   grid    - estado atual (valores 0..9, onde 0 = vazio)
 *   initial - tabuleiro inicial (para distinguir células fixas das preenchidas)
 *
 * Se "initial" for NULL, imprime todas as células da mesma forma.
 */
void ui_print_grid(const int grid[9][9], const int initial[9][9]);

/*
 * Imprime o resultado de uma jogada enviada ao servidor.
 *
 * Exemplo de uso:
 *   ui_print_move_result(r, c, v, "OK");
 */
void ui_print_move_result(int row, int col, int value, const char *result_str);

/*
 * Imprime um resumo final do jogo do ponto de vista do cliente.
 *
 * Parâmetros:
 *   client_id    - ID do cliente
 *   game_id      - ID do jogo
 *   inicio       - timestamp de início
 *   fim          - timestamp de fim
 *   total_moves  - número total de jogadas enviadas
 *   correct      - número de jogadas corretas
 *   incorrect    - número de jogadas incorretas
 */
void ui_print_summary(int client_id,
                      int game_id,
                      time_t inicio,
                      time_t fim,
                      int total_moves,
                      int correct,
                      int incorrect);

#endif
