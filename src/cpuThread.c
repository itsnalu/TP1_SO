#include "../include/cpuThread.h"
#include <stdio.h>
#include <stdlib.h>

void cpuThreadInicializar(CPUThread_t *cpu) {
    pthread_mutex_lock(&cpu->mutex);
    cpu->processo_atual = NULL;
    cpu->indice_processo_na_tabela = CPU_OCIOSA_THREAD;
    cpu->pc_registrador_cpu = 0;
    cpu->quantum_total_alocado = 0;
    cpu->tempo_executado_neste_quantum = 0;
    pthread_mutex_init(&cpu->mutex, NULL);
    pthread_mutex_unlock(&cpu->mutex);
}

void cpuThreadExecutarInstrucao(CPUThread_t *cpu, int tempo_global) {
    pthread_mutex_lock(&cpu->mutex);
    
    if (cpu->processo_atual == NULL) {
        pthread_mutex_unlock(&cpu->mutex);
        return;
    }
    
    // Executa a instrução do processo atual
    psThreadExecutarInstrucao(cpu->processo_atual, tempo_global);
    
    // Atualiza o PC da CPU
    cpu->pc_registrador_cpu = cpu->processo_atual->pc;
    
    // Incrementa o tempo de execução neste quantum
    cpu->tempo_executado_neste_quantum++;
    
    pthread_mutex_unlock(&cpu->mutex);
}

void cpuThreadFinalizar(CPUThread_t *cpu) {
    pthread_mutex_lock(&cpu->mutex);
    cpu->processo_atual = NULL;
    cpu->indice_processo_na_tabela = CPU_OCIOSA_THREAD;
    pthread_mutex_unlock(&cpu->mutex);
    pthread_mutex_destroy(&cpu->mutex);
} 