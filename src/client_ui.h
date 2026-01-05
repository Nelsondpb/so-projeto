#ifndef CLIENT_UI_H
#define CLIENT_UI_H

#include <time.h>

/*
 * Imprime um banner inicial no terminal com informações do cliente.
 *
 * Mostra o título da aplicação, o ID do cliente e o ID do jogo
 * (caso já tenha sido atribuído pelo servidor).
 *
 * Parâmetros:
 *  - client_id : identificador do cliente
 *  - game_id   : identificador do jogo (-1 se ainda não existir)
 */
void ui_print_banner(int client_id, int game_id);

/*
 * Imprime o tabuleiro atual de Sudoku no terminal.
 *
 * O tabuleiro é apresentado no formato 9x9 com separadores
 * para os blocos 3x3.
 *
 * Parâmetros:
 *  - grid    : estado atual do tabuleiro
 *  - initial : tabuleiro inicial, usado para identificar células fixas
 *
 * Se initial for NULL, todas as células são tratadas como normais.
 */
void ui_print_grid(const int grid[9][9], const int initial[9][9]);

/*
 * Imprime no terminal o resultado de uma jogada enviada ao servidor.
 *
 * A mensagem indica a posição, o valor enviado e o resultado
 * devolvido pelo servidor (ex: OK, ERR, FIXED).
 *
 * Parâmetros:
 *  - row        : linha da jogada (0-based)
 *  - col        : coluna da jogada (0-based)
 *  - value      : valor enviado
 *  - result_str : string com o resultado devolvido pelo servidor
 */
void ui_print_move_result(int row, int col, int value, const char *result_str);

/*
 * Imprime um resumo final do jogo do ponto de vista do cliente.
 *
 * Apresenta estatísticas como duração do jogo, número de jogadas
 * totais, corretas e incorretas.
 *
 * Parâmetros:
 *  - client_id   : ID do cliente
 *  - game_id     : ID do jogo
 *  - inicio      : instante de início do jogo
 *  - fim         : instante de fim do jogo
 *  - total_moves : número total de jogadas enviadas
 *  - correct     : número de jogadas corretas
 *  - incorrect   : número de jogadas incorretas
 */
void ui_print_summary(int client_id,
                      int game_id,
                      time_t inicio,
                      time_t fim,
                      int total_moves,
                      int correct,
                      int incorrect);

#endif
