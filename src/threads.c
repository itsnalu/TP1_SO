#include "../include/gerenciador.h"      // Para GerenciadorDeProcessos_t e acesso à tabela_de_processos
#include "../include/threads.h"         // Para o protótipo de finalizarThreads
#include "../include/processoSimulado.h" // Para ProcessoSimulado_t e acesso ao campo 'thread'
#include "../include/config.h"          // Para MAX_PROCESSOS_SIMULADOS_NO_SISTEMA e mutex global tabela_processos_mutex
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h> // Para NULL
#include <unistd.h> // Para perror (opcional, dependendo do tratamento de erro do join)


// Função para aguardar a finalização de todas as threads de processos simulados.
// Chamada pelo gerenciador no final da simulação (comando 'M').
void finalizarThreads(GerenciadorDeProcessos_t* gerenciador) {
    if (!gerenciador) {
        fprintf(stderr, "[Thread] ERRO: Gerenciador nulo em finalizarThreads.\n");
        return;
    }

    printf("[Thread] Aguardando a finalização de todas as threads de processos simulados...\n");
    
    pthread_mutex_lock(&tabela_processos_mutex); // Protege o acesso à tabela de processos
    
    for (int i = 0; i < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; i++) {
        if (gerenciador->slot_tabela_ocupado[i]) {
            ProcessoSimulado_t *processo = &gerenciador->tabela_de_processos[i];
            // Verifica se a thread foi criada (processo->thread não é 0)
            // e se não é a thread atual (para evitar deadlock se finalizarThreads fosse chamada por uma thread de processo)
            if (processo->thread != 0 && !pthread_equal(pthread_self(), processo->thread)) {
                printf("[Thread] Aguardando thread do PID %d (SysThreadID 0x%lx) terminar...\n",
                       processo->pid, (unsigned long)processo->thread);
                
                // pthread_mutex_unlock(&tabela_processos_mutex); // Desbloquear antes de join se join for longo
                int ret_join = pthread_join(processo->thread, NULL);
                // pthread_mutex_lock(&tabela_processos_mutex);   // Re-bloquear

                if (ret_join != 0) {
                    printf("[Thread] Aviso: pthread_join para PID %d (SysThreadID 0x%lx) retornou %d (pode ser normal para threads destacadas ou já finalizadas).\n",
                            processo->pid, (unsigned long)processo->thread, ret_join);
                } else {
                    printf("[Thread] Thread do PID %d (SysThreadID 0x%lx) finalizada e juntada com sucesso.\n",
                           processo->pid, (unsigned long)processo->thread);
                }
                processo->thread = 0; // Marca que a thread foi juntada ou a tentativa foi feita
            }
        }
    }
    pthread_mutex_unlock(&tabela_processos_mutex);
    
    // A destruição dos mutexes globais é feita em main.c
    printf("[Thread] Finalização de threads de processos concluída.\n");
}