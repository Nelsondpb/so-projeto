#include "solver.h"

/*
 * Verifica se é válido colocar o valor v na posição (r, c)
 * de acordo com as regras do Sudoku:
 *  - não repetir na linha
 *  - não repetir na coluna
 *  - não repetir no bloco 3x3
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

    /* verificar sub-bloco 3x3 */
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
 * Função recursiva de backtracking que tenta resolver o tabuleiro.
 *
 * Percorre o tabuleiro em ordem linha/coluna à procura da primeira
 * célula vazia (valor 0) e tenta colocar valores de 1 a 9 que sejam
 * válidos. Se em algum ponto ficar preso, volta atrás (backtrack).
 */
static int backtrack(int grid[9][9]) {
    /* procurar próxima célula vazia */
    int r = -1, c = -1;
    int found_empty = 0;

    for (int i = 0; i < 9 && !found_empty; i++) {
        for (int j = 0; j < 9 && !found_empty; j++) {
            if (grid[i][j] == 0) {
                r = i;
                c = j;
                found_empty = 1;
            }
        }
    }

    /* se não há células vazias, está resolvido */
    if (!found_empty) return 1;

    /* tentar valores 1..9 */
    for (int v = 1; v <= 9; v++) {
        if (is_valid(grid, r, c, v)) {
            grid[r][c] = v;

            if (backtrack(grid)) {
                return 1;
            }

            /* backtrack */
            grid[r][c] = 0;
        }
    }

    /* nenhuma opção funcionou */
    return 0;
}

int solve_sudoku(int grid[9][9]) {
    return backtrack(grid);
}
