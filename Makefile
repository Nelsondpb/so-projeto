CC = gcc
CFLAGS = -Wall -Wextra -g -I./src
SRCS = src/config.c src/games.c src/logs.c src/verify.c
CLIENT_SRC = src/cliente.c
SERVER_SRC = src/servidor.c

.PHONY: all clean test

all: servidor cliente
	@echo "\033[1;32m✔ Compilação concluída\033[0m"

servidor: $(SRCS) $(SERVER_SRC)
	$(CC) $(CFLAGS) -o servidor $(SERVER_SRC) $(SRCS)
	@echo "\033[1;36m--> servidor criado\033[0m"

cliente: $(SRCS) $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o cliente $(CLIENT_SRC) $(SRCS)
	@echo "\033[1;36m--> cliente criado\033[0m"

test: all
	@echo "\033[1;33mExecutar ./servidor server.conf e ./cliente cliente.conf para demo\033[0m"

clean:
	@echo "\033[1;31mLimpando binarios e logs...\033[0m"
	rm -f servidor cliente
	rm -rf logs
	@echo "\033[1;31mLimpeza concluída\033[0m"
