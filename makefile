CC = gcc
# Se quiser compilação condicional, use uma flag como -DUSE_THREADS
# CFLAGS = -Wall -Iinclude # Sem -pthread aqui por padrão
CFLAGS = -Wall -Iinclude -pthread # Adiciona -pthread para compilar com suporte a threads

LDFLAGS = -lm -pthread # Adiciona -pthread para linkar com a biblioteca pthread

SRCS = src/main.c src/processoSimulado.c src/processos.c src/fila.c src/cpu.c src/estados.c src/gerenciador.c src/processoImpressao.c src/lista.c src/threads.c 

TARGET_PRIORITY = programa
TARGET_FIFO = programa_fifo
# Adicione um novo alvo para a versão com threads, se desejar manter separado
# TARGET_THREADED = programa_threaded

all: $(TARGET_PRIORITY) # Adicione $(TARGET_FIFO) $(TARGET_THREADED) se tiver alvos separados

priority: $(TARGET_PRIORITY)

fifo: $(TARGET_FIFO)

# Regra para criar o executável de prioridade (agora com -pthread)
$(TARGET_PRIORITY): $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Regra para criar o executável FIFO (agora com -pthread)
$(TARGET_FIFO): $(SRCS)
	$(CC) $(CFLAGS) -DUSE_FIFO $^ -o $@ $(LDFLAGS)

# Exemplo de alvo para uma versão explicitamente com threads (se usar -DUSE_THREADS)
# $(TARGET_THREADED): $(SRCS)
#	$(CC) $(CFLAGS) -DUSE_THREADS $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET_PRIORITY) $(TARGET_FIFO) *.o src/*.o # Adicione $(TARGET_THREADED)

# ...
.PHONY: all clean priority fifo # Adicione threaded run_threaded