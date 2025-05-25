#ifndef FILA_H
#define FILA_H

#include <stdlib.h>
#include <stdio.h>

#define FILA_TAMANHO_INICIAL_PADRAO 10

typedef struct {
    int *elementos;
    int capacidade;
    int tamanho;
    int inicio_fila;
    int fim_fila;
} FilaProcessos_t;

void filaInicializar(FilaProcessos_t *f);
int filaEstaVazia(const FilaProcessos_t *f);
void filaEnfileirar(FilaProcessos_t *f, int indice_processo);
int filaDesenfileirar(FilaProcessos_t *f);
void filaLiberarMemoria(FilaProcessos_t *f);
// void filaImprimir(const FilaProcessos_t *f); // Opcional para debug

#endif // FILA_H