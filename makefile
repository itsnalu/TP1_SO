CC = gcc
CFLAGS = -Wall -Iinclude

# Arquivos fonte com caminho correto
SRCS = src/main.c src/processoControle.c src/processoSimulado.c src/processos.c src/fila.c src/cpu.c src/estados.c src/gerenciador.c src/processoImpressao.c

# Nome do executável
TARGET = programa

all:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET) 

.PHONY: all clean run
