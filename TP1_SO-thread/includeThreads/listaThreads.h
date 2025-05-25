#ifndef LISTA_THREADS_H
#define LISTA_THREADS_H

#include <pthread.h>

// Estrutura para célula da lista
typedef struct CelulaListaThread {
    void *item;
    struct CelulaListaThread *prox;
} CelulaListaThread_t;

// Estrutura principal da lista
typedef struct {
    CelulaListaThread_t *primeiro;
    CelulaListaThread_t *ultimo;
    int tamanho;
    pthread_mutex_t mutex;
} ListaThreads_t;

// Funções da lista
void listaThreadsInicializar(ListaThreads_t *lista);
void listaThreadsInserir(ListaThreads_t *lista, void *item);
void *listaThreadsRemover(ListaThreads_t *lista, int indice);
void *listaThreadsObter(ListaThreads_t *lista, int indice);
int listaThreadsTamanho(ListaThreads_t *lista);
int listaThreadsEstaVazia(ListaThreads_t *lista);
void listaThreadsLimpar(ListaThreads_t *lista);
void listaThreadsFinalizar(ListaThreads_t *lista);

#endif // LISTA_THREADS_H 