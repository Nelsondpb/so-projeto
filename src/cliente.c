#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#include "config.h"
#include "logs.h"
#include "protocol.h"
#include "solver.h"
#include "client_ui.h"

/* ================= CONFIG ================= */

/*
 * Número de threads usadas para resolver o Sudoku em paralelo.
 */
#define N_THREADS 4

/*
 * Número máximo de tentativas de resolução por thread.
 */
#define MAX_TENTATIVAS_THREAD 3

/* ================= TAREFA DA THREAD ================= */

/*
 * Estrutura que representa a tarefa atribuída a cada thread.
 */
typedef struct {
    int tid;   /* identificador da thread */
} SolverTask;

/* ================= ESTADO GLOBAL ================= */

/*
 * Configuração do cliente carregada do ficheiro.
 */
static ClientConfig g_cfg;

/*
 * Socket de comunicação com o servidor.
 */
static int sockfd = -1;

/*
 * Tabuleiros do Sudoku:
 *  - inicial : tabuleiro recebido do servidor
 *  - atual   : estado atual do jogo
 *  - solucao : solução encontrada pelas threads
 */
static int inicial[9][9];
static int atual[9][9];
static int solucao[9][9];

/*
 * Identificador do jogo e instante de início.
 */
static int jogo_id = -1;
static time_t inicio;

/*
 * Estatísticas das jogadas enviadas ao servidor.
 */
static int total_moves = 0;
static int correct_moves = 0;
static int incorrect_moves = 0;

/* ================= SINCRONIZAÇÃO ================= */

/*
 * Mutex para proteger o acesso à solução partilhada.
 */
static pthread_mutex_t mutex_solucao = PTHREAD_MUTEX_INITIALIZER;

/*
 * Barreira para sincronizar o arranque das threads.
 */
static pthread_barrier_t start_barrier;

/*
 * Semáforo para limitar o número de threads
 * a resolver simultaneamente.
 */
static sem_t sem_solver;

/*
 * Indica se alguma thread já encontrou a solução.
 */
static int solucao_encontrada = 0;

/* ================= UTIL ================= */

/*
 * Copia um tabuleiro de Sudoku para outro.
 */
static void copiar_tabuleiro(int dst[9][9], int src[9][9]) {
    memcpy(dst, src, sizeof(int) * 81);
}

/*
 * Processa mensagens STATUS recebidas do servidor.
 * Usado para apresentar mensagens informativas e
 * resultados do modo DUEL.
 */
static void process_status_line(const char *line) {
    if (strncmp(line, "STATUS", 6) != 0) return;

    char buf[512];
    strncpy(buf, line, sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;

    char *msg = get_field_value(buf, "MSG");
    char *res = get_field_value(buf, "RESULT");

    if (msg) {
        printf("%s\n", msg);
    }

    if (res) {
        if (strcmp(res, "WIN") == 0) {
            printf("🏆 Resultado do duelo: VITÓRIA.\n");
        } else if (strcmp(res, "LOSE") == 0) {
            printf("❌ Resultado do duelo: DERROTA.\n");
        } else if (strcmp(res, "DRAW") == 0) {
            printf("⚖️ Resultado do duelo: EMPATE.\n");
        }
    }

    if (msg) free(msg);
    if (res) free(res);
}

/* ================= COMUNICAÇÃO ================= */

/*
 * Estabelece ligação ao servidor e pede um novo jogo.
 * Recebe o tabuleiro inicial e inicializa o estado do cliente.
 */
static int pedir_jogo(void) {

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(g_cfg.server_port);

    if (inet_pton(AF_INET, g_cfg.server_ip, &sa.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&sa, sizeof(sa)) != 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    char req[128];
    snprintf(req, sizeof(req),
             "PIDE_JOGO;CLIENT_ID=%d", g_cfg.client_id);
    if (send_line(sockfd, req) != 0) {
        fprintf(stderr, "Erro a enviar pedido de jogo\n");
        close(sockfd);
        return -1;
    }

    char buf[2048];

    /* Pode receber mensagens STATUS antes do JOGO */
    while (1) {
        if (recv_line(sockfd, buf, sizeof(buf)) <= 0) {
            fprintf(stderr, "Erro a receber resposta do servidor\n");
            close(sockfd);
            return -1;
        }

        if (strncmp(buf, "STATUS", 6) == 0) {
            process_status_line(buf);
            continue;
        }

        if (strncmp(buf, "JOGO", 4) == 0) {
            break;
        }
    }

    char *id = get_field_value(buf, "ID");
    char *tab = get_field_value(buf, "TAB");
    if (!id || !tab) {
        fprintf(stderr, "Resposta de jogo inválida: %s\n", buf);
        if (id) free(id);
        if (tab) free(tab);
        close(sockfd);
        return -1;
    }

    jogo_id = atoi(id);
    free(id);

    int k = 0;
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            int v = tab[k++] - '0';
            inicial[r][c] = v;
            atual[r][c] = v;
        }
    }
    free(tab);

    inicio = time(NULL);

    log_event(g_cfg.log_file, g_cfg.client_id,
              "GAME_RECEIVED", "Sudoku recebido do servidor");

    ui_print_banner(g_cfg.client_id, jogo_id);
    ui_print_grid(atual, inicial);

    return 0;
}

/* ================= THREAD SOLVER ================= */

/*
 * Função executada por cada thread de resolução.
 * Usa barreira, semáforo e mutex para sincronização.
 */
static void* solver_thread(void *arg) {

    SolverTask *task = (SolverTask*)arg;
    char desc[128];

    snprintf(desc, sizeof(desc),
             "Thread %d criada", task->tid);
    log_event(g_cfg.log_file, g_cfg.client_id,
              "THREAD_CREATED", desc);

    /* Esperar que todas as threads estejam prontas */
    log_event(g_cfg.log_file, g_cfg.client_id,
              "BARRIER_WAIT", desc);
    pthread_barrier_wait(&start_barrier);
    log_event(g_cfg.log_file, g_cfg.client_id,
              "BARRIER_PASS", desc);

    for (int t = 1;
         t <= MAX_TENTATIVAS_THREAD && !solucao_encontrada;
         t++) {

        snprintf(desc, sizeof(desc),
                 "Thread %d tentativa %d",
                 task->tid, t);
        log_event(g_cfg.log_file, g_cfg.client_id,
                  "THREAD_TRY", desc);

        /* Limitar o número de threads a resolver simultaneamente */
        log_event(g_cfg.log_file, g_cfg.client_id,
                  "SEM_WAIT", desc);
        sem_wait(&sem_solver);
        log_event(g_cfg.log_file, g_cfg.client_id,
                  "SEM_ENTER", desc);

        usleep(200000);

        int local[9][9];
        copiar_tabuleiro(local, atual);

        if (!solucao_encontrada && solve_sudoku(local)) {

            log_event(g_cfg.log_file, g_cfg.client_id,
                      "MUTEX_LOCK", desc);
            pthread_mutex_lock(&mutex_solucao);

            if (!solucao_encontrada) {
                copiar_tabuleiro(solucao, local);
                solucao_encontrada = 1;

                snprintf(desc, sizeof(desc),
                         "Thread %d encontrou a solução",
                         task->tid);

                log_event(g_cfg.log_file, g_cfg.client_id,
                          "SOLUTION_FOUND", desc);

                printf("Thread %d encontrou uma solução válida.\n",
                       task->tid);
            } else {
                log_event(g_cfg.log_file, g_cfg.client_id,
                          "THREAD_ABORT",
                          "Outra thread já encontrou a solução");
            }

            pthread_mutex_unlock(&mutex_solucao);
            log_event(g_cfg.log_file, g_cfg.client_id,
                      "MUTEX_UNLOCK", desc);
        }

        log_event(g_cfg.log_file, g_cfg.client_id,
                  "SEM_EXIT", desc);
        sem_post(&sem_solver);
    }

    snprintf(desc, sizeof(desc),
             "Thread %d terminou", task->tid);
    log_event(g_cfg.log_file, g_cfg.client_id,
              "THREAD_END", desc);

    printf("Thread %d terminou.\n", task->tid);
    return NULL;
}

/* ================= ENVIO DA SOLUÇÃO ================= */

/*
 * Recebe mensagens STATUS finais do servidor,
 * como o resultado de um duelo.
 */
static void receber_status_final(void) {
    char buf[512];
    while (1) {
        int n = recv_line(sockfd, buf, sizeof(buf));
        if (n <= 0) break;

        if (strncmp(buf, "STATUS", 6) == 0) {
            process_status_line(buf);

            char tmp[512];
            strncpy(tmp, buf, sizeof(tmp)-1);
            tmp[sizeof(tmp)-1] = 0;
            char *res = get_field_value(tmp, "RESULT");
            if (res) {
                free(res);
                break;
            }
        }
    }
}

/*
 * Envia a solução encontrada para o servidor célula a célula.
 */
static void enviar_solucao(void) {

    if (!solucao_encontrada) {
        fprintf(stderr, "Nenhuma solução encontrada pelas threads.\n");
        return;
    }

    char msg[256], resp[512];

    printf("A enviar solução ao servidor...\n");

    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {

            if (inicial[r][c] != 0)
                continue;

            int correto = solucao[r][c];

            int n_erradas = rand() % 3;

            for (int i = 0; i < n_erradas; i++) {
                int errado;
                do {
                    errado = (rand() % 9) + 1;
                } while (errado == correto);

                snprintf(msg, sizeof(msg),
                         "JOGADA;CLIENT_ID=%d;ID=%d;R=%d;C=%d;V=%d",
                         g_cfg.client_id, jogo_id,
                         r + 1, c + 1, errado);

                if (send_line(sockfd, msg) != 0) return;
                if (recv_line(sockfd, resp, sizeof(resp)) <= 0) return;

                total_moves++;
                incorrect_moves++;

                char *result = get_field_value(resp, "RESULT");
                ui_print_move_result(r, c, errado,
                                     result ? result : "ERR");
                if (result) free(result);
            }

            snprintf(msg, sizeof(msg),
                     "JOGADA;CLIENT_ID=%d;ID=%d;R=%d;C=%d;V=%d",
                     g_cfg.client_id, jogo_id,
                     r + 1, c + 1, correto);

            if (send_line(sockfd, msg) != 0) return;
            if (recv_line(sockfd, resp, sizeof(resp)) <= 0) return;

            total_moves++;
            char *result = get_field_value(resp, "RESULT");
            char *solved = get_field_value(resp, "SOLVED");

            if (result && strcmp(result, "OK") == 0) {
                correct_moves++;
                atual[r][c] = correto;
            } else {
                incorrect_moves++;
            }

            ui_print_move_result(r, c, correto,
                                 result ? result : "RESP");

            if (result) free(result);

            if (solved && atoi(solved) == 1) {
                free(solved);
                ui_print_grid(atual, inicial);
                receber_status_final();
                return;
            }
            if (solved) free(solved);

            ui_print_grid(atual, inicial);
        }
    }
}

/* ================= MAIN ================= */

/*
 * Função principal do cliente.
 */
int main(int argc, char **argv) {

    srand((unsigned int)time(NULL));

    const char *cfgfile = (argc > 1 ? argv[1] : "cliente.conf");

    if (load_client_config(cfgfile, &g_cfg) != 0) {
        fprintf(stderr, "Erro ao carregar configuração do cliente (%s)\n", cfgfile);
        return 1;
    }

    log_event(g_cfg.log_file, g_cfg.client_id,
              "CLIENT_START", "Cliente iniciado");

    if (pedir_jogo() != 0) {
        fprintf(stderr, "Erro ao pedir jogo ao servidor\n");
        return 1;
    }

    pthread_barrier_init(&start_barrier, NULL, N_THREADS);
    sem_init(&sem_solver, 0, 2);

    pthread_t threads[N_THREADS];
    SolverTask tasks[N_THREADS];

    for (int i = 0; i < N_THREADS; i++) {
        tasks[i].tid = i;
        if (pthread_create(&threads[i], NULL,
                           solver_thread, &tasks[i]) != 0) {
            fprintf(stderr, "Erro a criar thread %d\n", i);
        }
    }

    for (int i = 0; i < N_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    enviar_solucao();

    close(sockfd);

    log_event(g_cfg.log_file, g_cfg.client_id,
              "CLIENT_END", "Cliente terminou");

    time_t fim = time(NULL);
    ui_print_summary(g_cfg.client_id,
                     jogo_id,
                     inicio,
                     fim,
                     total_moves,
                     correct_moves,
                     incorrect_moves);

    pthread_barrier_destroy(&start_barrier);
    sem_destroy(&sem_solver);

    return 0;
}
