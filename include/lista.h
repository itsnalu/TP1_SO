#ifndef LISTA_H
#define LISTA_H

#include "processoSimulado.h"

typedef struct Celula {
    ProcessoSimulado_t* processo;
    struct Celula* proximo;
} Celula;
typedef struct {
    Celula* inicio;
    Celula* fim;
} Lista;

// Funções básicas da lista
Lista* criaLista();
int listaVazia(Lista* lista);
void insereTabela(Lista* lista, ProcessoSimulado_t* processo);
ProcessoSimulado_t* buscaProcesso(Lista* lista, int PID);
void removeTabela(Lista* lista, int PID);
int maiorPIDTabela(Lista* lista);
void imprimeTabela(Lista* lista);
#endif