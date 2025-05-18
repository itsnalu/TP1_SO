#include "../include/processoImpressao.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void processoImpressaoIniciar(GerenciadorDeProcessos_t *gerenciador, int tipo_impressao) {
    ProcessoImpressao_t *impressao = (ProcessoImpressao_t *)malloc(sizeof(ProcessoImpressao_t));
    if (!impressao) {
        perror("Erro ao alocar memória para processo de impressão");
        exit(EXIT_FAILURE);
    }

    impressao->pid = getpid();
    impressao->tipo_impressao = tipo_impressao;
    impressao->gerenciador = gerenciador;

    if (tipo_impressao == 0) {
        processoImpressaoImprimirEstado(impressao);
    } else {
        processoImpressaoImprimirEstatisticas(impressao);
    }

    processoImpressaoFinalizar(impressao);
}

void processoImpressaoImprimirEstado(ProcessoImpressao_t *impressao) {
    printf("\n=== Estado Atual do Sistema ===\n");
    printf("Tempo Global: %ld\n", impressao->gerenciador->tempo_simulacao_global);
    
    // Imprime estado da CPU
    printf("\nCPU:\n");
    if (impressao->gerenciador->cpu_sistema.processo_atual) {
        printf("  Processo Atual: PID %d\n", impressao->gerenciador->cpu_sistema.processo_atual->pid);
        printf("  PC: %d\n", impressao->gerenciador->cpu_sistema.pc_registrador_cpu);
        printf("  Quantum Total: %d\n", impressao->gerenciador->cpu_sistema.quantum_total_alocado);
        printf("  Tempo Executado: %d\n", impressao->gerenciador->cpu_sistema.tempo_executado_neste_quantum);
    } else {
        printf("  CPU Ociosa\n");
    }
    
    // Imprime processos prontos
    printf("\nProcessos Prontos:\n");
    for (int i = 0; i < NUM_NIVEIS_PRIORIDADE; i++) {
        printf("  Prioridade %d:\n", i);
        if (!filaEstaVazia(&impressao->gerenciador->processos_prontos.filas_por_prioridade[i])) {
            int idx = impressao->gerenciador->processos_prontos.filas_por_prioridade[i].inicio_fila;
            int count = 0;
            while (count < impressao->gerenciador->processos_prontos.filas_por_prioridade[i].tamanho) {
                int pid = impressao->gerenciador->processos_prontos.filas_por_prioridade[i].elementos[idx];
                ProcessoSimulado_t *p = &impressao->gerenciador->tabela_de_processos[pid];
                printf("    PID %d (PC: %d, Tempo CPU: %ld)\n", 
                       pid, p->pc, p->tempo_total_cpu_usado);
                idx = (idx + 1) % impressao->gerenciador->processos_prontos.filas_por_prioridade[i].capacidade;
                count++;
            }
        } else {
            printf("    Nenhum processo\n");
        }
    }
    
    // Imprime processos bloqueados
    printf("\nProcessos Bloqueados:\n");
    if (!filaEstaVazia(&impressao->gerenciador->processos_bloqueados.fila_geral_bloqueados)) {
        int idx = impressao->gerenciador->processos_bloqueados.fila_geral_bloqueados.inicio_fila;
        int count = 0;
        while (count < impressao->gerenciador->processos_bloqueados.fila_geral_bloqueados.tamanho) {
            int pid = impressao->gerenciador->processos_bloqueados.fila_geral_bloqueados.elementos[idx];
            ProcessoSimulado_t *p = &impressao->gerenciador->tabela_de_processos[pid];
            printf("  PID %d (Tempo Restante Bloqueio: %d)\n", 
                   pid, p->tempo_restante_bloqueio);
            idx = (idx + 1) % impressao->gerenciador->processos_bloqueados.fila_geral_bloqueados.capacidade;
            count++;
        }
    } else {
        printf("  Nenhum processo bloqueado\n");
    }
    printf("\n==============================\n");
}

void processoImpressaoImprimirEstatisticas(ProcessoImpressao_t *impressao) {
    printf("\n=== Estatísticas Finais ===\n");
    printf("Tempo Total de Simulação: %ld\n", impressao->gerenciador->tempo_simulacao_global);
    
    // Imprime estatísticas de cada processo
    for (int i = 0; i < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; i++) {
        if (impressao->gerenciador->slot_tabela_ocupado[i]) {
            ProcessoSimulado_t *p = &impressao->gerenciador->tabela_de_processos[i];
            printf("\nProcesso %d:\n", i);
            printf("  Estado Final: %d\n", p->estado_atual);
            printf("  Tempo Total de CPU: %ld\n", p->tempo_total_cpu_usado);
            printf("  Tempo de Chegada: %ld\n", p->tempo_chegada_sistema);
            printf("  Prioridade Final: %d\n", p->prioridade);
        }
    }
    printf("\n==============================\n");
}

void processoImpressaoFinalizar(ProcessoImpressao_t *impressao) {
    free(impressao);
    exit(EXIT_SUCCESS);
} 