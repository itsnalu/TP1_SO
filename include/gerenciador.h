#ifndef GERENCIADOR_H
#define GERENCIADOR_H

#include "config.h"
#include "processoSimulado.h" // Para ProcessoSimulado_t e ThreadArgs_t
#include "estados.h"          // Para EstadoPronto_t, EstadoBloqueado_t
// A CPU_Simulada_t foi simplificada e pode ser apenas um int para processo_ativo_pid
// Se precisar de cpu.h, inclua-o, mas a tendência foi simplificar.

// Estrutura que representa o gerenciador de processos
typedef struct GerenciadorDeProcessos_s {
    long tempo_simulacao_global; // Contador global de unidades de tempo

    // Tabela de Processos: armazena os PCBs simulados
    ProcessoSimulado_t tabela_de_processos[MAX_PROCESSOS_SIMULADOS_NO_SISTEMA];
    char slot_tabela_ocupado[MAX_PROCESSOS_SIMULADOS_NO_SISTEMA]; // 1 se ocupado, 0 se livre

    // Filas de Estados
    EstadoPronto_t processos_prontos;       // Agora com múltiplas filas de prioridade
    EstadoBloqueado_t processos_bloqueados; // Fila única para processos bloqueados

    // Controle da CPU Simulada
    int processo_ativo_pid; // PID do processo cuja thread está conceitualmente "na CPU"
                            // -1 se a CPU estiver ociosa.
} GerenciadorDeProcessos_t;


// --- Funções do Gerenciador ---
void gerenciadorProcessosSimulados(int fd_read_pipe, ProcessoSimulado_t *info_processo_inicial_main);
int configurarNovoProcessoNaTabela(GerenciadorDeProcessos_t *gerenciador,
                                     ProcessoSimulado_t *info_novo_processo,
                                     int pid_pai,
                                     int prioridade_inicial_sugerida,
                                     long tempo_chegada);

#endif // GERENCIADOR_H