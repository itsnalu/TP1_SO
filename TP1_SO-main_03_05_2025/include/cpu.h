#ifndef CPU_H   
#define CPU_H

#include "processoSimulado.h"

typedef struct {
    TipoLista *programa;      // Ponteiro para o programa do processo (lista de instruções)
    int pid;                  // PID do processo em execução
    int pc;                   // Contador de programa atual
    int *memoria;             // Vetor de variáveis inteiras do processo
    int tempo_quantum_usado;  // Unidades de tempo usadas no quantum atual
} CPU;

#endif