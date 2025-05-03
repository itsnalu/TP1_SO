CC = gcc
CFLAGS = -Wall -Iinclude

# Arquivos fonte com caminho correto
SRCS = src/main.c src/gerenciador.c src/processo_simulado.c src/processos.c

# Nome do executável
TARGET = programa

all:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET) 

.PHONY: all clean run
