#ifndef CPU_H
#define CPU_H

// #include "processos.h" // O include original da "Ana" era "processos.h", que define Processos_s.
                          // A struct CPU_t usa ProcessoSimulado_t*. processoSimulado.h é mais apropriado.
#include "processoSimulado.h" // Para ProcessoSimulado_t

#define CPU_OCIOSA -1 // Valor que indica que a CPU está ociosa.


typedef struct {
    // Índice na TabelaDeProcessos do ProcessoSimulado_t atualmente "na CPU".
    // -1 (CPU_OCIOSA) indica que nenhuma thread de processo foi despachada recentemente ou a última cedeu.
    int indice_processo_na_tabela;

    // Ponteiro para o processo simulado atualmente "na CPU".
    // Pode ser NULL se a CPU estiver ociosa.
    ProcessoSimulado_t *processo_atual;     


    int pc_registrador_cpu;             // Espelho do PC do processo_atual.
    int quantum_total_alocado;          // Espelho do quantum_alocado_atual do processo_atual.
    int tempo_executado_neste_quantum;  // Espelho do tempo_usado_no_quantum_atual do processo_atual.

} CPU_t;

// Inicializa a CPU para um estado ocioso/padrão.
void cpuInicializar(CPU_t *cpu);

// Libera a CPU, marcando-a como ociosa (opcional, mas pode ser útil).
// Se o gerenciador usar 'processo_ativo_pid', esta função pode não ser estritamente necessária,
// pois setar processo_ativo_pid = -1 já indica ociosidade.
void cpuLiberar(CPU_t *cpu); // Define a CPU para ociosa


#endif // CPU_H