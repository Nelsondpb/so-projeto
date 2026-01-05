#ifndef LOGS_H
#define LOGS_H

/*
 * Regista um evento num ficheiro de log.
 *
 * Cada entrada de log contém:
 *  - timestamp
 *  - identificador do utilizador
 *  - tipo de evento
 *  - descrição opcional
 *
 * Parâmetros:
 *  - logfile : nome ou caminho do ficheiro de log
 *  - userId  : identificador do cliente ou do servidor
 *  - event   : nome do evento a registar
 *  - desc    : descrição opcional do evento
 *
 * Retorna:
 *  - 0  em caso de sucesso
 *  - -1 em caso de erro
 */
int log_event(const char *logfile, int userId,
              const char *event, const char *desc);

#endif
