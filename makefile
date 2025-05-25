# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I./include
LDFLAGS = -pthread

# Arquivos fonte
SRCS = src/cpu.c src/estados.c src/fila.c src/gerenciador.c src/lista.c \
       src/main.c src/processoControle.c src/processoImpressao.c \
       src/processoSimulado.c src/processos.c src/threads.c

# Executável
TARGET = simulador

# Regra principal
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

# Limpeza
clean:
	rm -f $(TARGET)

# Recompilação
rebuild: clean all

# Execução
run: all
	./$(TARGET)

.PHONY: all clean rebuild run
