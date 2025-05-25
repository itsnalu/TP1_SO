#ifndef CPU_THREAD_H
#define CPU_THREAD_H

// Includes do sistema
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// Forward declarations
struct ProcessoSimuladoThread_s;
typedef struct ProcessoSimuladoThread_s ProcessoSimuladoThread_t;

#define CPU_OCIOSA_THREAD -1

// Estrutura da CPU para threads
typedef struct {
    ProcessoSimuladoThread_t *processo_atual;  // Processo em execução
    volatile int indice_processo_na_tabela;    // Índice na tabela de processos
    volatile int pc_registrador_cpu;           // Program Counter
    volatile int quantum_total_alocado;        // Quantum total para o processo atual
    volatile int tempo_executado_neste_quantum;// Tempo já usado do quantum atual
    pthread_mutex_t mutex;                     // Mutex para sincronização
} CPUThread_t;

// Funções de gerenciamento da CPU
void cpuThreadInicializar(CPUThread_t *cpu);
void cpuThreadExecutarInstrucao(CPUThread_t *cpu, int tempo_global);
void cpuThreadFinalizar(CPUThread_t *cpu);

// Funções auxiliares
int cpuThreadEstaOciosa(const CPUThread_t *cpu);
void cpuThreadAtribuirProcesso(CPUThread_t *cpu, ProcessoSimuladoThread_t *processo, int indice);
void cpuThreadLiberarProcesso(CPUThread_t *cpu);

#endif // CPU_THREAD_H 