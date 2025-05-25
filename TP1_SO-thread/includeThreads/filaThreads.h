#ifndef FILA_THREADS_H
#define FILA_THREADS_H

// Includes do sistema
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_FILA_THREADS 100

// Estrutura da fila thread-safe
typedef struct {
    int elementos[MAX_FILA_THREADS];   // Array de PIDs
    volatile int inicio_fila;          // Índice do início da fila
    volatile int fim_fila;            // Índice do fim da fila
    volatile int tamanho;             // Número atual de elementos
    volatile int capacidade;          // Capacidade máxima da fila
    pthread_mutex_t mutex;            // Mutex para sincronização
} FilaThreads_t;

// Funções de gerenciamento da fila
void filaThreadsInicializar(FilaThreads_t *fila);
void filaThreadsEnfileirar(FilaThreads_t *fila, int pid);
int filaThreadsDesenfileirar(FilaThreads_t *fila);
int filaThreadsEstaVazia(const FilaThreads_t *fila);
void filaThreadsLiberarMemoria(FilaThreads_t *fila);

// Funções auxiliares
void filaThreadsRemoverElemento(FilaThreads_t *fila, int pid);
int filaThreadsContemElemento(const FilaThreads_t *fila, int pid);
int filaThreadsObterTamanho(const FilaThreads_t *fila);

#endif // FILA_THREADS_H 