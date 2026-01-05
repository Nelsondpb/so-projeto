#ifndef DUEL_H
#define DUEL_H

#include <time.h>

/*
 * Estrutura que representa um duelo entre dois clientes.
 *
 * Guarda toda a informação necessária para determinar
 * o vencedor de um jogo competitivo.
 */
typedef struct {
    int id;             /* identificador interno do duelo */
    int jogo_idx;       /* índice do jogo atribuído */
    int fd1, fd2;       /* sockets dos dois clientes */
    int client_id1;     /* CLIENT_ID do jogador 1 */
    int client_id2;     /* CLIENT_ID do jogador 2 */
    time_t inicio;      /* instante de início do duelo */
    double tempo1;      /* tempo de resolução do jogador 1 */
    double tempo2;      /* tempo de resolução do jogador 2 */
    int erros1;         /* número de erros do jogador 1 */
    int erros2;         /* número de erros do jogador 2 */
    int terminou1;      /* indica se o jogador 1 terminou */
    int terminou2;      /* indica se o jogador 2 terminou */
} Duel;

/*
 * Inicializa o sistema de duelos.
 *
 * Deve ser chamada uma vez no arranque do servidor.
 */
void duel_init(void);

/*
 * Liberta os recursos associados ao sistema de duelos.
 *
 * Deve ser chamada no encerramento do servidor.
 */
void duel_shutdown(void);

/*
 * Coloca um cliente à espera de um adversário para duelo.
 *
 * Se existir outro cliente à espera, cria um duelo
 * e atribui um jogo livre.
 *
 * Parâmetros:
 *  - fd                : socket do cliente
 *  - client_id         : ID lógico do cliente
 *  - get_free_game_idx : callback que devolve um jogo disponível
 *
 * Retorna:
 *  - índice do jogo atribuído
 *  - -1 em caso de erro
 */
int duel_wait_for_opponent(int fd,
                           int client_id,
                           int (*get_free_game_idx)(void));

/*
 * Regista que um cliente terminou o jogo num duelo.
 *
 * Guarda estatísticas de tempo e erros e,
 * quando ambos os jogadores terminam,
 * calcula o vencedor.
 *
 * Parâmetros:
 *  - fd      : socket do cliente
 *  - duracao : tempo de resolução em segundos
 *  - erros   : número de erros cometidos
 */
void duel_register_finish(int fd, double duracao, int erros);

/*
 * Imprime no terminal do servidor um resumo
 * de todos os duelos já realizados.
 */
void duel_print_duels(void);

#endif
