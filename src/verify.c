#include "verify.h"

/*
 * Verifica se um conjunto de 9 valores contém repetições
 * ou valores inválidos.
 *
 * Um conjunto é considerado inválido se:
 *  - algum valor não estiver entre 1 e 9
 *  - existir algum valor repetido
 *
 * Parâmetros:
 *  - arr : array de 9 inteiros
 *
 * Retorna:
 *  - 0 se não existirem repetições
 *  - 1 se existirem valores inválidos ou repetidos
 */
static int tem_repetidos_range(int arr[9]) {
    int seen[10] = {0};

    for (int i = 0; i < 9; ++i) {
        int v = arr[i];
        if (v < 1 || v > 9) return 1;
        if (seen[v]) return 1;
        seen[v] = 1;
    }

    return 0;
}

/*
 * Valida uma grelha completa de Sudoku.
 *
 * Verifica todas as regras do Sudoku:
 *  - todas as linhas
 *  - todas as colunas
 *  - todos os blocos 3x3
 *
 * Parâmetros:
 *  - grelha : tabuleiro de Sudoku 9x9
 *
 * Retorna:
 *  - 1 se a grelha for válida
 *  - 0 se a grelha violar alguma regra
 */
int valida_grelha(int grelha[9][9]) {
    int tmp[9];

    /* verificar linhas */
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j)
            tmp[j] = grelha[i][j];
        if (tem_repetidos_range(tmp)) return 0;
    }

    /* verificar colunas */
    for (int j = 0; j < 9; ++j) {
        for (int i = 0; i < 9; ++i)
            tmp[i] = grelha[i][j];
        if (tem_repetidos_range(tmp)) return 0;
    }

    /* verificar blocos 3x3 */
    for (int br = 0; br < 3; ++br)
        for (int bc = 0; bc < 3; ++bc) {
            int idx = 0;
            for (int i = br * 3; i < br * 3 + 3; ++i)
                for (int j = bc * 3; j < bc * 3 + 3; ++j)
                    tmp[idx++] = grelha[i][j];
            if (tem_repetidos_range(tmp)) return 0;
        }

    return 1;
}

/*
 * Verifica uma tentativa de solução enviada por um cliente.
 *
 * Primeiro valida se a grelha respeita as regras do Sudoku.
 * Depois compara a tentativa com a solução correta.
 *
 * Parâmetros:
 *  - tentativa : tabuleiro enviado pelo cliente
 *  - correta   : solução correta do servidor
 *
 * Retorna:
 *  - -1 se a grelha for inválida
 *  - número de células incorretas, se a grelha for válida
 */
int verifica_solucao(int tentativa[9][9], int correta[9][9]) {
    if (!valida_grelha(tentativa)) return -1;

    int err = 0;
    for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j)
            if (tentativa[i][j] != correta[i][j])
                err++;

    return err;
}
