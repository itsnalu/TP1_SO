#ifndef PROCESSO_IMPRESSAO_THREADS_H
#define PROCESSO_IMPRESSAO_THREADS_H

#include <stdarg.h>
#include "gerenciadorThreads.h"

// Forward declarations
struct ProcessoSimuladoThread_s;
typedef struct ProcessoSimuladoThread_s ProcessoSimuladoThread_t;

// Estrutura opaca do processo de impressão
typedef struct ProcessoImpressaoThread_s ProcessoImpressaoThread_t;

// Funções principais
ProcessoImpressaoThread_t* processoImpressaoThreadCriar(GerenciadorThreads_t *gerenciador, int tipo_impressao);
void processoImpressaoThreadIniciar(GerenciadorThreads_t *gerenciador, int tipo_impressao);
void processoImpressaoThreadExecutar(ProcessoImpressaoThread_t *impressao);
void processoImpressaoThreadFinalizar(ProcessoImpressaoThread_t *impressao);

// Funções de impressão
void processoImpressaoThreadImprimirEstado(ProcessoImpressaoThread_t *impressao);
void processoImpressaoThreadImprimirEstatisticas(ProcessoImpressaoThread_t *impressao);

#endif // PROCESSO_IMPRESSAO_THREADS_H 