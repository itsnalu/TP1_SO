#ifndef GERENCIADOR_H   
#define GERENCIADOR_H

#include "processoSimulado.h"
#include "cpu.h"
#include "estados.h"

typedef struct {
    int tempo; // Tempo de execução do processo
    CPU *cpu; // Ponteiro para a CPU
    EstadoPronto *estadoPronto; // Estado pronto
    EstadoBloqueado *estadoBloqueado; // Estado bloqueado
    EstadoExecucao *estadoExecucao; // Estado de execução
    ProcessoSimulado *TabelaDeProcessos[MAX_PROCESSOS]; // Tabela de processos
    int pc; // Contador de programa 
    int *memoria; // Ponteiro para a memória
    int quantum; // Tempo de quantum
} GerenciadorDeProcessos;

void criaNovoProcessoSimulado();
void substituirImagemParaNova();
void gerenciarTransicaoDeEstados();
void escalonarProcesso();
void trocarContexto();

#endif