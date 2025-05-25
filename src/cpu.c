#include "../include/cpu.h"        // Para a definição de CPU_t e CPU_OCIOSA
#include "../include/processoSimulado.h" // Para ProcessoSimulado_t (usado em CPU_t->processo_atual)
#include <stdio.h>                 // Para fprintf em caso de erro (opcional)

// Inicializa a CPU para um estado ocioso/padrão.
// No novo modelo, isso significa que nenhum processo simulado (thread) está
// atualmente despachado ou considerado "na CPU" pelo gerenciador.
void cpuInicializar(CPU_t *cpu) {
    if (!cpu) {
        fprintf(stderr, "CPU ERRO: Tentativa de inicializar CPU_t nula.\n");
        return;
    }
    cpu->indice_processo_na_tabela = CPU_OCIOSA;
    cpu->processo_atual = NULL;
    cpu->pc_registrador_cpu = 0;
    cpu->quantum_total_alocado = 0;
    cpu->tempo_executado_neste_quantum = 0;
}

// Libera a CPU, marcando-a como ociosa.
// Essencialmente, redefine para o estado inicial.
void cpuLiberar(CPU_t *cpu) {
    if (!cpu) {
        fprintf(stderr, "CPU ERRO: Tentativa de liberar CPU_t nula.\n");
        return;
    }
    cpu->indice_processo_na_tabela = CPU_OCIOSA;
    cpu->processo_atual = NULL;
    cpu->pc_registrador_cpu = 0;
    cpu->quantum_total_alocado = 0;
    cpu->tempo_executado_neste_quantum = 0;
}
