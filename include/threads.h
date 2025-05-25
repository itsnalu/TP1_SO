#ifndef THREADS_H
#define THREADS_H

#include <pthread.h>

// Estrutura para comunicação entre threads
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int comando_recebido;
} ThreadComunicacao_t;

#endif