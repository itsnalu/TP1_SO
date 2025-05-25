#ifndef ESTADOS_H
#define ESTADOS_H

#include "fila.h"   // Para a definição de FilaProcessos_t
#include "config.h" // Para NUM_NIVEIS_PRIORIDADE (e implicitamente para declarações de mutexes globais)

// Representa o conjunto de processos no estado PRONTO.
// Utiliza múltiplas filas, uma para cada nível de prioridade, para o escalonador MLFQ.
typedef struct {
    FilaProcessos_t filas_por_prioridade[NUM_NIVEIS_PRIORIDADE];
    // FilaProcessos_t fila_fifo; // Removida, assumindo foco no MLFQ para o modelo de threads.
                                  // Se a funcionalidade FIFO for mantida, este campo e suas funções devem ser readicionados.
} EstadoPronto_t;

// Representa o conjunto de processos no estado BLOQUEADO.
// Utiliza uma única fila para todos os processos bloqueados.
typedef struct {
    FilaProcessos_t fila_geral_bloqueados; 
} EstadoBloqueado_t;

// --- Funções de Inicialização e Liberação de Memória para os Estados ---

// Inicializa as filas de processos prontos (uma para cada nível de prioridade).
void estadosInicializarProntos(EstadoPronto_t *ep);

// Inicializa a fila de processos bloqueados.
void estadosInicializarBloqueados(EstadoBloqueado_t *eb);

// Libera a memória alocada para as filas de processos prontos.
void estadosLiberarProntos(EstadoPronto_t *ep);

// Libera a memória alocada para a fila de processos bloqueados.
void estadosLiberarBloqueados(EstadoBloqueado_t *eb);


// --- Funções de Manipulação de Processos nos Estados (devem ser implementadas como thread-safe em estados.c) ---

// Adiciona um PID à fila de prontos correspondente à sua prioridade.
void estadosAdicionarPronto(EstadoPronto_t *ep, int pid, int prioridade);

// Remove e retorna o PID do processo da fila de maior prioridade não vazia.
// Retorna -1 se todas as filas de prontos estiverem vazias.
int estadosRemoverPronto(EstadoPronto_t *ep); 

// Adiciona um PID à fila de processos bloqueados.
void estadosAdicionarBloqueado(EstadoBloqueado_t *eb, int pid);

// Remove e retorna o PID do processo do início da fila de bloqueados.
// Retorna -1 se a fila estiver vazia. (Mantido da "Ana", precisará ser thread-safe).
int estadosRemoverBloqueado(EstadoBloqueado_t *eb);

// Remove um PID específico da fila de bloqueados (necessário para despertar processos).
// Retorna 1 se removido com sucesso, 0 caso contrário.
int estadosRemoverBloqueadoEspecifico(EstadoBloqueado_t *eb, int pid_a_remover);


// Funções relacionadas à fila FIFO foram removidas, acompanhando a remoção da fila_fifo da struct EstadoPronto_t.
// Se a funcionalidade FIFO for necessária, os protótipos abaixo devem ser readicionados:
// void estadosAdicionarProntoFIFO(EstadoPronto_t *ep, int pid);
// int estadosRemoverProntoFIFO(EstadoPronto_t *ep);

#endif // ESTADOS_H