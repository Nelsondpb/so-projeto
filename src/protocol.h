#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>

/*
 * Envia uma linha de texto através de um socket.
 *
 * A função garante que toda a mensagem é enviada
 * e adiciona um caractere de nova linha ('\n')
 * caso este não exista.
 *
 * Parâmetros:
 *  - fd   : descritor do socket
 *  - line : string a enviar
 *
 * Retorna:
 *  - 0  em caso de sucesso
 *  - -1 em caso de erro
 */
int send_line(int fd, const char *line);

/*
 * Recebe uma linha de texto de um socket.
 *
 * A leitura termina ao encontrar um caractere
 * de nova linha ('\n') ou quando o buffer fica cheio.
 *
 * Parâmetros:
 *  - fd     : descritor do socket
 *  - buf    : buffer onde a mensagem será armazenada
 *  - buflen : tamanho máximo do buffer
 *
 * Retorna:
 *  - número de caracteres lidos
 *  - 0  se a ligação foi encerrada
 *  - -1 em caso de erro
 */
int recv_line(int fd, char *buf, size_t buflen);

/*
 * Extrai o valor de um campo de uma mensagem do protocolo.
 *
 * As mensagens seguem o formato:
 *   CHAVE=valor;CHAVE2=valor2;...
 *
 * Parâmetros:
 *  - msg : mensagem completa (pode ser modificada)
 *  - key : nome do campo a procurar
 *
 * Retorna:
 *  - string alocada dinamicamente com o valor encontrado
 *  - NULL se o campo não existir
 *
 * Nota:
 *  A memória devolvida deve ser libertada com free().
 */
char *get_field_value(char *msg, const char *key);

#endif
