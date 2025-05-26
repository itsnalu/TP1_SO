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
    pthread_mutex_init(&gerenciador->mutex_geral_gerenciador, NULL);
    pthread_mutex_init(&gerenciador->comunicacao_gerenciador_processos.mutex, NULL);
    pthread_cond_init(&gerenciador->comunicacao_gerenciador_processos.cond, NULL);
    gerenciador->comunicacao_gerenciador_processos.comando_recebido = 0;
    gerenciador->simulacao_terminando = 0;
}

void finalizarThreads(GerenciadorDeProcessos_t* gerenciador) {
  printf("[Gerenciador] Finalizando todas as threads de processos simulados...\n");
    gerenciador->simulacao_terminando = 1;

    for (int i = 0; i < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; i++) {
        if (gerenciador->slot_tabela_ocupado[i] && gerenciador->tabela_de_processos[i].thread_id != 0) {
            ProcessoSimulado_t *p = &gerenciador->tabela_de_processos[i];
            pthread_mutex_lock(&p->proc_mutex);
            pthread_cond_signal(&p->proc_cond);
            pthread_mutex_unlock(&p->proc_mutex);
        }
    }

    for (int i = 0; i < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; i++) {
        if (gerenciador->slot_tabela_ocupado[i] && gerenciador->tabela_de_processos[i].thread_id != 0) {
            printf("[Gerenciador] Aguardando thread do PID %d (%lu) finalizar...\n", i, (unsigned long)gerenciador->tabela_de_processos[i].thread_id);
            if (pthread_join(gerenciador->tabela_de_processos[i].thread_id, NULL) != 0) {
                perror("Erro ao aguardar thread do processo simulado");
            }
            gerenciador->tabela_de_processos[i].thread_id = 0; // Marcar como juntada
        }
    }
    pthread_mutex_destroy(&gerenciador->mutex_geral_gerenciador);
    pthread_mutex_destroy(&gerenciador->comunicacao_gerenciador_processos.mutex);
    pthread_cond_destroy(&gerenciador->comunicacao_gerenciador_processos.cond);

    printf("[Gerenciador] Todas as threads de processos simulados finalizadas.\n");
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
    
    printf("[Thread PID %d (%lu)] Iniciada.\n", processo->pid, (unsigned long)pthread_self());

    pthread_mutex_lock(&processo->proc_mutex);
    while (1) {
        while (!processo->is_scheduled_to_run && processo->estado_atual != EST_TERMINADO && !gerenciador->simulacao_terminando) {
            pthread_cond_wait(&processo->proc_cond, &processo->proc_mutex);
        }

        if (gerenciador->simulacao_terminando) {
            if(processo->estado_atual != EST_TERMINADO) processo->estado_atual = EST_TERMINADO;
            printf("[Thread PID %d (%lu)] Simulação terminando. Encerrando.\n", processo->pid, (unsigned long)pthread_self());
            break;
        }

        if (processo->estado_atual == EST_TERMINADO) {
            printf("[Thread PID %d (%lu)] Estado TERMINADO detectado. Encerrando.\n", processo->pid, (unsigned long)pthread_self());
            break;
        }

        processo->is_scheduled_to_run = 0; // Resetar flag para o próximo ciclo

        if (processo->estado_atual == EST_EXECUCAO) {
            ProcessoSimulado_t *novo_filho_ptr = NULL;

            psExecutarProximaInstrucao(processo, gerenciador->tempo_simulacao_global, &novo_filho_ptr);

            if (novo_filho_ptr) { // Se a instrução 'F' criou um filho
                pthread_mutex_lock(&gerenciador->mutex_geral_gerenciador);

                // Atribuir PID e colocar na tabela.
             
                atribuirPidAoProcesso(gerenciador, novo_filho_ptr); // Seta novo_filho_ptr->pid
                int pid_filho_real = novo_filho_ptr->pid;

                if (pid_filho_real != -1 && pid_filho_real < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA) {
                    ProcessoSimulado_t* filho_na_tabela = &gerenciador->tabela_de_processos[pid_filho_real];
            
                    memcpy(filho_na_tabela, novo_filho_ptr, sizeof(ProcessoSimulado_t)); // Copia todo o conteúdo, incluindo mutex/cond já inicializados.

                    ThreadArgs_t* args_filho = (ThreadArgs_t*)malloc(sizeof(ThreadArgs_t));
                    if (!args_filho) { perror("malloc args_filho"); /* tratar erro */ }
                    else {
                        args_filho->processo = filho_na_tabela;
                        args_filho->gerenciador = gerenciador;
                        if (pthread_create(&filho_na_tabela->thread_id, NULL, executarProcessoThread, args_filho) != 0) {
                            perror("Erro ao criar thread do processo filho");
                            gerenciador->slot_tabela_ocupado[pid_filho_real] = 0; // Liberar slot
                            free(args_filho); // Liberar args se a thread não foi criada
                        } else {
                            // pthread_detach(filho_na_tabela->thread_id); ou juntar depois
                            printf("[Gerenciador] Thread para novo filho PID %d criada.\n", pid_filho_real);
                        }
                    }
                     // Adicionar à fila de prontos
                    #ifdef USE_FIFO
                        estadosAdicionarProntoFIFO(&gerenciador->processos_prontos, pid_filho_real);
                    #else
                        estadosAdicionarPronto(&gerenciador->processos_prontos, pid_filho_real, filho_na_tabela->prioridade);
                    #endif
                } else {
                     printf("[Gerenciador] Erro: PID inválido ou limite de processos atingido para filho.\n");
                     psLiberarMemoria(novo_filho_ptr); // Se não foi para a tabela, liberar
                }
                pthread_mutex_unlock(&gerenciador->mutex_geral_gerenciador);
                 if (novo_filho_ptr && (pid_filho_real == -1 || pid_filho_real >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA)) {
                    // Se não foi adicionado à tabela, liberar o ponteiro original
                    // psLiberarMemoria(novo_filho_ptr); // Já tratado acima se pid_filho_real é inválido
                }

            } // Fim do if (novo_filho_ptr)
        } // Fim do if (processo->estado_atual == EST_EXECUCAO)

        // Sinalizar ao Gerenciador que esta fatia de tempo/instrução terminou
        pthread_mutex_lock(&gerenciador->comunicacao_gerenciador_processos.mutex);
        gerenciador->comunicacao_gerenciador_processos.comando_recebido = processo->pid + 1; // Informa qual PID (ou apenas um flag)
        pthread_cond_signal(&gerenciador->comunicacao_gerenciador_processos.cond);
        pthread_mutex_unlock(&gerenciador->comunicacao_gerenciador_processos.mutex);

    } // Fim do while(1)
    pthread_mutex_unlock(&processo->proc_mutex);

    printf("[Thread PID %d (%lu)] Saindo.\n", processo->pid, (unsigned long)pthread_self());
    return NULL;
}

void* executarImpressaoThread(void* arg) {
    ProcessoImpressao_t* args_impressao = (ProcessoImpressao_t*)arg;
    if (!args_impressao || !args_impressao->gerenciador) {
        fprintf(stderr, "[Thread Impressão] Erro: Argumentos inválidos para thread de impressão.\n");
        if(args_impressao) free(args_impressao); // Libera se foi alocado
        return NULL;
    }

    GerenciadorDeProcessos_t* gerenciador = args_impressao->gerenciador;
    int tipo_impressao = args_impressao->tipo_impressao; // 0 para estado atual (I), 1 para final (M)

    printf("[Thread Impressão %lu] Iniciada (Tipo: %s).\n",
           (unsigned long)pthread_self(), tipo_impressao == 1 ? "FINAL" : "ESTADO ATUAL");
    if (pthread_mutex_lock(&gerenciador->mutex_cpu) != 0) {
        perror("[Thread Impressão] Erro ao bloquear mutex_cpu");
        free(args_impressao);
        return NULL;
    }

    printf("[Thread Impressão %lu] Realizando impressão...\n", (unsigned long)pthread_self());

    if (tipo_impressao == 1) { // Comando 'M' - Estatísticas Finais
        processoImpressaoImprimirEstatisticas(args_impressao);
    } else { // Comando 'I' - Estado Atual
        processoImpressaoImprimirEstado(args_impressao);
    }

    if (pthread_mutex_unlock(&gerenciador->mutex_cpu) != 0) {
        perror("[Thread Impressão] Erro ao desbloquear mutex_cpu");
    }

    if (sem_post(&gerenciador->sem_impressao) != 0) {
        perror("[Thread Impressão] Erro ao liberar sem_impressao");
    }

    printf("[Thread Impressão %lu] Finalizada.\n", (unsigned long)pthread_self());

    // Libera a estrutura de argumentos que foi alocada em processoImpressaoIniciar
    processoImpressaoFinalizar(args_impressao); // ou apenas free(args_impressao) se ProcessoImpressao_t não tiver cleanup complexo

    return NULL;
}