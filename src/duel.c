#include "duel.h"
#include "protocol.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_GREEN   "\033[1;32m"
#define C_YELLOW  "\033[1;33m"
#define C_CYAN    "\033[1;36m"
#define C_RED     "\033[1;31m"

/* Estrutura para um cliente à espera de duelo */
typedef struct {
    int fd;
    int client_id;
} WaitingClient;

static WaitingClient g_waiting = { .fd = -1, .client_id = 0 };
static Duel *g_duels = NULL;
static int g_duel_count = 0;
static int g_next_duel_id = 1;

static pthread_mutex_t g_mutex_duel = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cond_wait = PTHREAD_COND_INITIALIZER;

void duel_init(void) {
    pthread_mutex_lock(&g_mutex_duel);
    g_waiting.fd = -1;
    g_waiting.client_id = 0;
    g_duels = NULL;
    g_duel_count = 0;
    g_next_duel_id = 1;
    pthread_mutex_unlock(&g_mutex_duel);
}

void duel_shutdown(void) {
    pthread_mutex_lock(&g_mutex_duel);
    free(g_duels);
    g_duels = NULL;
    g_duel_count = 0;
    g_waiting.fd = -1;
    g_waiting.client_id = 0;
    pthread_mutex_unlock(&g_mutex_duel);
}

/* Cria um novo duelo com dois clientes e um jogo_idx */
static Duel* create_duel(int fd1, int client_id1,
                         int fd2, int client_id2,
                         int jogo_idx) {
    Duel *tmp = realloc(g_duels, sizeof(Duel) * (g_duel_count + 1));
    if (!tmp) return NULL;
    g_duels = tmp;

    Duel *d = &g_duels[g_duel_count];
    d->id = g_next_duel_id++;
    d->jogo_idx = jogo_idx;
    d->fd1 = fd1;
    d->fd2 = fd2;
    d->client_id1 = client_id1;
    d->client_id2 = client_id2;
    d->inicio = time(NULL);
    d->tempo1 = d->tempo2 = 0.0;
    d->erros1 = d->erros2 = 0;
    d->terminou1 = d->terminou2 = 0;

    g_duel_count++;
    return d;
}

/* procura duelo pelo fd de um dos jogadores */
static Duel* find_duel_by_fd(int fd) {
    for (int i = 0; i < g_duel_count; ++i) {
        if (g_duels[i].fd1 == fd || g_duels[i].fd2 == fd) {
            return &g_duels[i];
        }
    }
    return NULL;
}

int duel_wait_for_opponent(int fd,
                           int client_id,
                           int (*get_free_game_idx)(void)) {
    pthread_mutex_lock(&g_mutex_duel);

    /* se não há ninguém à espera, este cliente passa a estar em espera */
    if (g_waiting.fd < 0) {
        g_waiting.fd = fd;
        g_waiting.client_id = client_id;

        /* informar cliente que está à espera */
        send_line(fd, "STATUS;MSG=A aguardar adversario para duelo...");

        /* espera até aparecer um segundo cliente ou até o fd mudar */
        while (g_waiting.fd == fd) {
            pthread_cond_wait(&g_cond_wait, &g_mutex_duel);
        }

        /* quando sair daqui, o duelo já foi criado por outro cliente */
        Duel *d = find_duel_by_fd(fd);
        int jogo_idx = d ? d->jogo_idx : -1;
        pthread_mutex_unlock(&g_mutex_duel);
        return jogo_idx;
    }

    /* já há um cliente à espera -> emparelhar */
    int fd1 = g_waiting.fd;
    int cid1 = g_waiting.client_id;

    /* limpar espera */
    g_waiting.fd = -1;
    g_waiting.client_id = 0;

    /* escolher jogo livre */
    int jogo_idx = -1;
    if (get_free_game_idx) {
        jogo_idx = get_free_game_idx();
    }

    if (jogo_idx < 0) {
        /* não há jogo livre -> não cria duelo */
        pthread_mutex_unlock(&g_mutex_duel);
        return -1;
    }

    /* criar duelo */
    Duel *d = create_duel(fd1, cid1, fd, client_id, jogo_idx);
    if (!d) {
        pthread_mutex_unlock(&g_mutex_duel);
        return -1;
    }

    /* informar ambos que o duelo começou */
    send_line(fd1, "STATUS;MSG=Duelo iniciado! Boa sorte!");
    send_line(fd,  "STATUS;MSG=Duelo iniciado! Boa sorte!");

    /* acordar o cliente que estava à espera */
    pthread_cond_broadcast(&g_cond_wait);

    pthread_mutex_unlock(&g_mutex_duel);
    return jogo_idx;
}

void duel_register_finish(int fd, double duracao, int erros) {
    pthread_mutex_lock(&g_mutex_duel);
    Duel *d = find_duel_by_fd(fd);
    if (!d) {
        pthread_mutex_unlock(&g_mutex_duel);
        return;
    }

    if (fd == d->fd1) {
        d->tempo1 = duracao;
        d->erros1 = erros;
        d->terminou1 = 1;
    } else if (fd == d->fd2) {
        d->tempo2 = duracao;
        d->erros2 = erros;
        d->terminou2 = 1;
    }

    /* se só um terminou, informar que está à espera do adversário */
    if ((d->terminou1 && !d->terminou2) ||
        (d->terminou2 && !d->terminou1)) {
        send_line(fd, "STATUS;MSG=Sudoku concluido. A aguardar adversario terminar...");
        pthread_mutex_unlock(&g_mutex_duel);
        return;
    }

    /* se ambos terminaram, imprimir resultado e informar cada cliente */
    if (d->terminou1 && d->terminou2) {
        printf("\n%sResultado do Duelo ID=%d%s\n", C_BOLD, d->id, C_RESET);
        printf("%s──────────────────────────────────────────────%s\n", C_CYAN, C_RESET);
        printf("  Jogador 1 (CLIENT_ID=%d): tempo=%.0f s, erros=%d\n",
               d->client_id1, d->tempo1, d->erros1);
        printf("  Jogador 2 (CLIENT_ID=%d): tempo=%.0f s, erros=%d\n",
               d->client_id2, d->tempo2, d->erros2);

        int vencedor = 0;
        /* critério 1: menor tempo */
        if (d->tempo1 < d->tempo2) vencedor = 1;
        else if (d->tempo2 < d->tempo1) vencedor = 2;
        else {
            /* critério 2: menos erros */
            if (d->erros1 < d->erros2) vencedor = 1;
            else if (d->erros2 < d->erros1) vencedor = 2;
        }

        if (vencedor == 1) {
            printf("  %sVencedor: CLIENT_ID=%d%s\n", C_GREEN, d->client_id1, C_RESET);
        } else if (vencedor == 2) {
            printf("  %sVencedor: CLIENT_ID=%d%s\n", C_GREEN, d->client_id2, C_RESET);
        } else {
            printf("  %sEmpate!%s\n", C_YELLOW, C_RESET);
        }
        printf("%s──────────────────────────────────────────────%s\n", C_CYAN, C_RESET);

        /* enviar resultado para os dois clientes */
        char buf[128];

        if (vencedor == 1) {
            snprintf(buf, sizeof(buf), "STATUS;RESULT=WIN;MSG=Venceu o duelo!");
            send_line(d->fd1, buf);
            snprintf(buf, sizeof(buf), "STATUS;RESULT=LOSE;MSG=Perdeu o duelo.");
            send_line(d->fd2, buf);
        } else if (vencedor == 2) {
            snprintf(buf, sizeof(buf), "STATUS;RESULT=LOSE;MSG=Perdeu o duelo.");
            send_line(d->fd1, buf);
            snprintf(buf, sizeof(buf), "STATUS;RESULT=WIN;MSG=Venceu o duelo!");
            send_line(d->fd2, buf);
        } else {
            snprintf(buf, sizeof(buf), "STATUS;RESULT=DRAW;MSG=Empate no duelo.");
            send_line(d->fd1, buf);
            send_line(d->fd2, buf);
        }
    }

    pthread_mutex_unlock(&g_mutex_duel);
}

void duel_print_duels(void) {
    pthread_mutex_lock(&g_mutex_duel);

    if (g_duel_count == 0) {
        printf("Nenhum duelo registado ainda.\n");
        pthread_mutex_unlock(&g_mutex_duel);
        return;
    }

    printf("\n%sDuelos realizados%s\n", C_BOLD, C_RESET);
    printf("%s──────────────────────────────────────────────%s\n", C_CYAN, C_RESET);
    for (int i = 0; i < g_duel_count; ++i) {
        Duel *d = &g_duels[i];
        printf("  Duelo ID=%d  JogoIdx=%d\n", d->id, d->jogo_idx);
        printf("    CLIENT_ID1=%d  tempo=%.0f s  erros=%d  terminou=%d\n",
               d->client_id1, d->tempo1, d->erros1, d->terminou1);
        printf("    CLIENT_ID2=%d  tempo=%.0f s  erros=%d  terminou=%d\n",
               d->client_id2, d->tempo2, d->erros2, d->terminou2);
    }
    printf("%s──────────────────────────────────────────────%s\n\n", C_CYAN, C_RESET);

    pthread_mutex_unlock(&g_mutex_duel);
}
