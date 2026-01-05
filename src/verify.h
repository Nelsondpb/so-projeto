#ifndef VERIFY_H
#define VERIFY_H

/*
 * Verifica uma tentativa de solução enviada por um cliente.
 *
 * A função valida primeiro se a grelha respeita as regras
 * do Sudoku e depois compara com a solução correta.
 *
 * Parâmetros:
 *  - tentativa : tabuleiro enviado pelo cliente
 *  - correta   : tabuleiro solução armazenado no servidor
 *
 * Retorna:
 *  - -1 se a grelha for inválida
 *  - número de células incorretas se a grelha for válida
 */
int verifica_solucao(int tentativa[9][9], int correta[9][9]);

/*
 * Valida uma grelha completa de Sudoku.
 *
 * Verifica se todas as linhas, colunas e blocos 3x3
 * respeitam as regras do Sudoku.
 *
 * Parâmetros:
 *  - grelha : tabuleiro de Sudoku 9x9
 *
 * Retorna:
 *  - 1 se a grelha for válida
 *  - 0 se a grelha for inválida
 */
int valida_grelha(int grelha[9][9]);

#endif
