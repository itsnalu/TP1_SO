#include "../include/processoSimuladoThreads.h"
#include "../include/processoSimulado.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "../include/processosThreads.h"
#include "../include/estadosThreads.h"

// Estrutura para representar um processo simulado
typedef struct {
    int pid;
    int tempo_execucao;
    int tempo_restante;
    pthread_t thread;
    pthread_mutex_t mutex;
} ProcessoSimulado_t;

// Função auxiliar para tratamento de erros
static void psThreadLogErro(const char *funcao, int erro) {
    fprintf(stderr, "[ERRO] %s falhou: %s\n", funcao, strerror(erro));
}

ProcessoSimuladoThread_t* psThreadCriar(ProcessoSimulado_t *processo) {
    ProcessoSimuladoThread_t *pt = malloc(sizeof(ProcessoSimuladoThread_t));
    if (!pt) {
        psThreadLogErro("malloc", errno);
        return NULL;
    }
    
    memset(pt, 0, sizeof(ProcessoSimuladoThread_t));
    pt->processo = processo;
    pt->executando = 0;
    pt->quantum_restante = 0;
    pt->deve_terminar = 0;
    
    return pt;
}

int psThreadInicializar(ProcessoSimuladoThread_t *pt) {
    int erro;
    
    if (!pt) return -1;
    
    // Inicializa mutex
    erro = pthread_mutex_init(&pt->mutex, NULL);
    if (erro) {
        psThreadLogErro("pthread_mutex_init", erro);
        return -1;
    }
    
    // Inicializa semáforo
    erro = sem_init(&pt->semaforo, 0, 0);
    if (erro) {
        psThreadLogErro("sem_init", errno);
        pthread_mutex_destroy(&pt->mutex);
        return -1;
    }
    
    return 0;
}

int psThreadExecutar(ProcessoSimuladoThread_t *pt) {
    int erro;
    
    if (!pt) return -1;
    
    pthread_mutex_lock(&pt->mutex);
    if (!pt->executando) {
        pt->executando = 1;
        erro = pthread_create(&pt->thread_id, NULL, psThreadFuncaoExecucao, pt);
        if (erro) {
            psThreadLogErro("pthread_create", erro);
            pt->executando = 0;
            pthread_mutex_unlock(&pt->mutex);
            return -1;
        }
    }
    pthread_mutex_unlock(&pt->mutex);
    
    return 0;
}

int psThreadPausar(ProcessoSimuladoThread_t *pt) {
    if (!pt) return -1;
    
    pthread_mutex_lock(&pt->mutex);
    if (pt->executando) {
        pt->executando = 0;
    }
    pthread_mutex_unlock(&pt->mutex);
    
    return 0;
}

int psThreadRetomar(ProcessoSimuladoThread_t *pt) {
    if (!pt) return -1;
    
    pthread_mutex_lock(&pt->mutex);
    if (!pt->executando) {
        pt->executando = 1;
        sem_post(&pt->semaforo);
    }
    pthread_mutex_unlock(&pt->mutex);
    
    return 0;
}

int psThreadFinalizar(ProcessoSimuladoThread_t *pt) {
    if (!pt) return -1;
    
    pthread_mutex_lock(&pt->mutex);
    pt->deve_terminar = 1;
    pt->executando = 0;
    sem_post(&pt->semaforo); // Libera a thread se estiver bloqueada
    pthread_mutex_unlock(&pt->mutex);
    
    // Espera a thread terminar
    if (pt->thread_id) {
        pthread_join(pt->thread_id, NULL);
    }
    
    return 0;
}

void psThreadDestruir(ProcessoSimuladoThread_t *pt) {
    if (!pt) return;
    
    psThreadFinalizar(pt);
    pthread_mutex_destroy(&pt->mutex);
    sem_destroy(&pt->semaforo);
    free(pt);
}

void* psThreadFuncaoExecucao(void *arg) {
    ProcessoSimuladoThread_t *pt = (ProcessoSimuladoThread_t*)arg;
    
    while (!pt->deve_terminar) {
        pthread_mutex_lock(&pt->mutex);
        while (!pt->executando && !pt->deve_terminar) {
            pthread_mutex_unlock(&pt->mutex);
            sem_wait(&pt->semaforo);
            pthread_mutex_lock(&pt->mutex);
        }
        
        if (pt->deve_terminar) {
            pthread_mutex_unlock(&pt->mutex);
            break;
        }
        
        // Executa uma instrução do processo se houver quantum disponível
        if (pt->quantum_restante > 0) {
            ProcessoSimulado_t *novo_processo = NULL;
            psExecutarProximaInstrucao(pt->processo, 0, &novo_processo);
            pt->quantum_restante--;
        }
        
        pthread_mutex_unlock(&pt->mutex);
        usleep(1000); // Pequena pausa para não sobrecarregar a CPU
    }
    
    return NULL;
}

void psThreadAtualizarQuantum(ProcessoSimuladoThread_t *pt, int novo_quantum) {
    if (!pt) return;
    
    pthread_mutex_lock(&pt->mutex);
    pt->quantum_restante = novo_quantum;
    pthread_mutex_unlock(&pt->mutex);
}

int psThreadQuantumEsgotado(const ProcessoSimuladoThread_t *pt) {
    if (!pt) return 1;
    return pt->quantum_restante <= 0;
}

void psThreadAtualizarEstado(ProcessoSimuladoThread_t *pt, int novo_estado) {
    if (!pt || !pt->processo) return;
    
    pthread_mutex_lock(&pt->mutex);
    pt->processo->estado_atual = novo_estado;
    pthread_mutex_unlock(&pt->mutex);
}

void psThreadLogEstado(const ProcessoSimuladoThread_t *pt, const char *mensagem) {
    if (!pt || !mensagem) return;
    
    printf("[Thread PID %d] %s (Executando: %d, Quantum: %d)\n",
           pt->processo ? pt->processo->pid : -1,
           mensagem,
           pt->executando,
           pt->quantum_restante);
}

// Função que será executada pela thread do processo
void* processoSimuladoExecutar(void* arg) {
    ProcessoSimulado_t* processo = (ProcessoSimulado_t*)arg;
    
    while (processo->tempo_restante > 0) {
        pthread_mutex_lock(&processo->mutex);
        
        // Verifica se o processo está em execução
        if (processosThreadsObterEstado(processo->pid) == EST_EXECUCAO_THREADS) {
            processo->tempo_restante--;
            processosThreadsIncrementarTempoCPU(processo->pid);
            
            // Se terminou a execução
            if (processo->tempo_restante <= 0) {
                processosThreadsDefinirEstado(processo->pid, EST_TERMINADO_THREADS);
                printf("Processo %d terminou sua execução\n", processo->pid);
            }
        }
        
        pthread_mutex_unlock(&processo->mutex);
        
        // Pequena pausa para simular processamento
        struct timespec ts = {0, 100000000}; // 100ms
        nanosleep(&ts, NULL);
    }
    
    return NULL;
}

// Cria um novo processo simulado
ProcessoSimulado_t* processoSimuladoCriar(int tempo_execucao) {
    ProcessoSimulado_t* processo = malloc(sizeof(ProcessoSimulado_t));
    if (!processo) return NULL;
    
    processo->tempo_execucao = tempo_execucao;
    processo->tempo_restante = tempo_execucao;
    processo->pid = processosThreadsCriar(0); // Prioridade 0 por padrão
    
    if (processo->pid < 0) {
        free(processo);
        return NULL;
    }
    
    pthread_mutex_init(&processo->mutex, NULL);
    
    // Cria a thread do processo
    if (pthread_create(&processo->thread, NULL, processoSimuladoExecutar, processo) != 0) {
        processosThreadsFinalizar(processo->pid);
        pthread_mutex_destroy(&processo->mutex);
        free(processo);
        return NULL;
    }
    
    return processo;
}

// Finaliza um processo simulado
void processoSimuladoFinalizar(ProcessoSimulado_t* processo) {
    if (!processo) return;
    
    // Espera a thread terminar
    pthread_join(processo->thread, NULL);
    
    // Limpa os recursos
    pthread_mutex_destroy(&processo->mutex);
    processosThreadsFinalizar(processo->pid);
    free(processo);
} 