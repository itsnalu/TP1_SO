#ifndef FILA_H
#define FILA_H

#include <stdlib.h> // Para NULL, malloc, free, exit
#include <stdio.h>  // Para perror

#define FILA_TAMANHO_INICIAL_PADRAO 10 // Tamanho inicial para alocação da fila

// Estrutura de uma fila dinâmica para armazenar índices de processos.
typedef struct {
    int *elementos;     // Array que armazena os elementos da fila
    int capacidade;     // Capacidade atual do array 'elementos'
    int tamanho;        // Número atual de elementos presentes na fila
    int inicio_fila;    // Índice do primeiro elemento (para desenfileirar)
    int fim_fila;       // Índice da próxima posição livre (para enfileirar, em fila circular)
} FilaProcessos_t;

// Inicializa uma FilaProcessos_t.
void filaInicializar(FilaProcessos_t *f);

// Verifica se a fila está vazia. Retorna 1 se vazia, 0 caso contrário.
int filaEstaVazia(const FilaProcessos_t *f);

// Adiciona um indice_processo ao final da fila. Redimensiona se necessário.
void filaEnfileirar(FilaProcessos_t *f, int indice_processo);

// Remove e retorna o indice_processo do início da fila.
// Retorna -1 (ou um valor de erro definido) se a fila estiver vazia.
int filaDesenfileirar(FilaProcessos_t *f);

// Libera a memória alocada para os elementos da fila.
void filaLiberarMemoria(FilaProcessos_t *f);

#endif // FILA_H