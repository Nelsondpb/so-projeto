#include "games.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * Converte uma string com 81 dígitos num tabuleiro 9x9.
 *
 * A string pode conter outros caracteres, sendo apenas
 * considerados os dígitos de 0 a 9.
 *
 * Parâmetros:
 *  - s    : string com a representação do tabuleiro
 *  - grid : matriz 9x9 a preencher
 *
 * Retorna:
 *  - 0  em caso de sucesso
 *  - -1 em caso de erro
 */
static int parse_grid_from_string(const char *s, int grid[9][9]) {
    if (!s) return -1;

    size_t len = strlen(s);
    if (len < 81) return -1;

    int idx = 0;
    for (size_t i = 0; i < len && idx < 81; ++i) {
        char c = s[i];
        if (isdigit((unsigned char)c)) {
            grid[idx / 9][idx % 9] = c - '0';
            idx++;
        }
    }

    if (idx != 81) return -1;
    return 0;
}

/*
 * Carrega os jogos de Sudoku a partir de um ficheiro CSV.
 *
 * Cada linha do ficheiro deve conter:
 *  - ID do jogo
 *  - tabuleiro inicial
 *  - tabuleiro solução
 *
 * Parâmetros:
 *  - path        : caminho para o ficheiro CSV
 *  - games_out   : ponteiro para o array de jogos carregados
 *  - n_games_out : número total de jogos carregados
 *
 * Retorna:
 *  - 0  em caso de sucesso
 *  - -1 em caso de erro
 */
int load_games_csv(const char *path, SudokuGame **games_out, int *n_games_out) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[2048];
    SudokuGame *arr = NULL;
    int count = 0;

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || strlen(line) < 3) continue;

        char *ln = line;
        char *pnl = strchr(ln, '\n');
        if (pnl) *pnl = 0;

        char *tok = strtok(ln, ",");
        if (!tok) continue;

        int id = atoi(tok);
        char *initial = strtok(NULL, ",");
        char *solution = strtok(NULL, ",");

        if (!initial || !solution) continue;

        while (*initial && isspace((unsigned char)*initial)) initial++;
        while (*solution && isspace((unsigned char)*solution)) solution++;

        SudokuGame *tmp = realloc(arr, sizeof(SudokuGame) * (count + 1));
        if (!tmp) {
            free(arr);
            fclose(f);
            return -1;
        }
        arr = tmp;

        arr[count].id = id;

        if (parse_grid_from_string(initial, arr[count].initial) != 0) {
            memset(arr[count].initial, 0, sizeof(arr[count].initial));
        }

        if (parse_grid_from_string(solution, arr[count].solution) != 0) {
            memset(arr[count].solution, 0, sizeof(arr[count].solution));
        }

        count++;
    }

    fclose(f);

    *games_out = arr;
    *n_games_out = count;
    return 0;
}

/*
 * Liberta a memória associada ao array de jogos.
 *
 * Deve ser chamada quando o servidor termina
 * ou quando os jogos já não são necessários.
 */
void free_games(SudokuGame *games) {
    free(games);
}
