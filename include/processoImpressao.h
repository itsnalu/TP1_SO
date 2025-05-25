#ifndef PROCESSO_IMPRESSAO_H
#define PROCESSO_IMPRESSAO_H

#include "config.h" // Para declaração extern de impressao_mutex 
#include <pthread.h> // Para pthread_t


struct GerenciadorDeProcessos_s;

// Estrutura para passar argumentos para a thread de impressão.
// Substitui a antiga ProcessoImpressao_t para este propósito.
typedef struct {
    struct GerenciadorDeProcessos_s *gerenciador; // Ponteiro para a estrutura principal do gerenciador
    int tipo_impressao;                           // 0 para impressão de estado atual, 1 para estatísticas finais
} ImpressaoArgs_t;

// Inicia o processo de impressão.
// Esta função agora criará uma thread destacada que executará a lógica de impressão.
void processoImpressaoIniciar(struct GerenciadorDeProcessos_s *gerenciador, int tipo_impressao);

// Imprime o estado atual do sistema.
// Chamada pela thread de impressão. Recebe o ponteiro para o gerenciador.
void processoImpressaoImprimirEstado(struct GerenciadorDeProcessos_s *gerenciador_ptr);

// Imprime as estatísticas finais do sistema.
// Chamada pela thread de impressão. Recebe o ponteiro para o gerenciador.
void processoImpressaoImprimirEstatisticas(struct GerenciadorDeProcessos_s *gerenciador_ptr);

#endif // PROCESSO_IMPRESSAO_H