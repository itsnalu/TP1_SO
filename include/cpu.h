#ifndef CPU_H
#define CPU_H

// Representa a unidade central de processamento simulada.
typedef struct {
    // Índice na TabelaDeProcessos do ProcessoSimulado_t atualmente carregado na CPU.
    // Valor como -1 (ou uma constante definida como CPU_OCIOSA) indica que a CPU está livre.
    int indice_processo_na_tabela;

    // Cópia do Program Counter do processo carregado. É o PC que a CPU utiliza e incrementa.
    // Deve ser sincronizado com ProcessoSimulado_t->pc durante a troca de contexto.
    int pc_registrador_cpu;

    // Quantum (fatia de tempo) total que foi alocado para o processo corrente nesta sua vez na CPU.
    int quantum_total_alocado;

    // Tempo de CPU já consumido pelo processo dentro do quantum_total_alocado.
    // Incrementado a cada unidade de tempo 'U' que o processo executa.
    int tempo_executado_neste_quantum;

} CPU_t;

// Inicializa a CPU para um estado ocioso/padrão.
void cpuInicializar(CPU_t *cpu);

#endif // CPU_H