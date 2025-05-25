#include "../includeThreads/estadosThread.h"
#include <stdio.h>
#include <stdlib.h>

void estadosThreadInicializar(EstadosThread_t *estados) {
    pthread_mutex_init(&estados->mutex, NULL);
    filaThreadsInicializar(&estados->fila_geral_prontos);
    filaThreadsInicializar(&estados->fila_geral_bloqueados);
}

void estadosThreadAdicionarPronto(EstadosThread_t *estados, int pid, int prioridade) {
    pthread_mutex_lock(&estados->mutex);
    filaThreadsEnfileirar(&estados->fila_geral_prontos, pid);
    pthread_mutex_unlock(&estados->mutex);
    printf("[EstadosThread] PID %d adicionado à fila de prontos (Prioridade: %d)\n", pid, prioridade);
}

void estadosThreadAdicionarBloqueado(EstadosThread_t *estados, int pid) {
    pthread_mutex_lock(&estados->mutex);
    filaThreadsEnfileirar(&estados->fila_geral_bloqueados, pid);
    pthread_mutex_unlock(&estados->mutex);
    printf("[EstadosThread] PID %d adicionado à fila de bloqueados\n", pid);
}

int estadosThreadRemoverPronto(EstadosThread_t *estados) {
    pthread_mutex_lock(&estados->mutex);
    int pid = filaThreadsDesenfileirar(&estados->fila_geral_prontos);
    pthread_mutex_unlock(&estados->mutex);
    if (pid != -1) {
        printf("[EstadosThread] PID %d removido da fila de prontos\n", pid);
    }
    return pid;
}

int estadosThreadRemoverBloqueado(EstadosThread_t *estados) {
    pthread_mutex_lock(&estados->mutex);
    int pid = filaThreadsDesenfileirar(&estados->fila_geral_bloqueados);
    pthread_mutex_unlock(&estados->mutex);
    if (pid != -1) {
        printf("[EstadosThread] PID %d removido da fila de bloqueados\n", pid);
    }
    return pid;
}

void estadosThreadFinalizar(EstadosThread_t *estados) {
    pthread_mutex_lock(&estados->mutex);
    filaThreadsLiberarMemoria(&estados->fila_geral_prontos);
    filaThreadsLiberarMemoria(&estados->fila_geral_bloqueados);
    pthread_mutex_unlock(&estados->mutex);
    pthread_mutex_destroy(&estados->mutex);
} 