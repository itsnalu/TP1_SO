#include "estados.h"
#include "config.h"  // Para NUM_NIVEIS_PRIORIDADE e declarações extern dos mutexes
#include <stdio.h>   // Para fprintf em caso de erro
#include <pthread.h> // Para mutexes (já incluído via config.h, mas explícito para clareza)

void estadosInicializarProntos(EstadoPronto_t *ep) {
    if (!ep) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de inicializar EstadoPronto_t nulo.\n");
        return;
    }
    for (int i = 0; i < NUM_NIVEIS_PRIORIDADE; i++) {
        filaInicializar(&ep->filas_prontos[i]);
    }
    // printf("DEBUG ESTADOS: Filas de prontos (MLFQ) inicializadas.\n");
}

void estadosLiberarProntos(EstadoPronto_t *ep) {
    if (!ep) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de liberar EstadoPronto_t nulo.\n");
        return;
    }
    for (int i = 0; i < NUM_NIVEIS_PRIORIDADE; i++) {
        filaLiberarMemoria(&ep->filas_prontos[i]);
    }
    // printf("DEBUG ESTADOS: Memória das filas de prontos (MLFQ) liberada.\n");
}

// Adiciona pid à fila de prontos do nível de prioridade especificado.
void estadosAdicionarPronto(EstadoPronto_t *ep, int pid, int prioridade) {
    if (!ep) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de adicionar PID %d a EstadoPronto_t nulo.\n", pid);
        return;
    }

    if (prioridade < 0 || prioridade >= NUM_NIVEIS_PRIORIDADE) {
        fprintf(stderr, "ESTADOS AVISO: PID %d com prioridade inválida %d. Ajustando para prioridade %d (mais baixa).\n", 
                pid, prioridade, NUM_NIVEIS_PRIORIDADE - 1);
        prioridade = NUM_NIVEIS_PRIORIDADE - 1; // Fallback para a fila de menor prioridade
    }

    pthread_mutex_lock(&prontos_mutex);
    
    // Verificar se já está na fila de destino para evitar duplicatas exatas na MESMA fila.
    FilaProcessos_t *fila_destino = &ep->filas_prontos[prioridade];
    int encontrado_na_mesma_fila = 0;
    if (fila_destino->tamanho > 0) {
        int idx = fila_destino->inicio_fila;
        for (int i = 0; i < fila_destino->tamanho; i++) {
            if (fila_destino->elementos[idx] == pid) {
                encontrado_na_mesma_fila = 1;
                break;
            }
            idx = (idx + 1) % fila_destino->capacidade;
        }
    }

    if (!encontrado_na_mesma_fila) {
        filaEnfileirar(&ep->filas_prontos[prioridade], pid);
        // printf("DEBUG ESTADOS: PID %d adicionado à fila de prontos (Prio %d).\n", pid, prioridade);
    } else {
        // printf("DEBUG ESTADOS: PID %d já presente na fila de prontos (Prio %d).\n", pid, prioridade);
    }
    
    pthread_mutex_unlock(&prontos_mutex);
}

// Remove e retorna o PID do processo da fila de maior prioridade não vazia.
// Retorna -1 se todas as filas de prontos estiverem vazias.
int estadosRemoverPronto(EstadoPronto_t *ep) {
    if (!ep) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de remover de EstadoPronto_t nulo.\n");
        return -1;
    }

    pthread_mutex_lock(&prontos_mutex);
    int pid_removido = -1;
    for (int i = 0; i < NUM_NIVEIS_PRIORIDADE; i++) { // Itera da maior prioridade (0) para a menor
        if (!filaEstaVazia(&ep->filas_prontos[i])) {
            pid_removido = filaDesenfileirar(&ep->filas_prontos[i]);
            // printf("DEBUG ESTADOS: PID %d removido da fila de prontos (Prio %d).\n", pid_removido, i);
            break; // Encontrou e removeu, sai do loop
        }
    }
    pthread_mutex_unlock(&prontos_mutex);
    return pid_removido;
}


// --- Funções para Estado BLOQUEADO (fila única) ---

void estadosInicializarBloqueados(EstadoBloqueado_t *eb) {
    if (!eb) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de inicializar EstadoBloqueado_t nulo.\n");
        return;
    }
    filaInicializar(&eb->fila_geral_bloqueados);
    // printf("DEBUG ESTADOS: Fila de bloqueados inicializada.\n");
}

void estadosLiberarBloqueados(EstadoBloqueado_t *eb) {
    if (!eb) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de liberar EstadoBloqueado_t nulo.\n");
        return;
    }
    filaLiberarMemoria(&eb->fila_geral_bloqueados);
    // printf("DEBUG ESTADOS: Memória da fila de bloqueados liberada.\n");
}

// Adiciona um processo à fila de bloqueados (thread-safe)
void estadosAdicionarBloqueado(EstadoBloqueado_t *eb, int pid) {
    if (!eb) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de adicionar PID %d a EstadoBloqueado_t nulo.\n", pid);
        return;
    }
    pthread_mutex_lock(&bloqueados_mutex);
    
    // Opcional: Verificar se o PID já existe na fila para evitar duplicatas.
    FilaProcessos_t *fila = &eb->fila_geral_bloqueados;
    int encontrado = 0;
    if(fila->tamanho > 0) {
        int idx = fila->inicio_fila;
        for (int i = 0; i < fila->tamanho; i++) {
            if (fila->elementos[idx] == pid) {
                encontrado = 1;
                break;
            }
            idx = (idx + 1) % fila->capacidade;
        }
    }

    if (!encontrado) {
        filaEnfileirar(&eb->fila_geral_bloqueados, pid);
        // printf("DEBUG ESTADOS: PID %d adicionado à fila de bloqueados.\n", pid);
    } else {
        // printf("DEBUG ESTADOS: PID %d já presente na fila de bloqueados.\n", pid);
    }
    
    pthread_mutex_unlock(&bloqueados_mutex);
}

// Remove um PID específico da fila de bloqueados (thread-safe).
// Retorna 1 se removido, 0 se não encontrado ou se a fila estava vazia.
int estadosRemoverBloqueadoEspecifico(EstadoBloqueado_t *eb, int pid_a_remover) {
    if (!eb) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de remover PID %d de EstadoBloqueado_t nulo.\n", pid_a_remover);
        return 0;
    }

    pthread_mutex_lock(&bloqueados_mutex);
    FilaProcessos_t *fila_original = &eb->fila_geral_bloqueados;
    int removido = 0;

    if (filaEstaVazia(fila_original)) {
        pthread_mutex_unlock(&bloqueados_mutex);
        return 0; // Nada a remover
    }

    FilaProcessos_t fila_temporaria;
    filaInicializar(&fila_temporaria); 

    // Transfere elementos para a fila temporária, exceto o que deve ser removido
    while (!filaEstaVazia(fila_original)) {
        int pid_atual = filaDesenfileirar(fila_original);
        if (pid_atual == pid_a_remover) {
            removido = 1;
        } else {
            filaEnfileirar(&fila_temporaria, pid_atual);
        }
    }
    
    filaLiberarMemoria(fila_original); 
    *fila_original = fila_temporaria;  // Atribui a nova fila (com seus elementos e metadados)
    
    pthread_mutex_unlock(&bloqueados_mutex);
    return removido;
}