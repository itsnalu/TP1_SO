#include "../include/config.h"
#include "../include/processos.h"

void inicializarGerenciador(Processos *gerenciador) {
    gerenciador->numProcessos = 0;
    gerenciador->capacidade = MAX_PROCESSOS;
    
    // Aloca memória para o vetor de processos
    gerenciador->processos = malloc(gerenciador->capacidade * sizeof(ProcessoSimulado_t*));
    if (gerenciador->processos == NULL) {
        perror("Erro ao alocar memória para os processos");
        exit(EXIT_FAILURE);
    }
}

ProcessoSimulado_t* criarNovoProcesso() {
    ProcessoSimulado_t *novo_processo = malloc(sizeof(ProcessoSimulado_t));
    if (novo_processo == NULL) {
        perror("Erro ao alocar memória para o novo processo");
        exit(EXIT_FAILURE);
    }
    
    // Inicialize os valores do processo (por exemplo, setando o contador de programa)
    novo_processo->pc = 0;
    novo_processo->numProcessos = 0;
    // Outras inicializações, conforme necessário

    return novo_processo;
}

void adicionarProcesso(Processos *gerenciador, ProcessoSimulado_t *processo) {
    if (gerenciador->numProcessos == gerenciador->capacidade) {
        // Se o vetor de processos estiver cheio, dobra a capacidade
        gerenciador->capacidade *= 2;

        // Tentando realocar memória
        ProcessoSimulado_t **novo_endereco = realloc(gerenciador->processos, gerenciador->capacidade * sizeof(ProcessoSimulado_t *));
        if (novo_endereco == NULL) {
            perror("Erro ao realocar memória para os processos");
            exit(EXIT_FAILURE);
        }
        gerenciador->processos = novo_endereco;
    }
    
    // Adiciona o processo no vetor
    gerenciador->processos[gerenciador->numProcessos++] = processo;
}
