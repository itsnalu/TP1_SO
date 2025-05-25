#include "fila.h" //

// Definindo o tamanho inicial padrão da fila
void filaInicializar(FilaProcessos_t *f){
    f->capacidade= FILA_TAMANHO_INICIAL_PADRAO; //
    f->elementos= (int*)malloc(f->capacidade*sizeof(int)); //
    if(f->elementos == NULL){
        perror("Erro ao alocar memória para a fila.\n"); //
        exit(EXIT_FAILURE);
    }
    f->tamanho=0; //
    f->inicio_fila=0; //
    f->fim_fila=0; //
}

int filaEstaVazia(const FilaProcessos_t *f){
    return f->tamanho == 0; //
}

void filaEnfileirar(FilaProcessos_t *f, int indice_processo) {
    if (f->tamanho == f->capacidade) {
        int nova_capacidade = (f->capacidade == 0) ? FILA_TAMANHO_INICIAL_PADRAO : f->capacidade * 2;
        int *novos_elementos = (int*)realloc(f->elementos, nova_capacidade * sizeof(int));
        
        if (novos_elementos == NULL) {
            perror("Erro ao redimensionar a fila"); //
            // Consider not exiting here, but returning an error or handling it
            return;
        }
        f->elementos = novos_elementos;

        // Reorganizar elementos se a fila deu a volta (wrap-around)
        // e o início não está em 0.
        if (f->inicio_fila != 0 && f->tamanho != 0) { // Apenas se necessário
            if (f->fim_fila <= f->inicio_fila) { // Wrap-around
                // Elementos do início_fila até capacidade-1 são movidos para o final do array antigo
                // Elementos de 0 até fim_fila-1 estão no começo do array antigo
                // Esta parte precisa ser cuidadosa para realloc e circular buffer.
                // Simplificação: se está cheio e wrap around, realinhar para que inicio_fila seja 0
                int *temp_buffer = (int*)malloc(f->tamanho * sizeof(int));
                if(!temp_buffer) { perror("Malloc failed for fila realign"); return; }
                int current;
                for(current = 0; current < f->tamanho; ++current) {
                    temp_buffer[current] = f->elementos[(f->inicio_fila + current) % f->capacidade];
                }
                for(current = 0; current < f->tamanho; ++current) {
                    f->elementos[current] = temp_buffer[current];
                }
                free(temp_buffer);
                f->inicio_fila = 0;
                f->fim_fila = f->tamanho;
            }
        }
        f->capacidade = nova_capacidade;
    }
    
    f->elementos[f->fim_fila] = indice_processo; //
    f->fim_fila = (f->fim_fila + 1) % f->capacidade; //
    f->tamanho++; //
}

int filaDesenfileirar(FilaProcessos_t *f){
    if(filaEstaVazia(f)){ //
        return -1; 
    }
    int pid = f->elementos[f->inicio_fila]; //
    f->inicio_fila = (f->inicio_fila + 1) % f->capacidade; //
    f->tamanho--; //
    return pid; //
}

void filaLiberarMemoria(FilaProcessos_t *f){
    if (f->elementos != NULL) {
        free(f->elementos); //
    }
    f->elementos = NULL; //
    f->capacidade = 0; //
    f->tamanho = 0; //
    f->inicio_fila = 0; //
    f->fim_fila = 0; //
}

