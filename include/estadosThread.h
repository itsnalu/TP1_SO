#ifndef ESTADOS_THREAD_H
#define ESTADOS_THREAD_H

// Includes do sistema
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// Includes do projeto
#include "filaThreads.h"

// Estrutura para gerenciamento de estados
typedef struct {
    FilaThreads_t fila_geral_prontos;     // Fila de processos prontos
    FilaThreads_t fila_geral_bloqueados;  // Fila de processos bloqueados
    pthread_mutex_t mutex;                 // Mutex para sincronização
} EstadosThread_t;

// Funções de gerenciamento de estados
void estadosThreadInicializar(EstadosThread_t *estados);
void estadosThreadFinalizar(EstadosThread_t *estados);

// Funções de manipulação de processos prontos
void estadosThreadAdicionarPronto(EstadosThread_t *estados, int pid, int prioridade);
int estadosThreadRemoverPronto(EstadosThread_t *estados);
int estadosThreadTemProcessoPronto(const EstadosThread_t *estados);

// Funções de manipulação de processos bloqueados
void estadosThreadAdicionarBloqueado(EstadosThread_t *estados, int pid);
int estadosThreadRemoverBloqueado(EstadosThread_t *estados);
int estadosThreadTemProcessoBloqueado(const EstadosThread_t *estados);

// Funções auxiliares
void estadosThreadAtualizarBloqueados(EstadosThread_t *estados);
void estadosThreadImprimirEstado(const EstadosThread_t *estados);

#endif // ESTADOS_THREAD_H 