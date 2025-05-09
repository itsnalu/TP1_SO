#ifndef PROCESSO_SIMULADO_H
#define PROCESSO_SIMULADO_H

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define os possíveis estados de um processo simulado.
typedef enum {
    EST_PRONTO,     // Aguardando para usar a CPU
    EST_EXECUCAO,   // Atualmente utilizando a CPU
    EST_BLOQUEADO,  // Aguardando um evento externo (ex: I/O simulado pela instrução 'B')
    EST_TERMINADO   // Execução concluída ou abortada
} EstadoProcesso_e;

// Representa uma única instrução do programa de um processo simulado.
typedef struct {
    char tipoInstrucaoChar; // Caractere da instrução (N, D, V, A, S, B, T, F, R)
    int arg1;               // Primeiro argumento numérico da instrução
    int arg2;               // Segundo argumento numérico da instrução
    char nome_arquivo_R[MAX_NOME_ARQUIVO_R]; // Nome do arquivo para a instrução 'R'
} Instrucao_t;

// Estruturas para a lista encadeada de instruções.
typedef struct CelulaInstrucao_s *ApontadorInstrucao_t;
typedef struct CelulaInstrucao_s {
    Instrucao_t instrucao;
    ApontadorInstrucao_t prox;
} CelulaInstrucao_t;

typedef struct {
    ApontadorInstrucao_t primeiro, ultimo;
    int tamanho;
} ListaInstrucoes_t;

// Estrutura principal que define um processo simulado.
typedef struct ProcessoSimulado_s {
    int pid;                // Identificador único do processo
    int pid_pai;            // PID do processo que criou este (se aplicável)
    EstadoProcesso_e estado_atual; // Estado corrente do processo
    int prioridade;         // Nível de prioridade para escalonamento

    int pc;                 // Program Counter: índice da próxima instrução a executar
    ListaInstrucoes_t listaInstrucoes; // Código do processo
    int memoria[MAX_MEMORIA_PROCESSO_SIMULADO]; // Espaço de memória do processo
    int num_variaveis_declaradas; // Número de variáveis definidas pela instrução 'N'

    // Métricas e contadores
    long tempo_chegada_sistema;     // Momento da criação ou primeira vez pronto
    long tempo_total_cpu_usado;     // Tempo total de CPU consumido pelo processo
    int tempo_restante_bloqueio;    // Unidades de tempo restantes para o bloqueio (instrução 'B')
    int tempo_usado_no_quantum_atual; // Tempo de CPU usado na fatia de tempo corrente

} ProcessoSimulado_t;

// --- Funções de Lista de Instruções (uso interno e para 'F') ---
void psInicializarListaInstrucoes(ListaInstrucoes_t *lista);
void psLiberarListaInstrucoes(ListaInstrucoes_t *lista);
void psInserirInstrucao(ListaInstrucoes_t *lista, Instrucao_t inst);
void psCopiarListaInstrucoes(ListaInstrucoes_t *destino, const ListaInstrucoes_t *origem); // Cópia profunda para 'F'

// --- Funções Principais do Processo Simulado (interface para o Gerenciador) ---

// Aloca e inicializa uma nova estrutura ProcessoSimulado_t.
ProcessoSimulado_t* psCriarNovo(int pid_sugerido, int pid_pai, int prioridade_inicial, long tempo_criacao);

// Carrega o programa de um arquivo para a lista de instruções do processo.
// Limpa o programa anterior e reseta PC/memória (usado para 'R' e carga inicial).
void psCarregarProgramaDeArquivo(ProcessoSimulado_t *p, const char* nome_arquivo_programa);

// Libera a memória alocada para um ProcessoSimulado_t, incluindo sua lista de instruções.
void psLiberarMemoria(ProcessoSimulado_t *p);

// Executa a próxima instrução do processo 'p' apontada pelo seu PC.
// Atualiza o estado do processo (p->estado_atual) e o PC conforme a instrução.
// Se a instrução for 'F', aloca um novo processo filho e o retorna via 'novo_processo_filho_ptr'.
// O chamador (Gerenciador) é responsável por gerenciar o 'novo_processo_filho_ptr'.
void psExecutarProximaInstrucao(ProcessoSimulado_t *p, long tempo_global_simulador, ProcessoSimulado_t **novo_processo_filho_ptr);

void psImprimirInstrucoes(const ProcessoSimulado_t *p); // Função para debug

#endif // PROCESSO_SIMULADO_H