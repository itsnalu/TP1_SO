#include "../includeThreads/filaThreads.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define FILA_TAMANHO_INICIAL_PADRAO 10

void filaThreadsInicializar(FilaThreads_t *fila) {
    fila->inicio_fila = 0;
    fila->fim_fila = -1;
    fila->tamanho = 0;
    fila->capacidade = MAX_FILA_THREADS;
    pthread_mutex_init(&fila->mutex, NULL);
}

void filaThreadsEnfileirar(FilaThreads_t *fila, int pid) {
    pthread_mutex_lock(&fila->mutex);
    if (fila->tamanho < fila->capacidade) {
        fila->fim_fila = (fila->fim_fila + 1) % fila->capacidade;
        fila->elementos[fila->fim_fila] = pid;
        fila->tamanho++;
        printf("[FilaThreads] PID %d enfileirado\n", pid);
    }
    pthread_mutex_unlock(&fila->mutex);
}

int filaThreadsDesenfileirar(FilaThreads_t *fila) {
    pthread_mutex_lock(&fila->mutex);
    if (fila->tamanho == 0) {
        pthread_mutex_unlock(&fila->mutex);
        return -1;
    }
    
    int pid = fila->elementos[fila->inicio_fila];
    fila->inicio_fila = (fila->inicio_fila + 1) % fila->capacidade;
    fila->tamanho--;
    printf("[FilaThreads] PID %d desenfileirado\n", pid);
    
    pthread_mutex_unlock(&fila->mutex);
    return pid;
}

int filaThreadsEstaVazia(FilaThreads_t *fila) {
    pthread_mutex_lock(&fila->mutex);
    int vazia = (fila->tamanho == 0);
    pthread_mutex_unlock(&fila->mutex);
    return vazia;
}

void filaThreadsRemoverElemento(FilaThreads_t *fila, int pid) {
    pthread_mutex_lock(&fila->mutex);
    
    int i = fila->inicio_fila;
    int encontrado = 0;
    int nova_fila[MAX_FILA_THREADS];
    int novo_tamanho = 0;
    
    for (int count = 0; count < fila->tamanho; count++) {
        if (fila->elementos[i] != pid) {
            nova_fila[novo_tamanho++] = fila->elementos[i];
        } else {
            encontrado = 1;
        }
        i = (i + 1) % fila->capacidade;
    }
    
    if (encontrado) {
        fila->tamanho = novo_tamanho;
        fila->inicio_fila = 0;
        fila->fim_fila = novo_tamanho - 1;
        
        for (i = 0; i < novo_tamanho; i++) {
            fila->elementos[i] = nova_fila[i];
        }
        printf("[FilaThreads] PID %d removido da fila\n", pid);
    }
    
    pthread_mutex_unlock(&fila->mutex);
}

void filaThreadsLiberarMemoria(FilaThreads_t *fila) {
    pthread_mutex_destroy(&fila->mutex);
} 