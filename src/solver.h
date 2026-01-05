#ifndef SOLVER_H
#define SOLVER_H

/*
 * Resolve um tabuleiro de Sudoku.
 *
 * A função recebe um tabuleiro parcialmente preenchido
 * e tenta encontrar uma solução válida utilizando
 * um algoritmo de backtracking.
 *
 * Parâmetros:
 *  - grid : tabuleiro de Sudoku 9x9
 *
 * Retorna:
 *  - 1 se o tabuleiro foi resolvido com sucesso
 *  - 0 se não existe solução válida
 */
int solve_sudoku(int grid[9][9]);

#endif
