#include "solver.h"

/*
 * Verifica se um valor pode ser colocado numa posição do tabuleiro.
 *
 * A validação é feita segundo as regras do Sudoku:
 *  - o valor não pode repetir na linha
 *  - o valor não pode repetir na coluna
 *  - o valor não pode repetir no bloco 3x3
 *
 * Parâmetros:
 *  - grid : tabuleiro de Sudoku
 *  - r    : linha a verificar
 *  - c    : coluna a verificar
 *  - v    : valor a testar (1 a 9)
 *
 * Retorna:
 *  - 1 se a jogada for válida
 *  - 0 se a jogada for inválida
 */
static int is_valid(int grid[9][9], int r, int c, int v) {
    /* verificar linha */
    for (int col = 0; col < 9; col++) {
        if (grid[r][col] == v) return 0;
    }

    /* verificar coluna */
    for (int row = 0; row < 9; row++) {
        if (grid[row][c] == v) return 0;
    }

    /* verificar bloco 3x3 */
    int br = (r / 3) * 3;
    int bc = (c / 3) * 3;
    for (int i = br; i < br + 3; i++) {
        for (int j = bc; j < bc + 3; j++) {
            if (grid[i][j] == v) return 0;
        }
    }

    return 1;
}

/*
 * Algoritmo recursivo de backtracking para resolver o Sudoku.
 *
 * Procura uma célula vazia e tenta colocar valores de 1 a 9.
 * Se uma escolha levar a um estado inválido, faz backtracking.
 *
 * Parâmetros:
 *  - grid : tabuleiro de Sudoku a resolver
 *
 * Retorna:
 *  - 1 se o tabuleiro for resolvido com sucesso
 *  - 0 se não existir solução válida
 */
static int backtrack(int grid[9][9]) {
    int r = -1, c = -1;
    int found_empty = 0;

    /* procurar a próxima célula vazia */
    for (int i = 0; i < 9 && !found_empty; i++) {
        for (int j = 0; j < 9 && !found_empty; j++) {
            if (grid[i][j] == 0) {
                r = i;
                c = j;
                found_empty = 1;
            }
        }
    }

    /* se não existem células vazias, o tabuleiro está resolvido */
    if (!found_empty) return 1;

    /* tentar valores de 1 a 9 */
    for (int v = 1; v <= 9; v++) {
        if (is_valid(grid, r, c, v)) {
            grid[r][c] = v;

            if (backtrack(grid)) {
                return 1;
            }

            /* backtracking */
            grid[r][c] = 0;
        }
    }

    return 0;
}

/*
 * Resolve um tabuleiro de Sudoku.
 *
 * Esta função é a interface pública do módulo solver
 * e chama internamente o algoritmo de backtracking.
 *
 * Parâmetros:
 *  - grid : tabuleiro de Sudoku a resolver
 *
 * Retorna:
 *  - 1 se o Sudoku foi resolvido
 *  - 0 se não existe solução
 */
int solve_sudoku(int grid[9][9]) {
    return backtrack(grid);
}
