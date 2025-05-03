#ifndef PROCESSOS_H
#define PROCESSOS_H

#include "processo_simulado.h"

#define MAX_PROCESSOS 100  // Número máximo de processos

typedef struct Processos {
    ProcessoSimulado **processos;
    int num_processos;
    int capacidade;
} Processos;

void inicializar_gerenciador(Processos *gerenciador);
ProcessoSimulado* criar_novo_processo();
void adicionar_processo(Processos *gerenciador, ProcessoSimulado *processo);

#endif
