/* 1.1 Implementar/Completar em include/fila.h e src/fila.c

    Definir a estrutura FilaProcessos_t para armazenar PIDs
    Implementar funções para inicializar filas
    Implementar funções para adicionar processos às filas
    Implementar funções para remover processos das filas
    Implementar funções para verificar se uma fila está vazia */

#include "fila.h"
// Definindo o tamanho inicial padrão da fila
void filaInicializar(FilaProcessos_t *f){
    f->capacidade= FILA_TAMANHO_INICIAL_PADRAO;
    f->elementos= (int*)malloc(f->capacidade*sizeof(int));
    if(f->elementos == NULL){
        perror("Erro ao alocar memória para a fila.\n");
    }
    f->tamanho=0;
    f->inicio_fila=0;
    f->fim_fila=0;
}

// A fila está vazia se o tamanho for 0
// Retorna 1 se a fila estiver vazia, 0 caso contrário
int filaEstaVazia(const FilaProcessos_t *f){
    return f->tamanho == 0;
}
// Função enfileirar um processo na fila

void filaEnfileirar(FilaProcessos_t *f, int indice_processo) {
    // Verifica se a fila está cheia e aumenta a capacidade se necessário
    if (f->tamanho == f->capacidade) {
        int nova_capacidade = f->capacidade * 2;
        int *novos_elementos = (int*)realloc(f->elementos, nova_capacidade * sizeof(int));
        
        if (novos_elementos == NULL) {
            perror("Erro ao redimensionar a fila");
            return;
        }
        
        // Se o início não está no começo do array, reorganiza os elementos
        if (f->inicio_fila > 0) {
            // Copia elementos do início até o fim do array
            if (f->inicio_fila + f->tamanho > f->capacidade) {
                // A fila dá a volta (circular)
                int elementos_final = f->capacidade - f->inicio_fila;
                
                // Move os elementos do início para o final do novo array
                for (int i = 0; i < elementos_final; i++) {
                    novos_elementos[i + f->capacidade] = novos_elementos[f->inicio_fila + i];
                }
                
                // Ajusta o início da fila
                f->inicio_fila = f->capacidade;
            }
        }
        
        f->elementos = novos_elementos;
        f->capacidade = nova_capacidade;
    }
    
    // Adiciona o elemento na fila
    f->elementos[f->fim_fila] = indice_processo;
    f->fim_fila = (f->fim_fila + 1) % f->capacidade;
    f->tamanho++;
}

//Função para desenfileirar um processo da fila
// Retorna o PID do processo removido ou -1 se a fila estiver vazia
int filaDesenfileirar(FilaProcessos_t *f){
    if(filaEstaVazia(f)){
        return -1; // Fila vazia
    }
    int pid = f->elementos[f->inicio_fila];
    f->inicio_fila = (f->inicio_fila + 1) % f->capacidade;
    f->tamanho--;
    return pid;
}
// Função para liberar a memória alocada para a fila
// Libera a memória alocada para os elementos da fila
// Reseta os atributos da fila
// A fila deve ser liberada após o uso
void filaLiberarMemoria(FilaProcessos_t *f){
    free(f->elementos);
    f->elementos = NULL;
    f->capacidade = 0;
    f->tamanho = 0;
    f->inicio_fila = 0;
    f->fim_fila = 0;
}
void filaImprimir(const FilaProcessos_t *f) {
    if (filaEstaVazia(f)) {
        printf("Fila vazia\n");
        return;
    }
    
    printf("Conteúdo da fila (tamanho %d):\n", f->tamanho);
    int idx = f->inicio_fila;
    for (int i = 0; i < f->tamanho; i++) {
        printf("  [%d] PID: %d\n", i, f->elementos[idx]);
        idx = (idx + 1) % f->capacidade;
    }
}
