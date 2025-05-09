#ifndef PROCESSOS_H
#define PROCESSOS_H

#include "processoSimulado.h"

#define MAX_PROCESSOS 100  // Número máximo de processos

typedef struct Processos {
    ProcessoSimulado **processos;
    int numProcessos;
    int capacidade;
} Processos;

void inicializarGerenciador(Processos *gerenciador);
ProcessoSimulado* criarNovoProcesso();
void adicionarProcesso(Processos *gerenciador, ProcessoSimulado *processo);

#endif
