/* 1.2 Completar em include/cpu.h e src/cpu.c

    Definir a estrutura CPU_t com todos os campos necessários
    Implementar funções para inicializar a CPU
    Implementar funções para atualizar registradores da CPU
    Implementar funções para salvar e restaurar o contexto da CPU */

#include "cpu.h"


void cpuInicializar(CPU_t *cpu){
    cpu->indice_processo_na_tabela = CPU_OCIOSA; // Inicializa como ociosa
    cpu->pc_registrador_cpu = 0; // Inicializa o PC
    cpu->quantum_total_alocado = 0; // Inicializa o quantum total alocado
    cpu->tempo_executado_neste_quantum = 0; // Inicializa o tempo executado neste quantum
    cpu->processo_atual = NULL; // Inicializa o ponteiro para o processo atual como NULL
}
// Atualiza os registradores da CPU com os valores do processo atual.
void cpuAtualizarRegistradores(CPU_t *cpu, ProcessoSimulado_t *processo, int quantum){
    if(processo == NULL){
        fprintf(stderr,"Erro:Tentativa de atualizar registradores com processo nulo\n");
        return;
    }
    cpu->processo_atual = processo; // Atualiza o processo atual
    cpu->pc_registrador_cpu = processo->pc; // Atualiza o PC com o valor do processo
    cpu->quantum_total_alocado = quantum; // Atualiza o quantum total alocado
    cpu->tempo_executado_neste_quantum = 0; // Reinicia o tempo executado neste quantum
}
// Salva o contexto do processo atual da CPU.
void cpuSalvarContexto(CPU_t *cpu){
    if(cpu->processo_atual == NULL){
        fprintf(stderr,"Erro:Tentativa de salvar contexto com processo nulo\n");
        return;
    }
    cpu->processo_atual->pc = cpu->pc_registrador_cpu; // Salva o PC do processo atual
    cpu->processo_atual->tempo_usado_no_quantum_atual = cpu->tempo_executado_neste_quantum; // Salva o tempo usado no quantum atual
    cpu->processo_atual->estado_atual = EST_PRONTO; // Atualiza o estado do processo para pronto para reinserção na fila
    // Adiciona o processo na fila de prontos (Não sei se isso é feito na cpu ou no gerenciador, verificar isso)
}
// Restaura o contexto de um processo para a CPU.
void cpuRestaurarContexto(CPU_t *cpu, ProcessoSimulado_t *processo){
    if(cpu->processo_atual == NULL){
        fprintf(stderr,"Erro:Tentativa de restaurar contexto com processo nulo\n");
        return;
    }
    cpu->tempo_executado_neste_quantum++; // Incrementa o tempo executado neste quantum
}
// Incrementa o tempo executado no quantum atual.
void cpuIncrementarTempoExecutado(CPU_t *cpu){
    if(cpu->processo_atual == NULL){
        fprintf(stderr,"Erro:Tentativa de incrementar tempo executado com processo nulo\n");
        return;
    }
    cpu->tempo_executado_neste_quantum++; // Incrementa o tempo executado neste quantum
}
// Verifica se o quantum alocado foi totalmente consumido.
int cpuQuantumConsumido(const CPU_t *cpu){
    return cpu->tempo_executado_neste_quantum >= cpu->quantum_total_alocado; // Retorna se o quantum foi consumido
}




