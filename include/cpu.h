#ifndef CPU_H
#define CPU_H
#define CPU_OCIOSA -1 // Valor que indica que a CPU está ociosa.
#include "processos.h"

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

    ProcessoSimulado_t *processo_atual;     // Apontador para o processo simulado atualmente na CPU.
    //Olhar se é uma representação de um estadoem execução, se nn, implementar isso em estados .c e .h

} CPU_t;

// Inicializa a CPU para um estado ocioso/padrão.
void cpuInicializar(CPU_t *cpu);
// Atualiza os registradores da CPU com os valores do processo atual.
void cpuAtualizarRegistradores(CPU_t *cpu, ProcessoSimulado_t *processo, int quantum);
// Salva o contexto do processo atual da CPU.
void cpuSalvarContexto(CPU_t *cpu);
// Restaura o contexto de um processo para a CPU.
void cpuRestaurarContexto(CPU_t *cpu, ProcessoSimulado_t *processo);
// Incrementa o tempo executado no quantum atual.
void cpuIncrementarTempoExecutado(CPU_t *cpu);
// Verifica se o quantum alocado foi totalmente consumido.
int cpuQuantumConsumido(const CPU_t *cpu);
// Libera a CPU, deixando-a ociosa.
void cpuLiberar(CPU_t *cpu);

#endif // CPU_H