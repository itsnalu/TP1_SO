#include "../include/gerenciador.h"
#include "../include/threads.h"
#include "../include/processoSimulado.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void inicializarThreads(GerenciadorDeProcessos_t* gerenciador) {
    // Inicializa mutexes
    pthread_mutex_init(&gerenciador->mutex_cpu, NULL);
    pthread_mutex_init(&gerenciador->mutex_fila_prontos, NULL);
    pthread_mutex_init(&gerenciador->mutex_fila_bloqueados, NULL);
    
    // Inicializa semáforo de impressão
    sem_init(&gerenciador->sem_impressao, 0, 1);
    
    // Inicializa comunicação entre threads
    pthread_mutex_init(&gerenciador->comunicacao.mutex, NULL);
    pthread_cond_init(&gerenciador->comunicacao.cond, NULL);
    gerenciador->comunicacao.comando_recebido = 0;
}

void finalizarThreads(GerenciadorDeProcessos_t* gerenciador) {
    // Destrói mutexes
    pthread_mutex_destroy(&gerenciador->mutex_cpu);
    pthread_mutex_destroy(&gerenciador->mutex_fila_prontos);
    pthread_mutex_destroy(&gerenciador->mutex_fila_bloqueados);
    
    // Destrói semáforo
    sem_destroy(&gerenciador->sem_impressao);
    
    // Destrói comunicação entre threads
    pthread_mutex_destroy(&gerenciador->comunicacao.mutex);
    pthread_cond_destroy(&gerenciador->comunicacao.cond);
}

void* executarProcessoThread(void* arg) {
    ThreadArgs_t* args = (ThreadArgs_t*)arg;
    ProcessoSimulado_t* processo = args->processo;
    GerenciadorDeProcessos_t* gerenciador = args->gerenciador;
    
    printf("[Thread] Iniciando execução do processo %d\n", processo->pid);
    
    while (processo->estado_atual != EST_TERMINADO) {
        // Aguarda sua vez de executar
        pthread_mutex_lock(&gerenciador->comunicacao.mutex);
        
        while (processo->estado_atual != EST_EXECUCAO) {
            pthread_cond_wait(&gerenciador->comunicacao.cond, &gerenciador->comunicacao.mutex);
        }
        
        pthread_mutex_unlock(&gerenciador->comunicacao.mutex);
        
        if (processo->estado_atual == EST_EXECUCAO) {
            // Executa uma instrução
            ProcessoSimulado_t* novo_filho = NULL;
            psExecutarProximaInstrucao(processo, gerenciador->tempo_simulacao_global, &novo_filho);
            
            // Trata criação de processo filho
            if (novo_filho) {
                pthread_mutex_lock(&gerenciador->comunicacao.mutex);
                // Lógica para criar thread do filho
                ThreadArgs_t* args_filho = malloc(sizeof(ThreadArgs_t));
                args_filho->processo = novo_filho;
                args_filho->gerenciador = gerenciador;
                
                pthread_t thread_filho;
                if (pthread_create(&thread_filho, NULL, executarProcessoThread, args_filho) != 0) {
                    perror("Erro ao criar thread do processo filho");
                    free(args_filho);
                }
                pthread_mutex_unlock(&gerenciador->comunicacao.mutex);
            }
            
            // Verifica se terminou ou bloqueou
            if (processo->estado_atual == EST_TERMINADO) {
                break;
            }
        }
        
        // Pequena pausa para não sobrecarregar a CPU
        usleep(1000);
    }
    
    printf("[Thread] Processo %d finalizado\n", processo->pid);
    free(args);
    return NULL;
}

void* executarImpressaoThread(void* arg) {
    ProcessoImpressao_t* impressao = (ProcessoImpressao_t*)arg;
    
    // Adquire mutex para garantir exclusão mútua na impressão
    pthread_mutex_lock(&impressao->gerenciador->mutex_cpu);
    
    if (impressao->tipo_impressao == 0) {
        processoImpressaoImprimirEstado(impressao);
    } else {
        processoImpressaoImprimirEstatisticas(impressao);
    }
    
    pthread_mutex_unlock(&impressao->gerenciador->mutex_cpu);
    
    free(impressao);
    return NULL;
}