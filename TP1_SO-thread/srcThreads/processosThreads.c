#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../includeThreads/processosThreads.h"
#include "../includeThreads/estadosThread.h"

// Mutex global para sincronização
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Estrutura para controle de processos com threads
typedef struct {
    int pid;
    pthread_t thread;
    int prioridade;
    int estado;
    int tempo_cpu;
} ProcessoThread_t;

// Array para armazenar processos
static ProcessoThread_t processos[MAX_PROCESSOS];
static int num_processos = 0;

// Inicializa o sistema de processos
void processosThreadsInicializar(void) {
    pthread_mutex_lock(&mutex);
    num_processos = 0;
    for (int i = 0; i < MAX_PROCESSOS; i++) {
        processos[i].pid = -1;
        processos[i].estado = EST_NOVO_THREADS;
        processos[i].tempo_cpu = 0;
        processos[i].prioridade = 0;
    }
    pthread_mutex_unlock(&mutex);
}

// Cria um novo processo
int processosThreadsCriar(int prioridade) {
    pthread_mutex_lock(&mutex);
    
    if (num_processos >= MAX_PROCESSOS) {
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    int novo_pid = num_processos;
    processos[novo_pid].pid = novo_pid;
    processos[novo_pid].estado = EST_PRONTO_THREADS;
    processos[novo_pid].prioridade = prioridade;
    processos[novo_pid].tempo_cpu = 0;
    num_processos++;

    pthread_mutex_unlock(&mutex);
    return novo_pid;
}

// Finaliza um processo
void processosThreadsFinalizar(int pid) {
    pthread_mutex_lock(&mutex);
    
    if (pid >= 0 && pid < num_processos) {
        processos[pid].estado = EST_TERMINADO_THREADS;
    }
    
    pthread_mutex_unlock(&mutex);
}

// Obtém o estado de um processo
int processosThreadsObterEstado(int pid) {
    int estado;
    pthread_mutex_lock(&mutex);
    
    estado = (pid >= 0 && pid < num_processos) ? processos[pid].estado : -1;
    
    pthread_mutex_unlock(&mutex);
    return estado;
}

// Define o estado de um processo
void processosThreadsDefinirEstado(int pid, int novo_estado) {
    pthread_mutex_lock(&mutex);
    
    if (pid >= 0 && pid < num_processos) {
        processos[pid].estado = novo_estado;
    }
    
    pthread_mutex_unlock(&mutex);
}

// Obtém a prioridade de um processo
int processosThreadsObterPrioridade(int pid) {
    int prioridade;
    pthread_mutex_lock(&mutex);
    
    prioridade = (pid >= 0 && pid < num_processos) ? processos[pid].prioridade : -1;
    
    pthread_mutex_unlock(&mutex);
    return prioridade;
}

// Define a prioridade de um processo
void processosThreadsDefinirPrioridade(int pid, int nova_prioridade) {
    pthread_mutex_lock(&mutex);
    
    if (pid >= 0 && pid < num_processos) {
        processos[pid].prioridade = nova_prioridade;
    }
    
    pthread_mutex_unlock(&mutex);
}

// Incrementa o tempo de CPU de um processo
void processosThreadsIncrementarTempoCPU(int pid) {
    pthread_mutex_lock(&mutex);
    
    if (pid >= 0 && pid < num_processos) {
        processos[pid].tempo_cpu++;
    }
    
    pthread_mutex_unlock(&mutex);
}

// Obtém o tempo de CPU de um processo
int processosThreadsObterTempoCPU(int pid) {
    int tempo;
    pthread_mutex_lock(&mutex);
    
    tempo = (pid >= 0 && pid < num_processos) ? processos[pid].tempo_cpu : -1;
    
    pthread_mutex_unlock(&mutex);
    return tempo;
} 