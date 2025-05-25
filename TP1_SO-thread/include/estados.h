#ifndef ESTADOS_H
#define ESTADOS_H
#include "fila.h"
#include "config.h"
// Representa o conjunto de processos no estado PRONTO.
// Utiliza múltiplas filas, uma para cada nível de prioridade.
typedef struct {
    FilaProcessos_t filas_por_prioridade[NUM_NIVEIS_PRIORIDADE];
    FilaProcessos_t fila_fifo; // para escalonador FIFO
} EstadoPronto_t;
// Representa o conjunto de processos no estado BLOQUEADO.
// Utiliza uma única fila para todos os processos bloqueados.
typedef struct {
    FilaProcessos_t fila_geral_bloqueados;
} EstadoBloqueado_t;
//Estado de execução de um processo
//typedef struct{
//} EstadoExecucao_t;
// Funções para inicializar as estruturas de estado.
void estadosInicializarProntos(EstadoPronto_t *ep);
void estadosInicializarBloqueados(EstadoBloqueado_t *eb);
//void estadosInicializarExecucao(EstadoExecucao_t *ee);
// Funções para liberar a memória alocada pelas filas nos estados.
void estadosLiberarProntos(EstadoPronto_t *ep);
void estadosLiberarBloqueados(EstadoBloqueado_t *eb);
//void estadosLiberarExecucao(EstadoExecucao_t *ee);
 // Funções para manipulação de procesos nos estados
void estadosAdicionarPronto(EstadoPronto_t *ep, int pid, int prioridade);
int estadosRemoverPronto(EstadoPronto_t *ep);
void estadosAdicionarBloqueado(EstadoBloqueado_t *eb, int pid);
int estadosRemoverBloqueado(EstadoBloqueado_t *eb);
//Funções escalonamento FIFO
void estadosAdicionarProntoFIFO(EstadoPronto_t *ep, int pid);
int estadosRemoverProntoFIFO(EstadoPronto_t *ep);
#endif // ESTADOS_H