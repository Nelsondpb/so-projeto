#include "games.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int parse_grid_from_string(const char *s, int grid[9][9]) {
    if (!s) return -1;
    size_t len = strlen(s);
    if (len < 81) return -1;
    int idx = 0;
    for (size_t i = 0; i < len && idx < 81; ++i) {
        char c = s[i];
        if (isdigit((unsigned char)c)) {
            grid[idx/9][idx%9] = c - '0';
            idx++;
        }
    }
    if (idx != 81) return -1;
    return 0;
}

int load_games_csv(const char *path, SudokuGame **games_out, int *n_games_out) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    char line[2048];
    SudokuGame *arr = NULL;
    int count = 0;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || strlen(line) < 3) continue;
        char *ln = line;
        char *pnl = strchr(ln, '\n'); if (pnl) *pnl = 0;
        char *tok = strtok(ln, ",");
        if (!tok) continue;
        int id = atoi(tok);
        char *initial = strtok(NULL, ",");
        char *solution = strtok(NULL, ",");
        if (!initial || !solution) continue;
        while (*initial && isspace((unsigned char)*initial)) initial++;
        while (*solution && isspace((unsigned char)*solution)) solution++;
        SudokuGame *tmp = realloc(arr, sizeof(SudokuGame) * (count + 1));
        if (!tmp) { free(arr); fclose(f); return -1; }
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

void free_games(SudokuGame *games) {
    free(games);
}
