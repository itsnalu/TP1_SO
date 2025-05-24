#include "../include/processosThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void processosThreadInicializar(ProcessoThread_t *processo) {
    pthread_mutex_init(&processo->mutex, NULL);
    pthread_mutex_lock(&processo->mutex);
    
    processo->pid = -1;
    processo->prioridade = 0;
    processo->tempo_inicio = time(NULL);
    processo->tempo_cpu = 0;
    processo->tempo_bloqueio = 0;
    
    pthread_mutex_unlock(&processo->mutex);
}

void processosThreadAtualizar(ProcessoThread_t *processo) {
    pthread_mutex_lock(&processo->mutex);
    processo->tempo_cpu++;
    pthread_mutex_unlock(&processo->mutex);
}

void processosThreadFinalizar(ProcessoThread_t *processo) {
    pthread_mutex_lock(&processo->mutex);
    processo->tempo_cpu = 0;
    processo->tempo_bloqueio = 0;
    pthread_mutex_unlock(&processo->mutex);
    pthread_mutex_destroy(&processo->mutex);
}

void processosThreadBloquear(ProcessoThread_t *processo, int tempo_bloqueio) {
    pthread_mutex_lock(&processo->mutex);
    processo->tempo_bloqueio = tempo_bloqueio;
    pthread_mutex_unlock(&processo->mutex);
}

void processosThreadDesbloquear(ProcessoThread_t *processo) {
    pthread_mutex_lock(&processo->mutex);
    processo->tempo_bloqueio = 0;
    pthread_mutex_unlock(&processo->mutex);
} 