#ifndef PROCESSOS_THREAD_H
#define PROCESSOS_THREAD_H

#include <pthread.h>
#include "config.h"

typedef struct {
    int pid;
    int prioridade;
    int tempo_inicio;
    int tempo_cpu;
    int tempo_bloqueio;
    pthread_mutex_t mutex;
} ProcessoThread_t;

void processosThreadInicializar(ProcessoThread_t *processo);
void processosThreadAtualizar(ProcessoThread_t *processo);
void processosThreadFinalizar(ProcessoThread_t *processo);
void processosThreadBloquear(ProcessoThread_t *processo, int tempo_bloqueio);
void processosThreadDesbloquear(ProcessoThread_t *processo);

#endif // PROCESSOS_THREAD_H 