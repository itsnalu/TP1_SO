CC = gcc
# CFLAGS = -Wall -Iinclude # Sem -pthread aqui por padrão se quiser compilação condicional
CFLAGS = -Wall -Iinclude -pthread # Adiciona -pthread para compilar sempre com suporte a threads

LDFLAGS = -lm -pthread # Adiciona -pthread para linkar com a biblioteca pthread

SRCS = src/main.c src/processoSimulado.c src/processos.c src/fila.c src/cpu.c src/estados.c src/gerenciador.c src/processoImpressao.c src/lista.c src/threads.c 

TARGET_PRIORITY = programa
TARGET_FIFO = programa_fifo
# TARGET_THREADED = programa_threaded # Descomente e defina se quiser um alvo separado para threads com -DUSE_THREADS

# O alvo 'all' agora compila ambos por padrão se você quiser
all: $(TARGET_PRIORITY) $(TARGET_FIFO) # Adicione $(TARGET_THREADED) se tiver

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
	rm -f $(TARGET_PRIORITY) $(TARGET_FIFO) *.o src/*.o # Adicione $(TARGET_THREADED) se tiver

# Alvos para executar
run: $(TARGET_PRIORITY)
	./$(TARGET_PRIORITY)

run_fifo: $(TARGET_FIFO)
	./$(TARGET_FIFO)

# run_threaded: $(TARGET_THREADED) # Descomente se tiver um alvo TARGET_THREADED
#	./$(TARGET_THREADED)

# Adicionar os novos alvos run ao .PHONY
.PHONY: all clean priority fifo run run_fifo # Adicione threaded run_threaded se tiver