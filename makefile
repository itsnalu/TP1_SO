CC = gcc
CFLAGS = -Wall -Iinclude
LDFLAGS = -lm # Adicionado para linkar a biblioteca matemática, se necessário

# Arquivos fonte (verifique se processoControle.c é realmente necessário ou se lista.c deve ser incluído)
# Assumindo que lista.c é necessário e processoControle.c não é diretamente usado pelo main
SRCS = src/main.c src/processoSimulado.c src/processos.c src/fila.c src/cpu.c src/estados.c src/gerenciador.c src/processoImpressao.c src/lista.c src/threads.c 

# Nomes dos executáveis
TARGET_PRIORITY = programa
TARGET_FIFO = programa_fifo

# Alvo padrão: compila a versão com prioridade
all: $(TARGET_PRIORITY)

# Alvo para compilar a versão com prioridade
priority: $(TARGET_PRIORITY)

# Alvo para compilar a versão com FIFO
fifo: $(TARGET_FIFO)

# Regra para criar o executável de prioridade
$(TARGET_PRIORITY): $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Regra para criar o executável FIFO
$(TARGET_FIFO): $(SRCS)
	$(CC) $(CFLAGS) -DUSE_FIFO $^ -o $@ $(LDFLAGS)

# Limpeza
clean:
	rm -f $(TARGET_PRIORITY) $(TARGET_FIFO) *.o src/*.o

# Executar a versão de prioridade (compila se necessário)
run: $(TARGET_PRIORITY)
	./$(TARGET_PRIORITY)

# Executar a versão FIFO (compila se necessário)
run_fifo: $(TARGET_FIFO)
	./$(TARGET_FIFO)

.PHONY: all clean priority fifo run run_fifo