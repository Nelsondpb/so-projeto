#ifndef SOLVER_H
#define SOLVER_H

/*
 * Resolve um tabuleiro de Sudoku 9x9.
 *
 * Parâmetros:
 *   grid - matriz 9x9 com o estado atual do tabuleiro.
 *          0 representa célula vazia.
 *
 * Retorna:
 *   1 se encontrou uma solução válida e o tabuleiro foi preenchido;
 *   0 se não existe solução válida.
 *
 * Nota:
 *   A função altera o tabuleiro "grid" in-place.
 */
int solve_sudoku(int grid[9][9]);

#endif
