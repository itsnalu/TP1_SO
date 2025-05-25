# Compilador C
CC = gcc

# Opções de compilação:
# -Wall: Habilita a maioria dos avisos comuns
# -Iinclude: Diz ao compilador para procurar por arquivos de cabeçalho no diretório 'include'
# -pthread: Habilita o suporte a POSIX threads na compilação e linkagem
# -g: Adiciona símbolos de debug
CFLAGS = -Wall -g -Iinclude -pthread

# Opções de linkagem:
# -lm: Linka com a biblioteca matemática (se usada)
# -pthread: Linka com a biblioteca POSIX threads
LDFLAGS = -lm -pthread

# Lista de todos os arquivos fonte .c que compõem o seu projeto
SRCS = src/main.c \
       src/gerenciador.c \
       src/processoSimulado.c \
       src/processoImpressao.c \
       src/estados.c \
       src/fila.c \
       src/cpu.c

# Nome do executável final
TARGET = simulador_so_threads

# Alvo padrão: compila o programa principal
all: $(TARGET)

# Regra para criar o executável principal
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Simulador '$(TARGET)' compilado com sucesso."

# Alvo para limpar arquivos compilados (objetos e o executável)
clean:
	rm -f $(TARGET) src/*.o *.o
	@echo "Arquivos compilados removidos."

# Alvo para executar o programa (compila se necessário)
run: $(TARGET)
	./$(TARGET)

# Declara que 'all', 'clean', e 'run' não são nomes de arquivos
.PHONY: all clean run