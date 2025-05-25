#include "../include/gerenciador.h"
#include "../include/threads.h"
#include "../include/processoSimulado.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

// Array para armazenar as threads ativas
static pthread_t threads_ativas[MAX_THREADS];
static int num_threads_ativas = 0;
static pthread_mutex_t mutex_threads = PTHREAD_MUTEX_INITIALIZER;

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
    printf("[Thread] Finalizando todas as threads...\n");
    
    // Aguarda todas as threads terminarem
    pthread_mutex_lock(&mutex_threads);
    for (int i = 0; i < num_threads_ativas; i++) {
        printf("[Thread] Aguardando thread %lu terminar...\n", threads_ativas[i]);
        if (pthread_join(threads_ativas[i], NULL) != 0) {
            perror("Erro ao aguardar thread");
        }
    }
    num_threads_ativas = 0;
    pthread_mutex_unlock(&mutex_threads);
    
    // Destrói os mutexes e semáforos
    pthread_mutex_destroy(&mutex_threads);
    pthread_mutex_destroy(&gerenciador->mutex_cpu);
    sem_destroy(&gerenciador->sem_impressao);
    
    printf("[Thread] Todas as threads finalizadas\n");
}

// Função para registrar uma nova thread
static void registrarThread(pthread_t thread) {
    pthread_mutex_lock(&mutex_threads);
    if (num_threads_ativas < MAX_THREADS) {
        threads_ativas[num_threads_ativas++] = thread;
    }
    pthread_mutex_unlock(&mutex_threads);
}

// Função para remover uma thread do registro
static void removerThread(pthread_t thread) {
    pthread_mutex_lock(&mutex_threads);
    for (int i = 0; i < num_threads_ativas; i++) {
        if (pthread_equal(threads_ativas[i], thread)) {
            // Move a última thread para a posição atual
            if (i < num_threads_ativas - 1) {
                threads_ativas[i] = threads_ativas[num_threads_ativas - 1];
            }
            num_threads_ativas--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_threads);
}

void* executarProcessoThread(void* arg) {
    ThreadArgs_t* args = (ThreadArgs_t*)arg;
    ProcessoSimulado_t* processo = args->processo;
    GerenciadorDeProcessos_t* gerenciador = args->gerenciador;
    
    // Registra a thread
    registrarThread(pthread_self());
    
    printf("[Thread] Iniciando execução do processo %d\n", processo->pid);
    
    while (processo->estado_atual != EST_TERMINADO) {
        // Aguarda sua vez de executar
        pthread_mutex_lock(&gerenciador->comunicacao.mutex);
        
        while (processo->estado_atual != EST_EXECUCAO) {
            // Usa timedwait para evitar deadlock
            struct timespec timeout;
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += 1; // 1 segundo de timeout
            
            int ret = pthread_cond_timedwait(&gerenciador->comunicacao.cond, 
                                           &gerenciador->comunicacao.mutex, 
                                           &timeout);
            
            if (ret == ETIMEDOUT) {
                printf("[Thread] Timeout aguardando execução do processo %d\n", processo->pid);
                break;
            }
        }
        
        pthread_mutex_unlock(&gerenciador->comunicacao.mutex);
        
        if (processo->estado_atual == EST_EXECUCAO) {
            // Executa uma instrução
            ProcessoSimulado_t* novo_filho = NULL;
            
            // Protege a execução da instrução
            pthread_mutex_lock(&gerenciador->mutex_cpu);
            psExecutarProximaInstrucao(processo, gerenciador->tempo_simulacao_global, &novo_filho);
            pthread_mutex_unlock(&gerenciador->mutex_cpu);
            
            // Trata criação de processo filho
            if (novo_filho) {
                pthread_mutex_lock(&gerenciador->comunicacao.mutex);
                // Lógica para criar thread do filho
                ThreadArgs_t* args_filho = malloc(sizeof(ThreadArgs_t));
                if (args_filho) {
                    args_filho->processo = novo_filho;
                    args_filho->gerenciador = gerenciador;
                    
                    pthread_t thread_filho;
                    if (pthread_create(&thread_filho, NULL, executarProcessoThread, args_filho) != 0) {
                        perror("Erro ao criar thread do processo filho");
                        free(args_filho);
                    } else {
                        // Registra a thread filho
                        registrarThread(thread_filho);
                    }
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
    
    // Remove a thread do registro antes de finalizar
    removerThread(pthread_self());
    
    free(args);
    return NULL;
}

void* executarImpressaoThread(void* arg) {
    ProcessoImpressao_t* processo = (ProcessoImpressao_t*)arg;
    GerenciadorDeProcessos_t* gerenciador = processo->gerenciador;
    
    // Registra a thread
    registrarThread(pthread_self());
    
    printf("[Thread] Iniciando processo de impressão\n");
    
    // Tenta obter o semáforo com timeout
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 2; // 2 segundos de timeout
    
    if (sem_timedwait(&gerenciador->sem_impressao, &timeout) == -1) {
        if (errno == ETIMEDOUT) {
            printf("[Thread] Timeout ao tentar obter semáforo de impressão\n");
            free(processo);
            removerThread(pthread_self());
            return NULL;
        }
        perror("Erro ao obter semáforo de impressão");
        free(processo);
        removerThread(pthread_self());
        return NULL;
    }
    
    // Protege a execução da impressão
    pthread_mutex_lock(&gerenciador->mutex_cpu);
    
    // Simula o tempo de impressão
    printf("[Thread] Imprimindo...\n");
    sleep(2);
    
    pthread_mutex_unlock(&gerenciador->mutex_cpu);
    
    // Libera o semáforo
    sem_post(&gerenciador->sem_impressao);
    
    printf("[Thread] Processo de impressão finalizado\n");
    
    // Remove a thread do registro antes de finalizar
    removerThread(pthread_self());
    
    free(processo);
    return NULL;
}