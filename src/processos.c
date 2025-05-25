#include "../include/config.h"    // Para constantes globais, se esta parte do código as usar.
#include "../include/processos.h"   // Para a definição de Processos_s
#include "../include/processoSimulado.h" // Para a definição de ProcessoSimulado_t
#include <stdio.h>  // Para perror
#include <stdlib.h> // Para malloc, exit, realloc
#include <string.h> // Para memset

// Esta função inicializa a estrutura Processos_s, que parece ser um sistema de
// gerenciamento de processos diferente do GerenciadorDeProcessos_t principal.
void inicializarGerenciador(Processos *gerenciador) {
    if (!gerenciador) {
        fprintf(stderr, "PROCESSOS_S ERRO: Tentativa de inicializar gerenciador Processos_s nulo.\n");
        return;
    }
    gerenciador->numProcessos = 0;
    // MAX_PROCESSOS é definido em include/processos.h como 100
    gerenciador->capacidade = MAX_PROCESSOS; 
    
    gerenciador->processos = (ProcessoSimulado_t**)malloc(gerenciador->capacidade * sizeof(ProcessoSimulado_t*));
    if (gerenciador->processos == NULL) {
        perror("PROCESSOS_S ERRO: Erro ao alocar memória para o vetor de processos em Processos_s");
        exit(EXIT_FAILURE);
    }
}

// Esta função criarNovoProcesso é DIFERENTE de psCriarNovo.
// Ela é parte do sistema Processos_s.
ProcessoSimulado_t* criarNovoProcesso() {
    ProcessoSimulado_t *novo_processo = (ProcessoSimulado_t*)malloc(sizeof(ProcessoSimulado_t));
    if (novo_processo == NULL) {
        perror("PROCESSOS_S ERRO: Erro ao alocar memória para novo processo em processos.c -> criarNovoProcesso");
        exit(EXIT_FAILURE);
    }
    
    // Inicializações básicas para um ProcessoSimulado_t
    novo_processo->pid = -1; // PID deve ser atribuído pelo sistema que usa esta função
    novo_processo->pid_pai = -1; // Sem pai por padrão
    novo_processo->estado_atual = EST_NOVO; // Começa como NOVO
    novo_processo->prioridade = 0;  // Prioridade padrão (mais alta)
    novo_processo->pc = 0;
    psInicializarListaInstrucoes(&novo_processo->listaInstrucoes); // Importante inicializar a lista
    memset(novo_processo->memoria, 0, sizeof(novo_processo->memoria));
    novo_processo->num_variaveis_declaradas = 0;
    novo_processo->tempo_chegada_sistema = 0; // Ou um valor apropriado passado como argumento
    novo_processo->tempo_total_cpu_usado = 0;
    novo_processo->tempo_restante_bloqueio = 0;
    
    // Campos para o novo modelo de thread/quantum
    novo_processo->quantum_alocado_atual = 0;
    novo_processo->tempo_usado_no_quantum_atual = 0;
    novo_processo->thread = 0; // Nenhum handle de thread POSIX ainda

    return novo_processo;
}

// Adiciona um processo ao gerenciador Processos_s
void adicionarProcesso(Processos *gerenciador, ProcessoSimulado_t *processo) {
    if (!gerenciador || !processo) {
        fprintf(stderr, "PROCESSOS_S ERRO: Gerenciador ou processo nulo em adicionarProcesso.\n");
        return;
    }

    if (gerenciador->numProcessos == gerenciador->capacidade) {
        gerenciador->capacidade *= 2;
        ProcessoSimulado_t **novo_endereco = (ProcessoSimulado_t**)realloc(gerenciador->processos, gerenciador->capacidade * sizeof(ProcessoSimulado_t *));
        if (novo_endereco == NULL) {
            perror("PROCESSOS_S ERRO: Erro ao realocar memória para o vetor de processos em Processos_s");
            exit(EXIT_FAILURE);
        }
        gerenciador->processos = novo_endereco;
    }    
    gerenciador->processos[gerenciador->numProcessos++] = processo;
}