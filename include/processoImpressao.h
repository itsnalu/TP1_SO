#ifndef PROCESSO_IMPRESSAO_H
#define PROCESSO_IMPRESSAO_H

#include "config.h"
#include "processoSimulado.h"
#include "gerenciador.h"
#include "fila.h"
#include "cpu.h"
#include "estados.h"
#include <semaphore.h>

struct GerenciadorDeProcessos_s;
typedef struct GerenciadorDeProcessos_s GerenciadorDeProcessos_t;

// Declaração do semáforo como variável global
extern sem_t sem_impressao;

typedef struct {
    int pid;                    // Identificador do processo de impressão
    GerenciadorDeProcessos_t *gerenciador; // Ponteiro para o gerenciador
    int tipo_impressao; // 0 = normal, 1 = final
} ProcessoImpressao_t;

// Inicia o processo de impressão
void processoImpressaoIniciar(GerenciadorDeProcessos_t *gerenciador, int tipo_impressao);

// Imprime o estado atual do sistema
void processoImpressaoImprimirEstado(ProcessoImpressao_t *impressao);

// Imprime as estatísticas do sistema
void processoImpressaoImprimirEstatisticas(ProcessoImpressao_t *impressao);

// Finaliza o processo de impressão
void processoImpressaoFinalizar(ProcessoImpressao_t *impressao);

#endif // PROCESSO_IMPRESSAO_H 