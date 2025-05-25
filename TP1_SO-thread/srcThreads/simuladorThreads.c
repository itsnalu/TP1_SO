#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "../include/gerenciadorThreads.h"
#include "../include/processoSimulado.h"
#include "../include/processoImpressao.h"
#include "../include/config.h"

sem_t sem_impressao;

int main(int argc, char *argv[]) {
    // Inicializa o semáforo de impressão
    sem_init(&sem_impressao, 0, 1);

    printf("\n===== Simulador de Gerenciamento de Processos com Threads =====\n\n");

    // Carrega o processo inicial
    char nome_arquivo[256];
    printf("Digite o nome do arquivo que contém as instruções para o processo inicial (ou Enter para usar 'init.txt'): ");
    if (fgets(nome_arquivo, sizeof(nome_arquivo), stdin) != NULL) {
        nome_arquivo[strcspn(nome_arquivo, "\n")] = 0;
    }
    if (strlen(nome_arquivo) == 0) {
        strcpy(nome_arquivo, "init.txt");
        printf("Usando arquivo de instruções padrão: %s\n", nome_arquivo);
    }

    // Cria o processo inicial
    ProcessoSimulado_t *processo_inicial = (ProcessoSimulado_t *)malloc(sizeof(ProcessoSimulado_t));
    if (!processo_inicial) {
        perror("Erro ao alocar memória para o processo inicial");
        return 1;
    }

    // Inicializa o processo
    processo_inicial->pid = 0;
    processo_inicial->pid_pai = -1;
    processo_inicial->estado_atual = EST_PRONTO;
    processo_inicial->prioridade = 0;
    processo_inicial->pc = 0;
    processo_inicial->tempo_chegada_sistema = 0;
    processo_inicial->tempo_total_cpu_usado = 0;
    processo_inicial->tempo_restante_bloqueio = 0;
    processo_inicial->tempo_usado_no_quantum_atual = 0;
    processo_inicial->numProcessos = 0;
    processo_inicial->num_variaveis_declaradas = -1;  // Indica que instrução N não foi executada ainda

    // Inicializa a lista de instruções
    processo_inicial->listaInstrucoes.primeiro = NULL;
    processo_inicial->listaInstrucoes.ultimo = NULL;
    processo_inicial->listaInstrucoes.tamanho = 0;

    // Carrega o programa do arquivo
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) {
        printf("Erro ao abrir o arquivo %s\n", nome_arquivo);
        free(processo_inicial);
        return 1;
    }

    // Lê as instruções do arquivo
    char linha[256];
    while (fgets(linha, sizeof(linha), arquivo)) {
        Instrucao_t instrucao;
        memset(&instrucao, 0, sizeof(Instrucao_t));

        char tipo;
        int arg1 = 0, arg2 = 0;
        char nomeArq[MAX_NOME_ARQUIVO_R] = "";

        int campos_lidos = sscanf(linha, "%c %d %d %s", &tipo, &arg1, &arg2, nomeArq);
        
        instrucao.tipoInstrucaoChar = tipo;
        instrucao.arg1 = arg1;
        instrucao.arg2 = arg2;

        if (tipo == 'R' && campos_lidos == 4) {
            strncpy(instrucao.nome_arquivo_R, nomeArq, MAX_NOME_ARQUIVO_R - 1);
        }

        // Adiciona a instrução na lista
        CelulaInstrucao_t *nova = malloc(sizeof(CelulaInstrucao_t));
        if (!nova) {
            perror("Erro ao alocar memória para instrução");
            fclose(arquivo);
            free(processo_inicial);
            return 1;
        }

        nova->instrucao = instrucao;
        nova->prox = NULL;

        if (processo_inicial->listaInstrucoes.primeiro == NULL) {
            processo_inicial->listaInstrucoes.primeiro = nova;
        } else {
            processo_inicial->listaInstrucoes.ultimo->prox = nova;
        }
        processo_inicial->listaInstrucoes.ultimo = nova;
        processo_inicial->listaInstrucoes.tamanho++;
    }

    fclose(arquivo);

    if (processo_inicial->listaInstrucoes.tamanho == 0) {
        printf("Erro: arquivo %s está vazio ou não contém instruções válidas\n", nome_arquivo);
        free(processo_inicial);
        return 1;
    }

    printf("Programa inicial '%s' carregado: %d instruções.\n\n", 
           nome_arquivo, processo_inicial->listaInstrucoes.tamanho);

    // Cria o pipe para comunicação
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Erro ao criar pipe");
        free(processo_inicial);
        return 1;
    }

    // Inicia o gerenciador de threads
    printf("Iniciando simulação com threads...\n");
    iniciar_gerenciador_threads(pipe_fd[0], processo_inicial);

    // Fecha os pipes e libera recursos
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    sem_destroy(&sem_impressao);

    // Libera a memória da lista de instruções
    CelulaInstrucao_t *atual = processo_inicial->listaInstrucoes.primeiro;
    while (atual != NULL) {
        CelulaInstrucao_t *temp = atual;
        atual = atual->prox;
        free(temp);
    }

    free(processo_inicial);

    printf("\n===== Simulação com Threads Concluída =====\n");
    return 0;
} 