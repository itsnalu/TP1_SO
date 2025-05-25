#ifndef THREADS_H
#define THREADS_H

#include "processoSimulado.h"
#include "gerenciador.h"

// Estrutura para passar argumentos para a thread do processo
typedef struct {
    ProcessoSimulado_t* processo;
    GerenciadorDeProcessos_t* gerenciador;
} ThreadArgs_t;

// Funções de gerenciamento de threads
void inicializarThreads(GerenciadorDeProcessos_t* gerenciador);
void finalizarThreads(GerenciadorDeProcessos_t* gerenciador);
void* executarProcessoThread(void* arg);

#endif