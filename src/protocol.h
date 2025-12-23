#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>

int send_line(int fd, const char *line);
int recv_line(int fd, char *buf, size_t buflen); 

char *get_field_value(char *msg, const char *key); 

#endif
