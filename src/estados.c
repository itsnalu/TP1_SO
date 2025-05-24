#include "estados.h"
#include "fila.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>

//inicializar as filas de processos prontos, uma para cada nivel de prioridade
void estadosInicializarProntos(EstadoPronto_t *ep){
    for(int i = 0; i < NUM_NIVEIS_PRIORIDADE; i++){
        filaInicializar(&ep->filas_por_prioridade[i]);
    }
    // Inicializa também a fila específica para FIFO
    filaInicializar(&ep->fila_fifo);
    printf("[DEBUG] Fila FIFO inicializada.\n"); // DEBUG
}
//inicializar as filas de processos bloqueados
void estadosInicializarBloqueados(EstadoBloqueado_t *eb){
    filaInicializar(&eb->fila_geral_bloqueados);
}
//Liberar memoria alocada para as filas de processos prontos
void estadosLiberarProntos(EstadoPronto_t *ep){
    for(int i = 0; i < NUM_NIVEIS_PRIORIDADE; i++){
        filaLiberarMemoria(&ep->filas_por_prioridade[i]);
    }
}
//Liberar memoria alocada para as filas de processos bloqueados
void estadosLiberarBloqueados(EstadoBloqueado_t *eb){
    filaLiberarMemoria(&eb->fila_geral_bloqueados);
}
//Adicionar um processo a fila de prontos
void estadosAdicionarPronto(EstadoPronto_t *ep, int pid, int prioridade) {
    // Verifica se a prioridade é válida
    // Prioridade deve estar entre 0 e NUM_NIVEIS_PRIORIDADE - 1
    // Se prioridade for negativa ou maior ou igual a NUM_NIVEIS_PRIORIDADE, imprime mensagem de erro
    if(prioridade < 0 || prioridade >= NUM_NIVEIS_PRIORIDADE){
        fprintf(stderr, "Prioridade invalida: %d\n", prioridade);
        return;
    }

    // Verifica se o processo já está na fila
    FilaProcessos_t *fila = &ep->filas_por_prioridade[prioridade];
    int idx = fila->inicio_fila;
    for (int i = 0; i < fila->tamanho; i++) {
        if (fila->elementos[idx] == pid) {
            printf("[DEBUG] Processo %d já está na fila de prontos (Prioridade: %d)\n", pid, prioridade);
            return;
        }
        idx = (idx + 1) % fila->capacidade;
    }

    // Adiciona o processo à fila de prontos correspondente à prioridade
    filaEnfileirar(&ep->filas_por_prioridade[prioridade], pid);
    printf("[DEBUG] Processo %d adicionado à fila de prontos (Prioridade: %d)\n", pid, prioridade);
}
//Remover um processo da fila de prontos
int estadosRemoverPronto(EstadoPronto_t *ep) {
    for (int i = 0; i < NUM_NIVEIS_PRIORIDADE; i++) {
        if (!filaEstaVazia(&ep->filas_por_prioridade[i])) {
            return filaDesenfileirar(&ep->filas_por_prioridade[i]);
        }
    }
    return -1; // Nenhum processo pronto disponível
}
// Função para adicionar processo à fila FIFO
void estadosAdicionarProntoFIFO(EstadoPronto_t *ep, int pid){
    FilaProcessos_t *fila = &ep->fila_fifo;
// Verifica se o processo já está na fila FIFO
    int idx = fila->inicio_fila;
    int i;
    for(i = 0; i < fila->tamanho; i++){
        if(fila->elementos[idx] == pid){
            printf("[DEBUG] Processo %d já está na fila FIFO\n", pid);
            return;
        }
        idx = (idx + 1) % fila->capacidade;
    }
    // Adiciona processo na fila FIFO
    filaEnfileirar(fila, pid);
    printf("[DEBUG] Processo %d adicionado à fila FIFO\n", pid);
}
// Remove o próximo processo da fila FIFO
int estadosRemoverProntoFIFO(EstadoPronto_t *ep) {
    if (filaEstaVazia(&ep->fila_fifo)) {
        return -1; // Fila vazia
    }
    return filaDesenfileirar(&ep->fila_fifo);
}
//Adicionar um processo a fila de bloqueados
void estadosAdicionarBloqueado(EstadoBloqueado_t *eb, int pid){
    // Adiciona o processo à fila de bloqueados
    filaEnfileirar(&eb->fila_geral_bloqueados, pid);
}
//Remover um processo da fila de bloqueados
int estadosRemoverBloqueado(EstadoBloqueado_t *eb) {
    if (filaEstaVazia(&eb->fila_geral_bloqueados)) {
        return -1; // Nenhum processo bloqueado disponível
    }
    return filaDesenfileirar(&eb->fila_geral_bloqueados);
}