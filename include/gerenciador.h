#ifndef GERENCIADOR_H
#define GERENCIADOR_H

#include "config.h"
#include "processoSimulado.h" // Seu módulo ProcessoSimulado_t
#include "cpu.h"              // Estrutura CPU_t
#include "estados.h"          // Estruturas EstadoPronto_t, EstadoBloqueado_t
#include "processoImpressao.h"
#include "lista.h"
#include "threads.h"
#include "fila.h"

// Estrutura que representa o gerenciador de processos
typedef struct GerenciadorDeProcessos_s {
    long tempo_simulacao_global; // Contador global de unidades de tempo da simulação

    CPU_t cpu_sistema; // A unidade de CPU simulada

    // Tabela principal de todos os processos existentes no sistema.
    ProcessoSimulado_t tabela_de_processos[MAX_PROCESSOS_SIMULADOS_NO_SISTEMA];
    // Controle de quais slots da tabela_de_processos estão em uso.
    char slot_tabela_ocupado[MAX_PROCESSOS_SIMULADOS_NO_SISTEMA];
    int proximo_pid_a_ser_alocado; // Contador para gerar PIDs únicos
    Lista* tabela_processos;

    EstadoPronto_t processos_prontos;       // Estrutura para gerenciar processos prontos
    EstadoBloqueado_t processos_bloqueados; // Estrutura para gerenciar processos bloqueados

    // Campos para cálculo de estatísticas
    long acumulador_tempo_de_vida_processos_concluidos;
    int total_processos_concluidos;

    pthread_mutex_t mutex_cpu;        // Mutex para acesso à CPU
    pthread_mutex_t mutex_fila_prontos; // Mutex para fila de prontos
    pthread_mutex_t mutex_fila_bloqueados; // Mutex para fila de bloqueados
    sem_t sem_impressao;             // Semáforo para controle de impressão

    // Comunicação entre threads
    ThreadComunicacao_t comunicacao;
} GerenciadorDeProcessos_t;

// Funções do gerenciador de processos
//função que cria um novo processo simulado
void criarProcessoSimulado(GerenciadorDeProcessos_t *gerenciador, char *nomeArquivo);
//função que substitui imagem atual de um processo para uma nova
void substituirImagemProcesso(GerenciadorDeProcessos_t *gerenciador, int pid, char *novaImagem);   
//função que gerencia estados de processos 
void gerenciarTransicoesEstados(GerenciadorDeProcessos_t *gerenciador, int pid, int novoEstado);
//função de escalonamento
void escalonarProcessos(GerenciadorDeProcessos_t *gerenciador);
//função de troca de contexto
void trocarContexto(GerenciadorDeProcessos_t *gerenciador);
//função principal do gerenciador de processos simulados
void gerenciadorProcessosSimulados(int fd_read, ProcessoSimulado_t *processo_inicial);
// Atribui um PID a um processo simulado
void atribuirPidAoProcesso(GerenciadorDeProcessos_t *gerenciador, ProcessoSimulado_t *processo);

#endif // GERENCIADOR_H