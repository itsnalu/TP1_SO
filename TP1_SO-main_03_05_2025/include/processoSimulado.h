#ifndef processoSimulado_H
#define processoSimulado_H
#include <stdio.h>

#define MAX_MEMORIA 100
#define MAX_PROCESSOS 100
// Processo Simulado está definindo os processos de forma estática, queremos lidar com
//ele de forma dinâmica, ou seja, o número de processos não é fixo e pode variar. Aí, a estrutura
//ProcessoSimulado deve ser alterada para lidar com isso. Para isso, vamos usar uma lista encadeada
// para armazenar os processos. Assim, o número de processos pode crescer ou diminuir conforme necessário.
typedef struct Processos Processos;

typedef enum {
    N, D, V, A, S, B, T, F, R  // Tipos de instrução
} tipoInstrucao;

typedef struct {
    char tipoInstrucao;
    int arg1;
    int arg2;
} instrucao;

typedef struct TipoCelula *TipoApontador;

typedef struct TipoCelula {
    instrucao Instrucao;
    TipoApontador Prox;
} TipoCelula;

typedef struct {
    TipoApontador Primeiro, Ultimo;
} TipoLista;

typedef struct {
    TipoLista listaInstrucoes;
    int pc;  // program counter
    int numProcessos;
    int memoria[100];
    int numVariaveis;
    char estado[50];    // EXECUCAO - BLOQUEADO - PRONTO 
    int qtd_bloq;
} ProcessoSimulado;

void carregar_programa(ProcessoSimulado *p, FILE *arquivo);
void executar_instrucao(ProcessoSimulado *processo);
ProcessoSimulado* criar_novo_processo();
void adicionarProcessoAoGerenciador(ProcessoSimulado *processo);

#endif
