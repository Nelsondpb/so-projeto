#ifndef GAMES_H
#define GAMES_H

#include "config.h"

typedef struct {
    int id;
    int initial[9][9];
    int solution[9][9];
} SudokuGame;

int load_games_csv(const char *path, SudokuGame **games_out, int *n_games_out);
void free_games(SudokuGame *games);

#endif
