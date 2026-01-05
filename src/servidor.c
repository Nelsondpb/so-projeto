#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>

#include "config.h"
#include "games.h"
#include "logs.h"
#include "verify.h"
#include "protocol.h"
#include "duel.h"

static ServerConfig g_cfg;
static SudokuGame *g_jogos = NULL;
static int g_num_jogos = 0;

static int *g_em_uso = NULL; 
static pthread_mutex_t g_mutex_jogos = PTHREAD_MUTEX_INITIALIZER;

static int g_total_clientes = 0;
static int g_em_jogo = 0;
static int g_resolvidos = 0;
static pthread_mutex_t g_mutex_stats = PTHREAD_MUTEX_INITIALIZER;

static int g_socket_escuta = -1;
static volatile int g_ativo = 1;

typedef struct {
    int jogo_id;  
    int total_entregues; 
    int tentativas;    
    int corretas;          
    int incorretas;      
    int resolvidos;       
    double soma_tempos;     
    double tempo_min;     
    double tempo_max;       
    pthread_mutex_t m;     
} GameStats;

static GameStats *g_stats = NULL;

typedef struct {
    int fd;
    char ip[64];
    int port;
    int jogo_idx;  
    time_t inicio_jogo; 
    pthread_t tid;
} ClientInfo;

static ClientInfo *g_clients = NULL;
static int g_clients_count = 0;
static pthread_mutex_t g_mutex_clients = PTHREAD_MUTEX_INITIALIZER;


static void add_client_info(int fd, const char *ip, int port, pthread_t tid) {
    pthread_mutex_lock(&g_mutex_clients);
    ClientInfo *tmp = realloc(g_clients, sizeof(ClientInfo) * (g_clients_count + 1));
    if (!tmp) {
        pthread_mutex_unlock(&g_mutex_clients);
        return;
    }
    g_clients = tmp;
    g_clients[g_clients_count].fd = fd;
    strncpy(g_clients[g_clients_count].ip, ip ? ip : "?", sizeof(g_clients[g_clients_count].ip)-1);
    g_clients[g_clients_count].ip[sizeof(g_clients[g_clients_count].ip)-1] = '\0';
    g_clients[g_clients_count].port = port;
    g_clients[g_clients_count].jogo_idx = -1;
    g_clients[g_clients_count].inicio_jogo = 0;
    g_clients[g_clients_count].tid = tid;
    g_clients_count++;
    pthread_mutex_unlock(&g_mutex_clients);
}

static void remove_client_info_by_fd(int fd) {
    pthread_mutex_lock(&g_mutex_clients);
    int i, found = -1;
    for (i = 0; i < g_clients_count; ++i) {
        if (g_clients[i].fd == fd) { found = i; break; }
    }
    if (found >= 0) {
        for (i = found; i < g_clients_count - 1; ++i) g_clients[i] = g_clients[i+1];
        g_clients_count--;
        if (g_clients_count == 0) {
            free(g_clients);
            g_clients = NULL;
        } else {
            ClientInfo *tmp = realloc(g_clients, sizeof(ClientInfo) * g_clients_count);
            if (tmp) g_clients = tmp;
        }
    }
    pthread_mutex_unlock(&g_mutex_clients);
}

static void set_client_game_idx(int fd, int jogo_idx) {
    pthread_mutex_lock(&g_mutex_clients);
    for (int i = 0; i < g_clients_count; ++i) {
        if (g_clients[i].fd == fd) {
            g_clients[i].jogo_idx = jogo_idx;
            if (jogo_idx >= 0) g_clients[i].inicio_jogo = time(NULL);
            else g_clients[i].inicio_jogo = 0;
            break;
        }
    }
    pthread_mutex_unlock(&g_mutex_clients);
}

static void init_game_stats(void) {
    if (!g_num_jogos) return;
    g_stats = calloc(g_num_jogos, sizeof(GameStats));
    if (!g_stats) return;
    for (int i = 0; i < g_num_jogos; ++i) {
        g_stats[i].jogo_id = g_jogos[i].id;
        g_stats[i].total_entregues = 0;
        g_stats[i].tentativas = 0;
        g_stats[i].corretas = 0;
        g_stats[i].incorretas = 0;
        g_stats[i].resolvidos = 0;
        g_stats[i].soma_tempos = 0.0;
        g_stats[i].tempo_min = 1e9;
        g_stats[i].tempo_max = 0.0;
        pthread_mutex_init(&g_stats[i].m, NULL);
    }
}

static void record_game_solved(int jogo_idx, double duracao_seg) {
    if (jogo_idx < 0 || jogo_idx >= g_num_jogos) return;
    GameStats *s = &g_stats[jogo_idx];
    pthread_mutex_lock(&s->m);
    s->resolvidos++;
    s->soma_tempos += duracao_seg;
    if (duracao_seg < s->tempo_min) s->tempo_min = duracao_seg;
    if (duracao_seg > s->tempo_max) s->tempo_max = duracao_seg;
    pthread_mutex_unlock(&s->m);
}

static void record_jogada(int jogo_idx, int correta) {
    if (jogo_idx < 0 || jogo_idx >= g_num_jogos) return;
    GameStats *s = &g_stats[jogo_idx];
    pthread_mutex_lock(&s->m);
    s->tentativas++;
    if (correta) s->corretas++;
    else s->incorretas++;
    pthread_mutex_unlock(&s->m);
}

static void print_last_lines_log(const char *logfile, int nlines) {
    if (!logfile || logfile[0] == 0) {
        printf("Log não configurado.\n");
        return;
    }
    char path[1024];
    if (strchr(logfile, '/') || strchr(logfile, '\\')) {
        strncpy(path, logfile, sizeof(path)-1);
        path[sizeof(path)-1] = '\0';
    } else {
        snprintf(path, sizeof(path), "logs/%s", logfile);
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Não foi possível abrir o ficheiro de log '%s'.\n", path);
        return;
    }

    char **buf = calloc(nlines, sizeof(char*));
    if (!buf) { fclose(f); return; }
    size_t cap = 0;
    ssize_t linelen;
    char *line = NULL;
    int idx = 0;
    while ((linelen = getline(&line, &cap, f)) != -1) {
        if (buf[idx]) free(buf[idx]);
        buf[idx] = strdup(line);
        idx = (idx + 1) % nlines;
    }
    free(line);
    int start = idx;
    int printed = 0;
    for (int i = 0; i < nlines; ++i) {
        int p = (start + i) % nlines;
        if (buf[p]) {
            printf("%s", buf[p]);
            printed++;
        }
    }
    if (printed == 0) printf("(ficheiro de log vazio)\n");

    for (int i = 0; i < nlines; ++i) if (buf[i]) free(buf[i]);
    free(buf);
    fclose(f);
}

void sair_limpo(int codigo) {
    if (g_socket_escuta >= 0) close(g_socket_escuta);
    if (g_jogos) free_games(g_jogos);
    if (g_em_uso) free(g_em_uso);
    if (g_stats) {
        for (int i = 0; i < g_num_jogos; ++i) pthread_mutex_destroy(&g_stats[i].m);
        free(g_stats);
    }
    if (g_clients) free(g_clients);

    duel_shutdown();

    log_event(g_cfg.log_file, 0, "SERVIDOR_STOP", "Servidor encerrado.");
    exit(codigo);
}

void sinal_interrupcao(int sig) {
    (void)sig;
    g_ativo = 0;
    printf("\nEncerramento solicitado. A desligar servidor...\n");
}

void* consola_servidor(void *arg) {
    (void)arg;
    const char *C_RESET = "\033[0m";
    const char *C_BOLD = "\033[1m";
    const char *C_GREEN = "\033[1;32m";
    const char *C_YELLOW = "\033[1;33m";
    const char *C_CYAN = "\033[1;36m";
    const char *C_BLUE = "\033[1;34m";
    const char *C_RED = "\033[1;31m";

    printf("\n\n%sComandos do Servidor%s\n", C_BOLD, C_RESET);
    printf("%s────────────────────────────────────────────────%s\n", C_CYAN, C_RESET);
    printf("  %sestado%s       → Mostrar o estado geral do servidor\n", C_GREEN, C_RESET);
    printf("  %slista%s        → Listar jogos atribuídos e jogadores ativos\n", C_GREEN, C_RESET);
    printf("  %sstats <id>%s   → Mostrar estatísticas detalhadas por jogo\n", C_YELLOW, C_RESET);
    printf("  %slogs%s         → Mostrar últimas 50 linhas do ficheiro de log\n", C_YELLOW, C_RESET);
    printf("  %sduelos%s       → Mostrar duelos 1v1 realizados\n", C_YELLOW, C_RESET);
    printf("  %sencerrar%s     → Desligar servidor\n", C_RED, C_RESET);
    printf("%s────────────────────────────────────────────────%s\n", C_CYAN, C_RESET);

    char cmdline[256];
    while (g_ativo) {
        printf("\n> ");
        fflush(stdout);
        if (!fgets(cmdline, sizeof(cmdline), stdin)) {
            if (feof(stdin)) break;
            continue;
        }
        cmdline[strcspn(cmdline, "\n")] = 0;
        if (strlen(cmdline) == 0) continue;

        if (strcmp(cmdline, "estado") == 0) {

            pthread_mutex_lock(&g_mutex_stats);
            printf("\n%sESTADO DO SERVIDOR%s\n", C_BOLD, C_RESET);
            printf("%s──────────────────────────────────────────%s\n", C_BLUE, C_RESET);
            printf("  Clientes Ligados:      %d\n", g_total_clientes);
            printf("  Jogos a Decorrer:      %d\n", g_em_jogo);
            printf("  Jogos Resolvidos:      %d\n", g_resolvidos);
            pthread_mutex_unlock(&g_mutex_stats);

            if (g_stats) {
                printf("\n%sEstatísticas por jogo:%s\n", C_BOLD, C_RESET);
                for (int i = 0; i < g_num_jogos; ++i) {
                    GameStats *s = &g_stats[i];
                    pthread_mutex_lock(&s->m);
                    double media = (s->resolvidos > 0) ? (s->soma_tempos / s->resolvidos) : 0.0;
                    if (s->tempo_min > 1e8) s->tempo_min = 0.0;
                    printf("  %sJogo ID=%d%s  Jogos Entregues=%d  Tentativas=%d  Corretas=%d  Incorretas=%d  Resolvidos=%d\n",
                           C_GREEN, s->jogo_id, C_RESET, s->total_entregues, s->tentativas, s->corretas, s->incorretas, s->resolvidos);
                    printf("    Tempo min: %.0f s  media: %.0f s  max: %.0f s\n",
                           s->tempo_min, media, s->tempo_max);
                    pthread_mutex_unlock(&s->m);
                }
            }

        } else if (strcmp(cmdline, "lista") == 0) {

            printf("\n%sJogos atribuídos e clientes:%s\n", C_BOLD, C_RESET);
            pthread_mutex_lock(&g_mutex_clients);
            if (g_clients_count == 0) {
                printf("  (nenhum cliente ligado)\n");
            } else {
                for (int i = 0; i < g_clients_count; ++i) {
                    ClientInfo *ci = &g_clients[i];
                    if (ci->jogo_idx >= 0 && ci->jogo_idx < g_num_jogos) {
                        printf("  %s:%d  -> Jogo ID=%d\n", ci->ip, ci->port, g_jogos[ci->jogo_idx].id);
                    } else {
                        printf("  %s:%d  -> (aguarda jogo)\n", ci->ip, ci->port);
                    }
                }
            }
            pthread_mutex_unlock(&g_mutex_clients);

        } else if (strncmp(cmdline, "stats ", 6) == 0) {

            int qid = atoi(cmdline + 6);
            int found = -1;
            for (int i = 0; i < g_num_jogos; ++i) if (g_stats[i].jogo_id == qid) { found = i; break; }
            if (found < 0) {
                printf("Jogo com id %d não encontrado.\n", qid);
            } else {
                GameStats *s = &g_stats[found];
                pthread_mutex_lock(&s->m);
                double media = (s->resolvidos > 0) ? (s->soma_tempos / s->resolvidos) : 0.0;
                if (s->tempo_min > 1e8) s->tempo_min = 0.0;
                printf("\n%sEstatísticas Jogo ID=%d%s\n", C_BOLD, s->jogo_id, C_RESET);
                printf("  Jogos Entregues: %d\n", s->total_entregues);
                printf("  Tentativas: %d\n", s->tentativas);
                printf("  Corretas: %d\n", s->corretas);
                printf("  Incorretas: %d\n", s->incorretas);
                printf("  Resolvidos: %d\n", s->resolvidos);
                printf("  Tempo min: %.0f s  media: %.0f s  max: %.0f s\n", s->tempo_min, media, s->tempo_max);
                pthread_mutex_unlock(&s->m);
            }

        } else if (strcmp(cmdline, "logs") == 0) {

            printf("\n%sÚltimas 50 linhas do log (%s):%s\n", C_BOLD, g_cfg.log_file, C_RESET);
            print_last_lines_log(g_cfg.log_file, 50);

        } else if (strcmp(cmdline, "duelos") == 0) {

            duel_print_duels();

        } else if (strcmp(cmdline, "encerrar") == 0) {

            printf("%sServidor Desligado%s\n", C_RED, C_RESET);
            g_ativo = 0;

            if (g_socket_escuta >= 0) {
                shutdown(g_socket_escuta, SHUT_RDWR);
                close(g_socket_escuta);
                g_socket_escuta = -1;
            }

        } else {
            printf("Comando desconhecido.\n");
        }
    }
    return NULL;
}

/* devolve índice de jogo livre e marca-o em uso;
   aqui consideramos que em modo duelo o mesmo jogo será entregue a 2 clientes,
   por isso incrementamos total_entregues em +2 nessa função específica. */
static int get_free_game_idx_for_duel(void) {
    pthread_mutex_lock(&g_mutex_jogos);
    int idx = -1;
    for (int i = 0; i < g_num_jogos; i++) {
        if (!g_em_uso[i]) {
            g_em_uso[i] = 1;
            idx = i;
            break;
        }
    }
    pthread_mutex_unlock(&g_mutex_jogos);
    if (idx >= 0) {
        pthread_mutex_lock(&g_stats[idx].m);
        g_stats[idx].total_entregues += 2; /* mesmo jogo para 2 clientes */
        pthread_mutex_unlock(&g_stats[idx].m);
    }
    return idx;
}

/* atribuição normal (um jogo para um cliente). */
int atribuir_jogo() {
    pthread_mutex_lock(&g_mutex_jogos);
    int idx = -1;
    for (int i = 0; i < g_num_jogos; i++) {
        if (!g_em_uso[i]) {
            g_em_uso[i] = 1;
            idx = i;
            break;
        }
    }
    pthread_mutex_unlock(&g_mutex_jogos);
    if (idx >= 0) {
        pthread_mutex_lock(&g_stats[idx].m);
        g_stats[idx].total_entregues++;
        pthread_mutex_unlock(&g_stats[idx].m);
    }
    return idx;
}

void libertar_jogo(int idx) {
    if (idx < 0 || idx >= g_num_jogos) return;
    pthread_mutex_lock(&g_mutex_jogos);
    g_em_uso[idx] = 0;
    pthread_mutex_unlock(&g_mutex_jogos);
}

int tabuleiro_completo(int atual[9][9], int solucao[9][9]) {
    for (int r = 0; r < 9; r++)
        for (int c = 0; c < 9; c++)
            if (atual[r][c] != solucao[r][c])
                return 0;
    return 1;
}

void* cliente_thread(void *arg) {

    int fd = *((int*)arg);
    free(arg);

    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    getpeername(fd, (struct sockaddr*)&cli_addr, &cli_len);

    char ip_str[64];
    inet_ntop(AF_INET, &cli_addr.sin_addr, ip_str, sizeof(ip_str));
    int cli_port = ntohs(cli_addr.sin_port);

    pthread_t self = pthread_self();
    add_client_info(fd, ip_str, cli_port, self);

    pthread_mutex_lock(&g_mutex_stats);
    g_total_clientes++;
    pthread_mutex_unlock(&g_mutex_stats);

    int client_id = 0;

    char desc_conn[128];
    snprintf(desc_conn, sizeof(desc_conn), "IP=%s PORT=%d", ip_str, cli_port);
    log_event(g_cfg.log_file, 0, "CLIENT_CONNECT", desc_conn);

    char linha[2048];

    if (recv_line(fd, linha, sizeof(linha)) <= 0) {
        log_event(g_cfg.log_file, 0, "CLIENT_DISCONNECT", "Cliente desconectou prematuramente.");
        close(fd);

        pthread_mutex_lock(&g_mutex_stats);
        g_total_clientes--;
        pthread_mutex_unlock(&g_mutex_stats);

        remove_client_info_by_fd(fd);
        return NULL;
    }

    char *cid = get_field_value(linha, "CLIENT_ID");
    if (cid) {
        client_id = atoi(cid);
        free(cid);
    }

    int idx = -1;

    if (g_cfg.mode == MODE_DUEL) {
        /* MODO DUELO 1v1 */
        idx = duel_wait_for_opponent(fd, client_id, get_free_game_idx_for_duel);
        if (idx < 0) {
            send_line(fd, "STATUS;MSG=Sem jogos disponíveis para duelo.");
            log_event(g_cfg.log_file, client_id, "JOGO_FALHA", "Sem jogos para duelo.");
            close(fd);
            remove_client_info_by_fd(fd);
            return NULL;
        }
    } else {
        /* MODO NORMAL */
        idx = atribuir_jogo();
        if (idx < 0) {
            send_line(fd, "RESP;MSG=Sem jogos disponíveis.");
            log_event(g_cfg.log_file, client_id, "JOGO_FALHA", "Sem jogos disponíveis.");
            close(fd);
            remove_client_info_by_fd(fd);
            return NULL;
        }
    }

    set_client_game_idx(fd, idx);

    char desc_jogo[128];
    snprintf(desc_jogo, sizeof(desc_jogo), "Jogo ID=%d atribuído a %s:%d", g_jogos[idx].id, ip_str, cli_port);
    log_event(g_cfg.log_file, client_id, "JOGO_ATRIBUIDO", desc_jogo);

    pthread_mutex_lock(&g_mutex_stats);
    g_em_jogo++;
    pthread_mutex_unlock(&g_mutex_stats);

    SudokuGame *jogo = &g_jogos[idx];
    int atual[9][9];
    memcpy(atual, jogo->initial, sizeof(atual));

    char tab[82];
    int k = 0;
    for (int r = 0; r < 9; r++)
        for (int c = 0; c < 9; c++)
            tab[k++] = '0' + atual[r][c];
    tab[81] = '\0';

    char msg[256];
    snprintf(msg, sizeof(msg), "JOGO;ID=%d;TAB=%s", jogo->id, tab);
    send_line(fd, msg);

    time_t t_inicio = time(NULL);

    int erros_cliente = 0;

    while (g_ativo) {

        if (recv_line(fd, linha, sizeof(linha)) <= 0) {
            log_event(g_cfg.log_file, client_id, "CLIENT_DISCONNECT", "Cliente saiu a meio do jogo.");

            pthread_mutex_lock(&g_mutex_stats);
            g_total_clientes--;
            g_em_jogo--;
            pthread_mutex_unlock(&g_mutex_stats);

            libertar_jogo(idx);
            log_event(g_cfg.log_file, client_id, "JOGO_LIBERADO", desc_jogo);

            remove_client_info_by_fd(fd);
            close(fd);
            return NULL;
        }

        if (strncmp(linha, "JOGADA", 6) != 0) continue;

        char *r = get_field_value(linha, "R");
        char *c = get_field_value(linha, "C");
        char *v = get_field_value(linha, "V");

        char desc_jogada[128];
        snprintf(desc_jogada, sizeof(desc_jogada),
                 "R=%s C=%s V=%s", r ? r : "?", c ? c : "?", v ? v : "?");
        log_event(g_cfg.log_file, client_id, "JOGADA_RECEBIDA", desc_jogada);

        if (!r || !c || !v) {
            send_line(fd, "RESP_JOGADA;RESULT=ERR;MSG=Faltam parametros");
            log_event(g_cfg.log_file, client_id, "JOGADA_ERRO", "Parâmetros em falta.");
            if (r) free(r);
            if (c) free(c);
            if (v) free(v);
            erros_cliente++;
            record_jogada(idx, 0);
            continue;
        }

        int rr = atoi(r) - 1;
        int cc = atoi(c) - 1;
        int vv = atoi(v);

        free(r);
        free(c);
        free(v);

        if (rr < 0 || rr > 8 || cc < 0 || cc > 8 || vv < 1 || vv > 9) {
            send_line(fd, "RESP_JOGADA;RESULT=ERR;MSG=Valores fora dos limites (1–9)");
            log_event(g_cfg.log_file, client_id, "JOGADA_ERRO", "Jogada fora dos limites.");
            erros_cliente++;
            record_jogada(idx, 0);
            continue;
        }

        if (jogo->initial[rr][cc] != 0) {
            send_line(fd, "RESP_JOGADA;RESULT=FIXED;MSG=Esta célula é fixa.");
            log_event(g_cfg.log_file, client_id, "JOGADA_NEGADA", "Tentou alterar célula fixa.");
            continue;
        }

        if (atual[rr][cc] != 0) {
            send_line(fd, "RESP_JOGADA;RESULT=OCCUPIED;MSG=Já existe um valor.");
            log_event(g_cfg.log_file, client_id, "JOGADA_NEGADA", "Tentou alterar célula preenchida.");
            continue;
        }

        if (vv == jogo->solution[rr][cc]) {

            atual[rr][cc] = vv;
            log_event(g_cfg.log_file, client_id, "JOGADA_CORRETA", "Valor correto inserido.");
            record_jogada(idx, 1);

            if (tabuleiro_completo(atual, jogo->solution)) {
                send_line(fd, "RESP_JOGADA;RESULT=OK;MSG=Sudoku concluído!;SOLVED=1");

                pthread_mutex_lock(&g_mutex_stats);
                g_em_jogo--;
                g_resolvidos++;
                pthread_mutex_unlock(&g_mutex_stats);

                time_t t_fim = time(NULL);
                double dur = difftime(t_fim, t_inicio);
                if (dur < 1.0) dur = 1.0;

                record_game_solved(idx, dur);

                if (g_cfg.mode == MODE_DUEL) {
                    duel_register_finish(fd, dur, erros_cliente);
                }

                log_event(g_cfg.log_file, client_id, "JOGO_TERMINADO", "Cliente concluiu o jogo.");

                libertar_jogo(idx);
                set_client_game_idx(fd, -1);
                break;
            }

            send_line(fd, "RESP_JOGADA;RESULT=OK;MSG=Valor correto!");

        } else {
            send_line(fd, "RESP_JOGADA;RESULT=ERR;MSG=Valor incorreto!");
            log_event(g_cfg.log_file, client_id, "JOGADA_INCORRETA", "Valor inserido não corresponde.");
            erros_cliente++;
            record_jogada(idx, 0);
        }
    }

    pthread_mutex_lock(&g_mutex_stats);
    g_total_clientes--;
    pthread_mutex_unlock(&g_mutex_stats);

    remove_client_info_by_fd(fd);

    close(fd);
    return NULL;
}

int main(int argc, char **argv) {

    const char *ficheiro_conf = (argc > 1 ? argv[1] : "servidor.conf");
    signal(SIGINT, sinal_interrupcao);

    if (load_server_config(ficheiro_conf, &g_cfg) != 0) return 1;

    if (load_games_csv(g_cfg.jogos_file, &g_jogos, &g_num_jogos) != 0) {
        fprintf(stderr, "Erro a carregar ficheiro de jogos '%s'\n", g_cfg.jogos_file);
        return 1;
    }

    g_em_uso = calloc(g_num_jogos, sizeof(int));
    init_game_stats();
    duel_init();

    log_event(g_cfg.log_file, 0, "SERVIDOR_START", "Servidor iniciado.");

    printf("\nServidor Sudoku iniciado na porta %d\n", g_cfg.port);
    printf("Ficheiro de jogos: %s\n", g_cfg.jogos_file);
    printf("Log: %s\n", g_cfg.log_file);
    printf("Clientes máximos: %d\n", g_cfg.max_clients);
    printf("Modo: %s\n", g_cfg.mode == MODE_DUEL ? "DUEL (1v1)" : "NORMAL");

    pthread_t consola;
    pthread_create(&consola, NULL, consola_servidor, NULL);

    g_socket_escuta = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(g_socket_escuta, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in sa = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(g_cfg.port)
    };

    if (bind(g_socket_escuta, (struct sockaddr*)&sa, sizeof(sa)) != 0) {
        perror("bind");
        sair_limpo(1);
    }

    if (listen(g_socket_escuta, g_cfg.max_clients) != 0) {
        perror("listen");
        sair_limpo(1);
    }

    while (g_ativo) {
        struct sockaddr_in cli;
        socklen_t len = sizeof(cli);
        int *fd = malloc(sizeof(int));
        if (!fd) continue;
        *fd = accept(g_socket_escuta, (struct sockaddr*)&cli, &len);
        if (*fd < 0) {
            free(fd);
            if (errno == EINTR) continue;
            break;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, cliente_thread, fd) != 0) {
            close(*fd);
            free(fd);
            continue;
        }
        pthread_detach(tid);
    }

    sair_limpo(0);
    return 0;
}
