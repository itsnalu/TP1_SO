#ifndef THREADS_H
#define THREADS_H

#include "gerenciador.h"
#include "processoSimulado.h"

// Estrutura para comunicação entre threads
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int comando_recebido;
} ThreadComunicacao_t;

// Estrutura para passar argumentos para a thread do processo
typedef struct {
    ProcessoSimulado_t* processo;
    GerenciadorDeProcessos_t* gerenciador;
} ThreadArgs_t;

// Funções de gerenciamento de threads
void inicializarThreads(GerenciadorDeProcessos_t* gerenciador);
void finalizarThreads(GerenciadorDeProcessos_t* gerenciador);
void* executarProcessoThread(void* arg);
void* executarImpressaoThread(void* arg);

// Funções de escalonamento com threads
void escalonarProcessosThreads(GerenciadorDeProcessos_t* gerenciador);
void executarUnidadeTempo(GerenciadorDeProcessos_t* gerenciador);

#endif