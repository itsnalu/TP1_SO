#ifndef GERENCIADOR_H
#define GERENCIADOR_H

#include "config.h"           // Para constantes e declarações extern de mutexes globais
#include "processoSimulado.h" // Para ProcessoSimulado_t e ThreadArgs_t
#include "cpu.h"              // Para CPU_t (estrutura original da "Ana")
#include "estados.h"          // Para EstadoPronto_t, EstadoBloqueado_t
// #include "lista.h"         // Incluir se Lista* tabela_processos for mantida e usada
// #include "threads.h"       // Incluir para o protótipo de finalizarThreads, se estiver em threads.h

// Estrutura que representa o gerenciador de processos
typedef struct GerenciadorDeProcessos_s {
    long tempo_simulacao_global; // Contador global de unidades de tempo da simulação

    CPU_t cpu_sistema; // A unidade de CPU simulada (estrutura original da "Ana").
                       // Seu campo cpu_sistema.indice_processo_na_tabela pode ser usado
                       // para espelhar o processo_ativo_pid.

    int processo_ativo_pid; // PID do processo simulado cuja thread está "na CPU".
                            // -1 se a CPU estiver conceitualmente ociosa.

    // Tabela principal de todos os processos existentes no sistema (array).
    ProcessoSimulado_t tabela_de_processos[MAX_PROCESSOS_SIMULADOS_NO_SISTEMA];
    // Controle de quais slots da tabela_de_processos (array) estão em uso.
    char slot_tabela_ocupado[MAX_PROCESSOS_SIMULADOS_NO_SISTEMA]; 
    
    // int proximo_pid_a_ser_alocado; // Lógica de alocação de PID movida para configurarNovoProcessoNaTabela.
    // Lista* tabela_processos; // Campo original da "Ana" para uma lista encadeada de processos.
                             // Avaliar se ainda é necessário com o uso do array tabela_de_processos.
                             // Se não for, pode ser removido. Comentado por enquanto.

    EstadoPronto_t processos_prontos;       // Estrutura para gerenciar processos prontos
    EstadoBloqueado_t processos_bloqueados; // Estrutura para gerenciar processos bloqueados

    // Campos para cálculo de estatísticas (originais da "Ana")
    long acumulador_tempo_de_vida_processos_concluidos;
    int total_processos_concluidos;

    // Os seguintes campos foram removidos da struct, pois os mutexes e o semáforo
    // para impressão agora são globais e declarados 'extern' em config.h:
    // pthread_mutex_t mutex_cpu;
    // pthread_mutex_t mutex_fila_prontos;
    // pthread_mutex_t mutex_fila_bloqueados;
    // sem_t sem_impressao;

    // A estrutura ThreadComunicacao_t também foi removida, pois não é usada no novo modelo.
    // ThreadComunicacao_t comunicacao;

} GerenciadorDeProcessos_t;

// ThreadArgs_t é definida em processoSimulado.h, que é incluído acima.

// --- Protótipos das Funções Principais do Gerenciador ---

// Função principal do loop do gerenciador de processos simulados.
void gerenciadorProcessosSimulados(int fd_read, ProcessoSimulado_t *processo_inicial_info);

// Executa uma unidade de tempo da simulação (avança tempo, trata bloqueados, chama escalonador).
void executarUnidadeTempo(GerenciadorDeProcessos_t* gerenciador);

// Escalonador de processos que despacha threads.
void escalonarProcessosThreads(GerenciadorDeProcessos_t* gerenciador);

// Função auxiliar para configurar um novo processo na tabela de processos do gerenciador.
// Atribui PID, copia dados, e marca o slot como ocupado.
int configurarNovoProcessoNaTabela(GerenciadorDeProcessos_t *gerenciador,
                                     ProcessoSimulado_t *info_novo_processo, // Struct temporária com info do processo
                                     int pid_pai,
                                     int prioridade_sugerida,
                                     long tempo_chegada);

// O protótipo de finalizarThreads (se implementado em threads.c) deve ser incluído via threads.h,
// ou, se movido para gerenciador.c, declarado como static aqui. Assumimos que threads.h o fornecerá.

#endif // GERENCIADOR_H