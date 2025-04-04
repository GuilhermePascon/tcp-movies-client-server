# Makefile para compilar o servidor e o cliente

# Compilador
CC = gcc

# Flags de compilação
CFLAGS = -Wall -Wextra -g

# Flags de ligação
LDFLAGS = -lpthread -ljansson

# Arquivos de origem
SERVER_SRC = server.c json_operations.c
CLIENT_SRC = client.c

# Executáveis
SERVER = server
CLIENT = client

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(SERVER) $(CLIENT)

.PHONY: all clean