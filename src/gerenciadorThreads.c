#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "../include/gerenciadorThreads.h"
#include "../include/config.h"
#include "../include/processoSimulado.h"
#include "../include/processoImpressao.h"
#include "../include/filaThreads.h"
#include "../include/processosThreads.h"
#include "../include/estadosThreads.h"

#define MAX_PROCESSOS 100

// Estrutura do gerenciador de threads
typedef struct {
    pthread_mutex_t mutex;
    pthread_t escalonador;
    int executando;
    int num_processos;
    ProcessoSimulado_t* processos[MAX_PROCESSOS];
} GerenciadorThreads_t;

// Variável global do gerenciador
static GerenciadorThreads_t gerenciador;
extern sem_t sem_impressao;

// Função auxiliar para executar instruções
static void executarInstrucaoThread(ProcessoSimulado_t *processo, Instrucao_t *instrucao) {
    switch (instrucao->tipoInstrucaoChar) {
        case 'U':  // Usar CPU
            sem_wait(&sem_impressao);
            printf("[Thread] PID %d executando instrução U %d\n", processo->pid, instrucao->arg1);
            sem_post(&sem_impressao);
            usleep(instrucao->arg1 * 1000);  // Simula uso da CPU
            break;
            
        case 'I':  // Operação de I/O
            sem_wait(&sem_impressao);
            printf("[Thread] PID %d realizando I/O por %d ms\n", processo->pid, instrucao->arg1);
            sem_post(&sem_impressao);
            processo->estado_atual = EST_BLOQUEADO;
            processo->tempo_restante_bloqueio = instrucao->arg1;
            filaThreadsEnfileirar(&gerenciador.fila_bloqueados, processo->pid);
            usleep(instrucao->arg1 * 1000);  // Simula I/O
            break;
            
        case 'R':  // Substituir imagem
            if (instrucao->nome_arquivo_R[0] != '\0') {
                sem_wait(&sem_impressao);
                printf("[Thread] PID %d substituindo imagem com %s\n", 
                       processo->pid, instrucao->nome_arquivo_R);
                sem_post(&sem_impressao);
                psCarregarProgramaDeArquivo(processo, instrucao->nome_arquivo_R);
            }
            break;
    }
}

// Thread principal de cada processo
static void* threadProcessoExecucao(void* arg) {
    ThreadProcesso_t* tp = (ThreadProcesso_t*)arg;
    ProcessoSimulado_t* processo = tp->processo;
    
    pthread_mutex_lock(&gerenciador.mutex);
    processo->estado_atual = EST_PRONTO;
    filaThreadsEnfileirar(&gerenciador.fila_prontos, processo->pid);
    pthread_mutex_unlock(&gerenciador.mutex);
    
    while (processo->estado_atual != EST_TERMINADO) {
        pthread_mutex_lock(&gerenciador.mutex);
        
        if (tp->em_execucao && processo->pc < processo->listaInstrucoes.tamanho) {
            CelulaInstrucao_t* celula = processo->listaInstrucoes.primeiro;
            for (int i = 0; i < processo->pc && celula != NULL; i++) {
                celula = celula->prox;
            }
            
            if (celula != NULL) {
                executarInstrucaoThread(processo, &celula->instrucao);
                processo->pc++;
                processo->tempo_total_cpu_usado++;
                
                if (processo->pc >= processo->listaInstrucoes.tamanho) {
                    processo->estado_atual = EST_TERMINADO;
                    tp->em_execucao = 0;
                    sem_wait(&sem_impressao);
                    printf("[Thread] PID %d finalizou execução\n", processo->pid);
                    sem_post(&sem_impressao);
                }
            }
        }
        
        pthread_mutex_unlock(&gerenciador.mutex);
        usleep(1000);  // 1ms entre verificações
    }
    
    return NULL;
}

// Função do escalonador
void* escalonadorExecutar(void* arg) {
    GerenciadorThreads_t* gerenciador = (GerenciadorThreads_t*)arg;
    
    while (1) {
        pthread_mutex_lock(&gerenciador->mutex);
        
        // Procura o próximo processo pronto
        int proximo = -1;
        for (int i = 0; i < gerenciador->num_processos; i++) {
            if (processosThreadsObterEstado(i) == EST_PRONTO_THREADS) {
                proximo = i;
                    break;
                }
            }
        
        // Se encontrou um processo pronto
        if (proximo >= 0) {
            // Coloca o processo em execução
            processosThreadsDefinirEstado(proximo, EST_EXECUCAO_THREADS);
            gerenciador->executando = proximo;
            printf("Processo %d começou a executar\n", proximo);
        }
        
        pthread_mutex_unlock(&gerenciador->mutex);
        
        // Pequena pausa para não sobrecarregar a CPU
        struct timespec ts = {0, 100000000}; // 100ms
        nanosleep(&ts, NULL);
    }
    
    return NULL;
}

// Inicializa o gerenciador de threads
GerenciadorThreads_t* gerenciadorThreadsInicializar(void) {
    GerenciadorThreads_t* gerenciador = malloc(sizeof(GerenciadorThreads_t));
    if (!gerenciador) return NULL;
    
    gerenciador->executando = -1;
    gerenciador->num_processos = 0;
    pthread_mutex_init(&gerenciador->mutex, NULL);
    
    // Inicializa o array de processos
    for (int i = 0; i < MAX_PROCESSOS; i++) {
        gerenciador->processos[i] = NULL;
    }
    
    // Cria a thread do escalonador
    if (pthread_create(&gerenciador->escalonador, NULL, escalonadorExecutar, gerenciador) != 0) {
        pthread_mutex_destroy(&gerenciador->mutex);
        free(gerenciador);
        return NULL;
    }
    
    return gerenciador;
}

// Finaliza o gerenciador de threads
void gerenciadorThreadsFinalizar(GerenciadorThreads_t* gerenciador) {
    if (!gerenciador) return;
    
    // Finaliza todos os processos
    for (int i = 0; i < gerenciador->num_processos; i++) {
        if (gerenciador->processos[i]) {
            processoSimuladoFinalizar(gerenciador->processos[i]);
        }
    }
    
    // Cancela a thread do escalonador
    pthread_cancel(gerenciador->escalonador);
    pthread_join(gerenciador->escalonador, NULL);
    
    // Limpa os recursos
    pthread_mutex_destroy(&gerenciador->mutex);
    free(gerenciador);
}

// Adiciona um novo processo ao gerenciador
int gerenciadorThreadsAdicionarProcesso(GerenciadorThreads_t* gerenciador, ProcessoSimulado_t* processo) {
    if (!gerenciador || !processo) return -1;
    
    pthread_mutex_lock(&gerenciador->mutex);
    
    // Verifica se há espaço
    if (gerenciador->num_processos >= MAX_PROCESSOS) {
        pthread_mutex_unlock(&gerenciador->mutex);
        return -1;
    }
    
    // Adiciona o processo
    gerenciador->processos[gerenciador->num_processos] = processo;
    gerenciador->num_processos++;
    
    pthread_mutex_unlock(&gerenciador->mutex);
    return 0;
}

// Imprime o estado atual do gerenciador
void gerenciadorThreadsImprimirEstado(GerenciadorThreads_t* gerenciador) {
    if (!gerenciador) return;
    
    pthread_mutex_lock(&gerenciador->mutex);
    
    printf("\nEstado do Gerenciador de Threads:\n");
    printf("Processo em execução: %d\n", gerenciador->executando);
    printf("Número total de processos: %d\n", gerenciador->num_processos);
    
    for (int i = 0; i < gerenciador->num_processos; i++) {
        printf("Processo %d - Estado: %d\n", i, processosThreadsObterEstado(i));
    }
    
    pthread_mutex_unlock(&gerenciador->mutex);
}

// Funções stub para manter compatibilidade com a interface
void gerenciadorThreadsEscalonar(void) {
    // Implementação vazia - escalonamento é feito pela thread dedicada
    printf("[Thread] Escalonamento sendo realizado pela thread dedicada\n");
}

void gerenciadorThreadsTrocarContexto(void) {
    // Implementação vazia - troca de contexto é feita pela thread dedicada
    printf("[Thread] Troca de contexto sendo realizada pela thread dedicada\n");
} 