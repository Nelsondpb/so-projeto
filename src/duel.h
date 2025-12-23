#ifndef DUEL_H
#define DUEL_H

#include <time.h>

/*
 * Estrutura interna para um duelo entre dois clientes.
 * Não precisas de usar isto diretamente no servidor,
 * é gerido por duel.c.
 */
typedef struct {
    int id;             /* id interno do duelo */
    int jogo_idx;       /* índice do jogo atribuído */
    int fd1, fd2;       /* descritores dos sockets dos clientes */
    int client_id1;     /* CLIENT_ID do jogador 1 */
    int client_id2;     /* CLIENT_ID do jogador 2 */
    time_t inicio;      /* timestamp do início do duelo */
    double tempo1;      /* tempo de resolução do cliente 1 */
    double tempo2;      /* tempo de resolução do cliente 2 */
    int erros1;         /* erros do cliente 1 */
    int erros2;         /* erros do cliente 2 */
    int terminou1;      /* boolean: cliente 1 terminou */
    int terminou2;      /* boolean: cliente 2 terminou */
} Duel;

/*
 * Inicializa o sistema de duelos.
 * Chamar uma vez no arranque do servidor.
 */
void duel_init(void);

/*
 * Liberta a memória e recursos dos duelos.
 * Chamar no shutdown do servidor.
 */
void duel_shutdown(void);

/*
 * Um cliente está à espera de duelo.
 * Retorna:
 *   - jogo_idx atribuído ao duelo, se o cliente foi emparelhado
 *   - -1 em caso de erro
 *
 * Esta função:
 *   - coloca o cliente numa fila de espera
 *   - quando houver dois clientes, escolhe um jogo livre (via callback)
 *   - indica ao servidor qual jogo_idx atribuir a este cliente
 *
 * Parâmetros:
 *   fd         - socket do cliente
 *   client_id  - ID lógico do cliente
 *   get_free_game_idx() - callback do servidor que devolve um jogo livre
 */
int duel_wait_for_opponent(int fd,
                           int client_id,
                           int (*get_free_game_idx)(void));

/*
 * Regista que um cliente terminou o jogo num duelo.
 *
 * Parâmetros:
 *   fd         - socket do cliente
 *   duracao    - tempo em segundos
 *   erros      - número de erros deste cliente
 *
 * A função:
 *   - encontra o duelo onde este fd está
 *   - regista tempo e erros
 *   - se os dois terminaram, calcula vencedor e imprime no servidor
 */
void duel_register_finish(int fd, double duracao, int erros);

/*
 * Imprime lista/resumo de duelos já realizados.
 * Podes chamar isto a partir da consola do servidor (comando "duelos").
 */
void duel_print_duels(void);

#endif
