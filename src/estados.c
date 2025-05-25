#include "../include/estados.h" // Caminho do include da "Ana"
#include "../include/fila.h"   // Para as operações de fila
#include "../include/config.h" // Para NUM_NIVEIS_PRIORIDADE e declarações extern dos mutexes globais
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> // Para pthread_mutex_lock e pthread_mutex_unlock

// As funções de inicialização e liberação não manipulam dados concorrentemente,
// então não precisam de mutexes diretamente nelas.
void estadosInicializarProntos(EstadoPronto_t *ep) {
    if (!ep) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de inicializar EstadoPronto_t nulo.\n");
        return;
    }
    for (int i = 0; i < NUM_NIVEIS_PRIORIDADE; i++) {
        filaInicializar(&ep->filas_por_prioridade[i]);
    }
    // A fila_fifo foi removida da struct EstadoPronto_t
    // Se fosse mantida: filaInicializar(&ep->fila_fifo);
    // printf("[DEBUG Estados] Filas de prontos (MLFQ) inicializadas.\n");
}

void estadosInicializarBloqueados(EstadoBloqueado_t *eb) {
    if (!eb) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de inicializar EstadoBloqueado_t nulo.\n");
        return;
    }
    filaInicializar(&eb->fila_geral_bloqueados);
    // printf("[DEBUG Estados] Fila de bloqueados inicializada.\n");
}

void estadosLiberarProntos(EstadoPronto_t *ep) {
    if (!ep) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de liberar EstadoPronto_t nulo.\n");
        return;
    }
    for (int i = 0; i < NUM_NIVEIS_PRIORIDADE; i++) {
        filaLiberarMemoria(&ep->filas_por_prioridade[i]);
    }
    // Se fila_fifo fosse mantida: filaLiberarMemoria(&ep->fila_fifo);
    // printf("[DEBUG Estados] Memória das filas de prontos (MLFQ) liberada.\n");
}

void estadosLiberarBloqueados(EstadoBloqueado_t *eb) {
    if (!eb) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de liberar EstadoBloqueado_t nulo.\n");
        return;
    }
    filaLiberarMemoria(&eb->fila_geral_bloqueados);
    // printf("[DEBUG Estados] Memória da fila de bloqueados liberada.\n");
}

// Adiciona um PID à fila de prontos correspondente à sua prioridade (thread-safe).
void estadosAdicionarPronto(EstadoPronto_t *ep, int pid, int prioridade) {
    if (!ep) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de adicionar PID %d a EstadoPronto_t nulo.\n", pid);
        return;
    }

    // Valida a prioridade e ajusta se necessário
    if (prioridade < 0 || prioridade >= NUM_NIVEIS_PRIORIDADE) {
        fprintf(stderr, "ESTADOS AVISO: PID %d com prioridade inválida %d. Ajustando para prioridade %d (mais baixa).\n", 
                pid, prioridade, NUM_NIVEIS_PRIORIDADE - 1);
        prioridade = NUM_NIVEIS_PRIORIDADE - 1; // Fallback para a fila de menor prioridade
    }

    pthread_mutex_lock(&prontos_mutex);
    
    // Opcional: Verificar se o PID já está na fila de destino para evitar duplicatas.
    // Esta checagem pode adicionar overhead, mas aumenta a robustez.
    int encontrado_na_fila = 0;
    FilaProcessos_t *fila_destino = &ep->filas_por_prioridade[prioridade];
    if (fila_destino->tamanho > 0) {
        int idx = fila_destino->inicio_fila;
        for (int i = 0; i < fila_destino->tamanho; i++) {
            if (fila_destino->elementos[idx] == pid) {
                encontrado_na_fila = 1;
                break;
            }
            idx = (idx + 1) % fila_destino->capacidade;
        }
    }

    if (!encontrado_na_fila) {
        filaEnfileirar(&ep->filas_por_prioridade[prioridade], pid);
        // printf("[DEBUG Estados] PID %d adicionado à fila de prontos (Prio %d).\n", pid, prioridade);
    } else {
        // printf("[DEBUG Estados] PID %d já presente na fila de prontos (Prio %d). Não adicionado novamente.\n", pid, prioridade);
    }
    
    pthread_mutex_unlock(&prontos_mutex);
}

// Remove e retorna o PID do processo da fila de maior prioridade não vazia (thread-safe).
// Retorna -1 se todas as filas de prontos estiverem vazias.
int estadosRemoverPronto(EstadoPronto_t *ep) {
    if (!ep) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de remover de EstadoPronto_t nulo.\n");
        return -1;
    }

    pthread_mutex_lock(&prontos_mutex);
    int pid_removido = -1;
    // Itera da maior prioridade (0) para a menor
    for (int i = 0; i < NUM_NIVEIS_PRIORIDADE; i++) { 
        if (!filaEstaVazia(&ep->filas_por_prioridade[i])) {
            pid_removido = filaDesenfileirar(&ep->filas_por_prioridade[i]);
            // printf("[DEBUG Estados] PID %d removido da fila de prontos (Prio %d).\n", pid_removido, i);
            break; // Encontrou e removeu, sai do loop
        }
    }
    pthread_mutex_unlock(&prontos_mutex);
    return pid_removido;
}

// Adiciona um PID à fila de processos bloqueados (thread-safe).
void estadosAdicionarBloqueado(EstadoBloqueado_t *eb, int pid) {
    if (!eb) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de adicionar PID %d a EstadoBloqueado_t nulo.\n", pid);
        return;
    }

    pthread_mutex_lock(&bloqueados_mutex);
    
    // Opcional: Verificar se o PID já existe na fila para evitar duplicatas.
    int encontrado = 0;
    FilaProcessos_t *fila_bloq = &eb->fila_geral_bloqueados;
    if(fila_bloq->tamanho > 0) {
        int idx = fila_bloq->inicio_fila;
        for (int i = 0; i < fila_bloq->tamanho; i++) {
            if (fila_bloq->elementos[idx] == pid) {
                encontrado = 1;
                break;
            }
            idx = (idx + 1) % fila_bloq->capacidade;
        }
    }

    if (!encontrado) {
        filaEnfileirar(&eb->fila_geral_bloqueados, pid);
        // printf("[DEBUG Estados] PID %d adicionado à fila de bloqueados.\n", pid);
    } else {
        // printf("[DEBUG Estados] PID %d já presente na fila de bloqueados. Não adicionado novamente.\n", pid);
    }
    
    pthread_mutex_unlock(&bloqueados_mutex);
}

// Remove e retorna o PID do processo do início da fila de bloqueados (thread-safe).
// Mantido da "Ana", mas agora thread-safe.
int estadosRemoverBloqueado(EstadoBloqueado_t *eb) {
    if (!eb) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de remover de EstadoBloqueado_t nulo.\n");
        return -1;
    }
    pthread_mutex_lock(&bloqueados_mutex);
    int pid_removido = -1;
    if (!filaEstaVazia(&eb->fila_geral_bloqueados)) {
        pid_removido = filaDesenfileirar(&eb->fila_geral_bloqueados);
        // printf("[DEBUG Estados] PID %d removido do início da fila de bloqueados.\n", pid_removido);
    } else {
        // printf("[DEBUG Estados] Tentativa de remover da fila de bloqueados vazia.\n");
    }
    pthread_mutex_unlock(&bloqueados_mutex);
    return pid_removido;
}

// Remove um PID específico da fila de bloqueados (thread-safe).
// Necessário para quando um processo acorda e precisa ser removido, não necessariamente do início.
int estadosRemoverBloqueadoEspecifico(EstadoBloqueado_t *eb, int pid_a_remover) {
    if (!eb) {
        fprintf(stderr, "ESTADOS ERRO: Tentativa de remover PID %d de EstadoBloqueado_t nulo.\n", pid_a_remover);
        return 0; // Retorna 0 para indicar falha ou não encontrado
    }

    pthread_mutex_lock(&bloqueados_mutex);
    FilaProcessos_t *fila_original = &eb->fila_geral_bloqueados;
    int removido_com_sucesso = 0;

    if (filaEstaVazia(fila_original)) {
        pthread_mutex_unlock(&bloqueados_mutex);
        return 0; // Nada a remover, fila vazia
    }

    // Cria uma fila temporária para armazenar os elementos que não serão removidos
    FilaProcessos_t fila_temporaria;
    filaInicializar(&fila_temporaria); 

    // Transfere elementos da fila original para a temporária, exceto o PID a ser removido
    while (!filaEstaVazia(fila_original)) {
        int pid_atual = filaDesenfileirar(fila_original);
        if (pid_atual == pid_a_remover) {
            removido_com_sucesso = 1; // Marca que o PID foi encontrado e "removido" (não copiado)
        } else {
            filaEnfileirar(&fila_temporaria, pid_atual); // Copia os outros PIDs
        }
    }
    
    // A fila original agora está vazia. Seus elementos alocados precisam ser liberados.
    filaLiberarMemoria(fila_original); 
    // A fila original agora recebe a estrutura da fila temporária (incluindo seus elementos e metadados).
    *fila_original = fila_temporaria;  
    
    pthread_mutex_unlock(&bloqueados_mutex);

    // if (removido_com_sucesso) {
    //     printf("[DEBUG Estados] PID %d removido especificamente da fila de bloqueados.\n", pid_a_remover);
    // } else {
    //     printf("[DEBUG Estados] PID %d não encontrado para remoção específica da fila de bloqueados.\n", pid_a_remover);
    // }
    return removido_com_sucesso; // Retorna 1 se removido, 0 se não
}

// As funções relacionadas à fila FIFO (estadosAdicionarProntoFIFO, estadosRemoverProntoFIFO)
// foram removidas, pois a fila_fifo foi removida da struct EstadoPronto_t no include/estados.h.
// Se a funcionalidade FIFO for reintroduzida, essas funções devem ser implementadas e tornadas thread-safe.