#define _POSIX_C_SOURCE 200809L
#include "protocol.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/*
 * Envia uma linha de texto através de um socket.
 *
 * A função garante que toda a string é enviada,
 * mesmo em caso de escritas parciais.
 * Se a string não terminar com '\n', este é acrescentado.
 *
 * Parâmetros:
 *  - fd   : descritor do socket
 *  - line : string a enviar
 *
 * Retorna:
 *  - 0  em caso de sucesso
 *  - -1 em caso de erro
 */
int send_line(int fd, const char *line) {
    if (!line) return -1;

    size_t len = strlen(line);
    int needs_nl = (len == 0 || line[len - 1] != '\n');

    ssize_t total = 0;
    ssize_t n;
    const char *p = line;
    size_t to_write = len;

    while (to_write > 0) {
        n = write(fd, p + total, to_write);
        if (n <= 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total += n;
        to_write -= n;
    }

    if (needs_nl) {
        if (write(fd, "\n", 1) != 1) return -1;
    }

    return 0;
}

/*
 * Recebe uma linha de texto de um socket.
 *
 * A leitura é feita carácter a carácter até encontrar '\n'
 * ou até o buffer ficar cheio.
 *
 * Parâmetros:
 *  - fd     : descritor do socket
 *  - buf    : buffer onde a linha será armazenada
 *  - buflen : tamanho máximo do buffer
 *
 * Retorna:
 *  - número de caracteres lidos
 *  - 0  se a ligação foi encerrada
 *  - -1 em caso de erro
 */
int recv_line(int fd, char *buf, size_t buflen) {
    if (!buf || buflen == 0) return -1;

    size_t pos = 0;

    while (1) {
        char c;
        ssize_t r = read(fd, &c, 1);

        if (r == 0) {
            if (pos == 0) return 0;
            break;
        }

        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }

        if (c == '\r') continue;
        if (c == '\n') break;

        if (pos + 1 < buflen) {
            buf[pos++] = c;
        } else {
            /* descartar o resto da linha */
            while (c != '\n') {
                ssize_t rr = read(fd, &c, 1);
                if (rr <= 0) break;
                if (c == '\n') break;
            }
            break;
        }
    }

    buf[pos] = '\0';
    return (int)pos;
}

/*
 * Extrai o valor de um campo de uma mensagem do protocolo.
 *
 * As mensagens têm o formato:
 *   CHAVE1=valor1;CHAVE2=valor2;...
 *
 * Parâmetros:
 *  - msg : mensagem completa (é modificada internamente)
 *  - key : nome do campo a procurar
 *
 * Retorna:
 *  - string alocada dinamicamente com o valor do campo
 *  - NULL se o campo não existir
 *
 * Nota:
 *  A memória devolvida deve ser libertada com free().
 */
char *get_field_value(char *msg, const char *key) {
    if (!msg || !key) return NULL;

    char *start = msg;
    size_t klen = strlen(key);

    while (start && *start) {
        char *sep = strchr(start, ';');
        size_t seglen = sep ? (size_t)(sep - start) : strlen(start);

        char *eq = memchr(start, '=', seglen);
        if (eq) {
            size_t keylen = (size_t)(eq - start);
            if (keylen == klen && strncmp(start, key, klen) == 0) {
                size_t val_len = seglen - keylen - 1;
                char *val = malloc(val_len + 1);
                if (!val) return NULL;

                memcpy(val, eq + 1, val_len);
                val[val_len] = '\0';
                return val;
            }
        }

        if (!sep) break;
        start = sep + 1;
    }

    return NULL;
}
