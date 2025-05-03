#ifndef PROCESSO_SIMULADO_H
#define PROCESSO_SIMULADO_H

#define MAX_MEMORIA 100
#define MAX_PROCESSOS 100

typedef struct Processos Processos;

typedef struct {
    char tp_inst;
    int arg1;
    int arg2;
} TipoInstrucao;

typedef struct TipoCelula *TipoApontador;

typedef struct TipoCelula {
    TipoInstrucao Instrucao;
    TipoApontador Prox;
} TipoCelula;

typedef struct {
    TipoApontador Primeiro, Ultimo;
} TipoLista;

typedef struct {
    TipoLista listaInstrucoes;
    int pc;  // program counter
    int num_processos;
    int memoria[100];
    int num_variaveis;
    char estado[50];    // EXECUCAO - BLOQUEADO - PRONTO
    int qtd_bloq;
} ProcessoSimulado;

void carregar_programa(ProcessoSimulado *p, FILE *arquivo);
void executar_instrucao(ProcessoSimulado *processo);
ProcessoSimulado* criar_novo_processo();

#endif
