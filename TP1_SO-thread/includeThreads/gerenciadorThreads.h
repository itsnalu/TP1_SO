#ifndef GERENCIADOR_THREADS_H
#define GERENCIADOR_THREADS_H

// Includes do sistema
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

// Forward declarations
struct ProcessoSimuladoThread_s;
typedef struct ProcessoSimuladoThread_s ProcessoSimuladoThread_t;

// Includes do projeto
#include "filaThreads.h"
#include "cpuThread.h"
#include "processoSimulado.h"
#include "processoSimuladoThreads.h"

#define MAX_THREADS 100

// Estrutura para controle de thread de processo
typedef struct {
    ProcessoSimulado_t *processo;
    pthread_t thread;
    volatile int em_execucao;
} ThreadProcesso_t;

// Estrutura principal do gerenciador de threads
typedef struct {
    ThreadProcesso_t threads[MAX_THREADS];
    volatile int num_threads;
    volatile int executando;
    int pipe_fd;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    FilaThreads_t fila_prontos;
    FilaThreads_t fila_bloqueados;
    CPUThread_t cpu_principal;
    CPUThread_t cpu_secundaria;
    volatile int tempo_simulacao_global;
} GerenciadorThreads_t;

// Estrutura opaca do gerenciador
//typedef struct GerenciadorThreads GerenciadorThreads_t;

// Funções exportadas
GerenciadorThreads_t* gerenciadorThreadsInicializar(void);
void gerenciadorThreadsFinalizar(GerenciadorThreads_t* gerenciador);
void gerenciadorThreadsEscalonar(void);
void gerenciadorThreadsTrocarContexto(void);

// Inicializa o gerenciador de threads
GerenciadorThreads_t* gerenciadorThreadsInicializar(void);

// Finaliza o gerenciador de threads
void gerenciadorThreadsFinalizar(GerenciadorThreads_t* gerenciador);

// Adiciona um novo processo ao gerenciador
int gerenciadorThreadsAdicionarProcesso(GerenciadorThreads_t* gerenciador, ProcessoSimuladoThread_t* processo);

// Imprime o estado atual do gerenciador
void gerenciadorThreadsImprimirEstado(GerenciadorThreads_t* gerenciador);

#endif // GERENCIADOR_THREADS_H