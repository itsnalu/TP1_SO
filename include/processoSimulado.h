#ifndef PROCESSO_SIMULADO_H
#define PROCESSO_SIMULADO_H

#include "config.h"
#include <pthread.h>

// Define os possíveis estados de um processo simulado.
typedef enum {
    EST_NOVO,       // Processo recém-criado, ainda não na fila de prontos
    EST_PRONTO,     // Aguardando para usar a CPU
    EST_EXECUCAO,   // Atualmente utilizando a CPU (sua thread está rodando)
    EST_BLOQUEADO,  // Aguardando um evento externo (instrução 'B')
    EST_TERMINADO   // Execução concluída ou abortada
} EstadoProcesso_e;

// Função para converter o enum do estado para string (útil para logs)
const char* estadoParaString(EstadoProcesso_e estado);

// Representa uma única instrução do programa de um processo simulado.
typedef struct {
    char tipoInstrucaoChar;
    int arg1;
    int arg2;
    char nome_arquivo_R[MAX_NOME_ARQUIVO_R];
} Instrucao_t;

// Estruturas para a lista encadeada de instruções.
typedef struct CelulaInstrucao_s *ApontadorInstrucao_t;
typedef struct CelulaInstrucao_s {
    Instrucao_t instrucao;
    ApontadorInstrucao_t prox;
} CelulaInstrucao_t;

typedef struct {
    ApontadorInstrucao_t primeiro;
    ApontadorInstrucao_t ultimo;
    int tamanho;
} ListaInstrucoes_t;

// Estrutura principal que define um processo simulado.
typedef struct ProcessoSimulado_s {
    int pid;
    int pid_pai;
    EstadoProcesso_e estado_atual;
    int prioridade; // Usado ativamente pelo escalonador MLFQ

    int pc;
    ListaInstrucoes_t listaInstrucoes;
    int memoria[MAX_MEMORIA_PROCESSO_SIMULADO];
    int num_variaveis_declaradas;

    long tempo_chegada_sistema;
    long tempo_total_cpu_usado;      // Incrementado pela thread do processo
    int tempo_restante_bloqueio;    // Para a instrução 'B'

    // Campos para MLFQ
    int quantum_alocado_atual;      // Quantum recebido ao ser despachado
    int tempo_usado_no_quantum_atual; // Tempo consumido no quantum atual

    pthread_t thread_id; // Handle para a thread POSIX que executa este processo simulado
} ProcessoSimulado_t;


// Forward declaration do GerenciadorDeProcessos_t
struct GerenciadorDeProcessos_s;

// Estrutura para passar argumentos para a thread do processo
typedef struct {
    ProcessoSimulado_t* processo;
    struct GerenciadorDeProcessos_s* gerenciador;
} ThreadArgs_t;


// Funções de Lista de Instruções
void psInicializarListaInstrucoes(ListaInstrucoes_t *lista);
void psLiberarListaInstrucoes(ListaInstrucoes_t *lista);
void psInserirInstrucao(ListaInstrucoes_t *lista, Instrucao_t inst);
void psCopiarListaInstrucoes(ListaInstrucoes_t *destino, const ListaInstrucoes_t *origem);

// Funções de Gerenciamento do Processo Simulado
ProcessoSimulado_t* psCriarNovo(int pid_pai_temp, int prioridade_inicial, long tempo_criacao);
void psCarregarProgramaDeArquivo(ProcessoSimulado_t *p, const char* nome_arquivo_programa);
void psLiberarMemoria(ProcessoSimulado_t *p); // Libera o processo e sua lista de instruções
Instrucao_t* psObterInstrucaoNoPc(const ProcessoSimulado_t *p);

// Função principal executada pela thread de cada processo simulado.
void* psExecutarProcesso(void* arg);

void psImprimirInstrucoes(const ProcessoSimulado_t *p); // Para debug

#endif // PROCESSO_SIMULADO_H