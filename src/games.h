#ifndef GAMES_H
#define GAMES_H

#include "config.h"

/*
 * Estrutura que representa um jogo de Sudoku.
 *
 * Contém o identificador do jogo, o tabuleiro inicial
 * e a respetiva solução completa.
 */
typedef struct {
    int id;                 /* identificador do jogo */
    int initial[9][9];      /* tabuleiro inicial */
    int solution[9][9];     /* tabuleiro solução */
} SudokuGame;

/*
 * Carrega os jogos de Sudoku a partir de um ficheiro CSV.
 *
 * O ficheiro deve conter, por linha:
 *  - ID do jogo
 *  - tabuleiro inicial
 *  - tabuleiro solução
 *
 * Parâmetros:
 *  - path        : caminho para o ficheiro CSV
 *  - games_out   : ponteiro para o array de jogos carregados
 *  - n_games_out : número de jogos carregados
 *
 * Retorna:
 *  - 0  em caso de sucesso
 *  - -1 em caso de erro
 */
int load_games_csv(const char *path, SudokuGame **games_out, int *n_games_out);

/*
 * Liberta a memória associada ao array de jogos.
 *
 * Deve ser chamada quando os jogos deixam
 * de ser necessários.
 */
void free_games(SudokuGame *games);

#endif
