#ifndef ESTADOS_H
#define ESTADOS_H

#include "fila.h" 
#include "config.h" // Para NUM_NIVEIS_PRIORIDADE

// Representa o conjunto de processos no estado PRONTO.
// Agora com múltiplas filas, uma para cada nível de prioridade.
typedef struct {
    FilaProcessos_t filas_prontos[NUM_NIVEIS_PRIORIDADE]; 
} EstadoPronto_t;

// Representa o conjunto de processos no estado BLOQUEADO.
// Mantém uma única fila geral para bloqueados.
typedef struct {
    FilaProcessos_t fila_geral_bloqueados; 
} EstadoBloqueado_t;

// Funções para inicializar as estruturas de estado.
void estadosInicializarProntos(EstadoPronto_t *ep);
void estadosInicializarBloqueados(EstadoBloqueado_t *eb);

// Funções para liberar a memória alocada pelas filas nos estados.
void estadosLiberarProntos(EstadoPronto_t *ep);
void estadosLiberarBloqueados(EstadoBloqueado_t *eb);

// Funções para manipulação de processos nos estados (thread-safe)
void estadosAdicionarPronto(EstadoPronto_t *ep, int pid, int prioridade);
int estadosRemoverPronto(EstadoPronto_t *ep); 
void estadosAdicionarBloqueado(EstadoBloqueado_t *eb, int pid);
int estadosRemoverBloqueadoEspecifico(EstadoBloqueado_t *eb, int pid_a_remover);

#endif // ESTADOS_H