#ifndef ESTADOS_H
#define ESTADOS_H

#include "fila.h"
#include "config.h"

// Representa o conjunto de processos no estado PRONTO.
// Utiliza múltiplas filas, uma para cada nível de prioridade.
typedef struct {
    FilaProcessos_t filas_por_prioridade[NUM_NIVEIS_PRIORIDADE];
} EstadoPronto_t;

// Representa o conjunto de processos no estado BLOQUEADO.
// Utiliza uma única fila para todos os processos bloqueados.
typedef struct {
    FilaProcessos_t fila_geral_bloqueados;
} EstadoBloqueado_t;

// Funções para inicializar as estruturas de estado.
void estadosInicializarProntos(EstadoPronto_t *ep);
void estadosInicializarBloqueados(EstadoBloqueado_t *eb);

// Funções para liberar a memória alocada pelas filas nos estados.
void estadosLiberarProntos(EstadoPronto_t *ep);
void estadosLiberarBloqueados(EstadoBloqueado_t *eb);

// (Protótipos de funções de manipulação, como adicionar/remover processos,
// seriam implementadas em estados.c e usadas pelo Gerenciador)

#endif // ESTADOS_H