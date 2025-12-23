#include "verify.h"

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

int valida_grelha(int grelha[9][9]) {
    int tmp[9];
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j) tmp[j] = grelha[i][j];
        if (tem_repetidos_range(tmp)) return 0;
    }
    for (int j = 0; j < 9; ++j) {
        for (int i = 0; i < 9; ++i) tmp[i] = grelha[i][j];
        if (tem_repetidos_range(tmp)) return 0;
    }
    for (int br = 0; br < 3; ++br) for (int bc = 0; bc < 3; ++bc) {
        int idx = 0;
        for (int i = br*3; i < br*3+3; ++i)
            for (int j = bc*3; j < bc*3+3; ++j)
                tmp[idx++] = grelha[i][j];
        if (tem_repetidos_range(tmp)) return 0;
    }
    return 1;
}

int verifica_solucao(int tentativa[9][9], int correta[9][9]) {
    if (!valida_grelha(tentativa)) return -1;
    int err = 0;
    for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j)
            if (tentativa[i][j] != correta[i][j]) err++;
    return err;
}
