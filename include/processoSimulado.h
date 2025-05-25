#ifndef PROCESSO_SIMULADO_H
#define PROCESSO_SIMULADO_H

#include "config.h" // Para MAX_NOME_ARQUIVO_R e outras constantes
#include <pthread.h> // Para pthread_t

// Define os possíveis estados de um processo simulado.
typedef enum {
    EST_NOVO,       // Processo recém-criado, ainda não na fila de prontos ou totalmente configurado
    EST_PRONTO,     // Aguardando para usar a CPU
    EST_EXECUCAO,   // Atualmente utilizando a CPU (sua thread está rodando)
    EST_BLOQUEADO,  // Aguardando um evento externo (ex: I/O simulado pela instrução 'B')
    EST_TERMINADO   // Execução concluída ou abortada
} EstadoProcesso_e;

// Função para converter o enum do estado para string (útil para logs)
const char* estadoParaString(EstadoProcesso_e estado);

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

// Forward declaration para a estrutura GerenciadorDeProcessos_s
struct GerenciadorDeProcessos_s;

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
    
    int quantum_alocado_atual;      // Quantum que o escalonador concedeu para esta execução
    int tempo_usado_no_quantum_atual; // Tempo de CPU usado na fatia de tempo corrente (incrementado pela thread)

    // int numProcessos; // Campo original da versão "Ana". Removido para resolver Erro 3, a menos que seja necessário.

    pthread_t thread; // Thread POSIX associada a este processo simulado

} ProcessoSimulado_t;

// Estrutura para passar argumentos para a thread do processo
typedef struct {
    ProcessoSimulado_t* processo;
    struct GerenciadorDeProcessos_s* gerenciador; 
} ThreadArgs_t;


// --- Funções de Lista de Instruções ---
void psInicializarListaInstrucoes(ListaInstrucoes_t *lista);
void psLiberarListaInstrucoes(ListaInstrucoes_t *lista);
void psInserirInstrucao(ListaInstrucoes_t *lista, Instrucao_t inst);
void psCopiarListaInstrucoes(ListaInstrucoes_t *destino, const ListaInstrucoes_t *origem);

// --- Funções Principais do Processo Simulado ---
ProcessoSimulado_t* psCriarNovo(int pid_pai, int prioridade_sugerida, long tempo_criacao);
void psCarregarProgramaDeArquivo(ProcessoSimulado_t *p, const char* nome_arquivo_programa);
void psLiberarMemoria(ProcessoSimulado_t *p);

// Instrucao_t* psObterInstrucaoNoPc(const ProcessoSimulado_t *p); // REMOVIDO O PROTÓTIPO DAQUI

void psImprimirInstrucoes(const ProcessoSimulado_t *p);
void* psExecutarProcesso(void* arg);

#endif // PROCESSO_SIMULADO_H