#include "../include/processoImpressao.h"
#include <stdio.h>
#include <stdlib.h>

void processoImpressaoIniciar(GerenciadorDeProcessos_t *gerenciador, int tipo_impressao) {
    ProcessoImpressao_t *impressao = (ProcessoImpressao_t *)malloc(sizeof(ProcessoImpressao_t));
    if (!impressao) {
        perror("Erro ao alocar memória para processo de impressão");
        exit(EXIT_FAILURE);
    }

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
    printf("\n╔════════════════════════════════╗");
    printf("\n║   ESTADO ATUAL DO SISTEMA      ║");
    printf("\n╚════════════════════════════════╝\n");

    printf("\n┌────────────────────────────────┐");
    printf("\n│ Tempo Global: %-16ld │", impressao->gerenciador->tempo_simulacao_global);
    printf("\n└────────────────────────────────┘");

    printf("\n\n┌───── CPU ──────────────────────┐");
    if (impressao->gerenciador->cpu_sistema.processo_atual) {
        ProcessoSimulado_t *proc_atual = impressao->gerenciador->cpu_sistema.processo_atual;
        printf("\n│ Processo Atual: PID %-10d │", proc_atual->pid);
        printf("\n│ PC: %-26d │", proc_atual->pc);
        printf("\n│ Quantum Total: %-15d │", impressao->gerenciador->cpu_sistema.quantum_total_alocado);
        printf("\n│ Tempo Executado: %-13d │", impressao->gerenciador->cpu_sistema.tempo_executado_neste_quantum);
    } else {
        printf("\n│          CPU OCIOSA            │");
    }
    printf("\n└────────────────────────────────┘");
    
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
                       p->pid, p->pc, p->tempo_total_cpu_usado);
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
                   p->pid, p->tempo_restante_bloqueio);
            idx = (idx + 1) % impressao->gerenciador->processos_bloqueados.fila_geral_bloqueados.capacidade;
            count++;
        }
    } else {
        printf("  Nenhum processo bloqueado\n");
    }
    printf("\n==============================\n");
}

void processoImpressaoImprimirEstatisticas(ProcessoImpressao_t *impressao) {
    printf("\n╔════════════════════════════════╗");
    printf("\n║      ESTATÍSTICAS FINAIS       ║");
    printf("\n╚════════════════════════════════╝\n");
    
    printf("\n┌────────────────────────────────┐");
    printf("\n│ Tempo Total de Simulação: %-4ld │", impressao->gerenciador->tempo_simulacao_global);
    printf("\n└────────────────────────────────┘\n");
    
    // Imprime estatísticas de cada processo
    for (int i = 0; i < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; i++) {
        if (impressao->gerenciador->slot_tabela_ocupado[i]) {
            ProcessoSimulado_t *p = &impressao->gerenciador->tabela_de_processos[i];
            printf("\n┌───── Processo %d ───────────────┐\n", i);
            printf("│ Estado Final: %-16d │\n", p->estado_atual);
            printf("│ Tempo Total de CPU: %-10ld │\n", p->tempo_total_cpu_usado);
            printf("│ Tempo de Chegada: %-12ld │\n", p->tempo_chegada_sistema);
            printf("│ Prioridade Final: %-12d │\n", p->prioridade);
            printf("└────────────────────────────────┘\n");
        }
    }
    printf("\n==============================\n");
}

void processoImpressaoFinalizar(ProcessoImpressao_t *impressao) {
    free(impressao);
}
 