#ifndef PROCESSO_SIMULADO_THREADS_H
#define PROCESSO_SIMULADO_THREADS_H

// Includes do sistema
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// Includes do projeto
#include "config.h"

// Forward declarations para evitar dependência circular
struct ProcessoSimulado_s;
typedef struct ProcessoSimulado_s ProcessoSimulado_t;

// Estrutura para controle de threads do processo simulado
typedef struct ProcessoSimuladoThread_s {
    ProcessoSimulado_t *processo;      // Ponteiro para o processo simulado
    pthread_t thread_id;               // ID da thread do processo
    pthread_mutex_t mutex;             // Mutex para sincronização
    sem_t semaforo;                    // Semáforo para controle de execução
    volatile int executando;           // Flag para indicar se está em execução
    volatile int quantum_restante;     // Quantum restante para execução
    volatile int deve_terminar;        // Flag para controle de término
} ProcessoSimuladoThread_t;

// Funções para gerenciamento de threads
ProcessoSimuladoThread_t* psThreadCriar(ProcessoSimulado_t *processo);
int psThreadInicializar(ProcessoSimuladoThread_t *processo_thread);
int psThreadExecutar(ProcessoSimuladoThread_t *processo_thread);
int psThreadPausar(ProcessoSimuladoThread_t *processo_thread);
int psThreadRetomar(ProcessoSimuladoThread_t *processo_thread);
int psThreadFinalizar(ProcessoSimuladoThread_t *processo_thread);
void psThreadDestruir(ProcessoSimuladoThread_t *processo_thread);

// Função principal de execução da thread
void* psThreadFuncaoExecucao(void *arg);

// Funções auxiliares para sincronização
void psThreadAtualizarQuantum(ProcessoSimuladoThread_t *processo_thread, int novo_quantum);
int psThreadQuantumEsgotado(const ProcessoSimuladoThread_t *processo_thread);
void psThreadAtualizarEstado(ProcessoSimuladoThread_t *processo_thread, int novo_estado);

// Funções de debug e logging
void psThreadLogEstado(const ProcessoSimuladoThread_t *processo_thread, const char *mensagem);

// Estrutura opaca para o processo simulado
typedef struct ProcessoSimulado ProcessoSimulado_t;

// Cria um novo processo simulado
ProcessoSimulado_t* processoSimuladoCriar(int tempo_execucao);

// Finaliza um processo simulado
void processoSimuladoFinalizar(ProcessoSimulado_t* processo);

#endif // PROCESSO_SIMULADO_THREADS_H 