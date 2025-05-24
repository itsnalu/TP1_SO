#include "../include/listaThreads.h"
#include <stdio.h>
#include <stdlib.h>

void listaThreadsInicializar(ListaThreads_t *lista) {
    pthread_mutex_init(&lista->mutex, NULL);
    pthread_mutex_lock(&lista->mutex);
    
    lista->primeiro = NULL;
    lista->ultimo = NULL;
    lista->tamanho = 0;
    
    pthread_mutex_unlock(&lista->mutex);
}

void listaThreadsInserir(ListaThreads_t *lista, void *item) {
    pthread_mutex_lock(&lista->mutex);
    
    CelulaListaThread_t *nova = (CelulaListaThread_t *)malloc(sizeof(CelulaListaThread_t));
    if (nova == NULL) {
        pthread_mutex_unlock(&lista->mutex);
        return;
    }
    
    nova->item = item;
    nova->prox = NULL;
    
    if (lista->primeiro == NULL) {
        lista->primeiro = nova;
    } else {
        lista->ultimo->prox = nova;
    }
    
    lista->ultimo = nova;
    lista->tamanho++;
    
    pthread_mutex_unlock(&lista->mutex);
}

void *listaThreadsRemover(ListaThreads_t *lista, int indice) {
    pthread_mutex_lock(&lista->mutex);
    
    if (indice < 0 || indice >= lista->tamanho || lista->primeiro == NULL) {
        pthread_mutex_unlock(&lista->mutex);
        return NULL;
    }
    
    CelulaListaThread_t *atual = lista->primeiro;
    CelulaListaThread_t *anterior = NULL;
    void *item = NULL;
    
    // Caso especial: remover o primeiro elemento
    if (indice == 0) {
        item = atual->item;
        lista->primeiro = atual->prox;
        if (lista->primeiro == NULL) {
            lista->ultimo = NULL;
        }
        free(atual);
        lista->tamanho--;
        pthread_mutex_unlock(&lista->mutex);
        return item;
    }
    
    // Navega até o elemento a ser removido
    for (int i = 0; i < indice && atual != NULL; i++) {
        anterior = atual;
        atual = atual->prox;
    }
    
    if (atual != NULL) {
        item = atual->item;
        anterior->prox = atual->prox;
        if (atual == lista->ultimo) {
            lista->ultimo = anterior;
        }
        free(atual);
        lista->tamanho--;
    }
    
    pthread_mutex_unlock(&lista->mutex);
    return item;
}

void *listaThreadsObter(ListaThreads_t *lista, int indice) {
    pthread_mutex_lock(&lista->mutex);
    
    if (indice < 0 || indice >= lista->tamanho || lista->primeiro == NULL) {
        pthread_mutex_unlock(&lista->mutex);
        return NULL;
    }
    
    CelulaListaThread_t *atual = lista->primeiro;
    for (int i = 0; i < indice && atual != NULL; i++) {
        atual = atual->prox;
    }
    
    void *item = (atual != NULL) ? atual->item : NULL;
    
    pthread_mutex_unlock(&lista->mutex);
    return item;
}

int listaThreadsTamanho(ListaThreads_t *lista) {
    pthread_mutex_lock(&lista->mutex);
    int tamanho = lista->tamanho;
    pthread_mutex_unlock(&lista->mutex);
    return tamanho;
}

int listaThreadsEstaVazia(ListaThreads_t *lista) {
    pthread_mutex_lock(&lista->mutex);
    int vazia = (lista->tamanho == 0);
    pthread_mutex_unlock(&lista->mutex);
    return vazia;
}

void listaThreadsLimpar(ListaThreads_t *lista) {
    pthread_mutex_lock(&lista->mutex);
    
    CelulaListaThread_t *atual = lista->primeiro;
    while (atual != NULL) {
        CelulaListaThread_t *temp = atual;
        atual = atual->prox;
        free(temp);
    }
    
    lista->primeiro = NULL;
    lista->ultimo = NULL;
    lista->tamanho = 0;
    
    pthread_mutex_unlock(&lista->mutex);
}

void listaThreadsFinalizar(ListaThreads_t *lista) {
    listaThreadsLimpar(lista);
    pthread_mutex_destroy(&lista->mutex);
} 