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

#ifdef USE_FIFO
    printf("\nProcessos Prontos (Fila FIFO):\n");
    FilaProcessos_t *fila_fifo = &impressao->gerenciador->processos_prontos.fila_fifo;
    if (!filaEstaVazia(fila_fifo)) {
        int idx = fila_fifo->inicio_fila;
        int count = 0;
        while (count < fila_fifo->tamanho) {
            int pid = fila_fifo->elementos[idx];
            if (pid >= 0 && pid < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA && impressao->gerenciador->slot_tabela_ocupado[pid]) {
                ProcessoSimulado_t *p = &impressao->gerenciador->tabela_de_processos[pid];
                if (p->estado_atual == EST_PRONTO) { // Verifica se realmente está PRONTO
                    printf("    PID %d (PC: %d, Tempo CPU: %ld)\n",
                           p->pid, p->pc, p->tempo_total_cpu_usado);
                }
            }
            idx = (idx + 1) % fila_fifo->capacidade;
            count++;
        }
    } else {
        printf("    Nenhum processo na fila FIFO\n");
    }
#else
    printf("\nProcessos Prontos:\n");
    for (int i = 0; i < NUM_NIVEIS_PRIORIDADE; i++) {
        printf("  Prioridade %d:\n", i);
        FilaProcessos_t *fila_prioridade = &impressao->gerenciador->processos_prontos.filas_por_prioridade[i];
        if (!filaEstaVazia(fila_prioridade)) {
            int idx = fila_prioridade->inicio_fila;
            int count = 0;
            while (count < fila_prioridade->tamanho) {
                int pid = fila_prioridade->elementos[idx];
                if (pid >= 0 && pid < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA && impressao->gerenciador->slot_tabela_ocupado[pid]) {
                    ProcessoSimulado_t *p = &impressao->gerenciador->tabela_de_processos[pid];
                     if (p->estado_atual == EST_PRONTO) { // Verifica se realmente está PRONTO
                        printf("    PID %d (PC: %d, Prioridade: %d, Tempo CPU: %ld)\n",
                               p->pid, p->pc, p->prioridade, p->tempo_total_cpu_usado);
                    }
                }
                idx = (idx + 1) % fila_prioridade->capacidade;
                count++;
            }
        } else {
            printf("    Nenhum processo\n");
        }
    }
#endif

    printf("\nProcessos Bloqueados:\n");
    FilaProcessos_t *fila_bloqueados = &impressao->gerenciador->processos_bloqueados.fila_geral_bloqueados;
    if (!filaEstaVazia(fila_bloqueados)) {
        int idx = fila_bloqueados->inicio_fila;
        int count = 0;
        // Itera sobre uma cópia dos PIDs ou de forma cuidadosa se a fila puder ser modificada por outro thread
        // Para a impressão, a iteração simples deve ser segura, pois a fila não é modificada *durante* esta função de impressão.
        while (count < fila_bloqueados->tamanho) {
            int pid = fila_bloqueados->elementos[idx];
            if (pid >= 0 && pid < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA && impressao->gerenciador->slot_tabela_ocupado[pid]) {
                ProcessoSimulado_t *p = &impressao->gerenciador->tabela_de_processos[pid];
                // Ele poderia ter sido despertado e movido para pronto entre a última ação e a impressão,
                // embora com 'U' sendo a unidade de tempo, isso é menos provável de ser um problema de corrida aqui.
                if (p->estado_atual == EST_BLOQUEADO) {
                     printf("  PID %d (Tempo Restante Bloqueio: %d, Prioridade: %d)\n",
                           p->pid, p->tempo_restante_bloqueio, p->prioridade);
                }
            }
            idx = (idx + 1) % fila_bloqueados->capacidade;
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
 