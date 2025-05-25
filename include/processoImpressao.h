#ifndef PROCESSO_IMPRESSAO_H
#define PROCESSO_IMPRESSAO_H

#include "config.h" // Para declaração extern de impressao_mutex e NUM_NIVEIS_PRIORIDADE
#include <pthread.h> // Para pthread_t (já incluído por config.h)

// Forward declaration para evitar dependência circular se gerenciador.h incluísse este.
// (GerenciadorDeProcessos_t é definido em gerenciador.h)
struct GerenciadorDeProcessos_s;

// Estrutura para passar argumentos para a thread de impressão
typedef struct {
    struct GerenciadorDeProcessos_s *gerenciador; // Ponteiro para o gerenciador de processos principal
    int tipo_impressao;                   // 0 para estado atual, 1 para estatísticas finais
} ImpressaoArgs_t;

void processoImpressaoIniciar(struct GerenciadorDeProcessos_s *gerenciador, int tipo_impressao);
void processoImpressaoImprimirEstado(struct GerenciadorDeProcessos_s *gerenciador_ptr);
void processoImpressaoImprimirEstatisticas(struct GerenciadorDeProcessos_s *gerenciador_ptr);

#endif // PROCESSO_IMPRESSAO_H