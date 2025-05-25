#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../includeThreads/processosThreads.h"
#include "../includeThreads/estadosThread.h"

#define NUM_PROCESSOS 5

int main() {
    printf("Iniciando simulação com threads...\n");
    
    // Inicializa o sistema
    processosThreadsInicializar();
    
    // Cria alguns processos de teste
    for (int i = 0; i < NUM_PROCESSOS; i++) {
        int prioridade = rand() % 10;  // Prioridade aleatória entre 0 e 9
        int pid = processosThreadsCriar(prioridade);
        if (pid >= 0) {
            printf("Processo %d criado com prioridade %d\n", pid, prioridade);
        } else {
            printf("Erro ao criar processo\n");
        }
    }
    
    // Simula algumas operações
    for (int i = 0; i < NUM_PROCESSOS; i++) {
        processosThreadsDefinirEstado(i, EST_EXECUCAO_THREADS);
        printf("Processo %d em execução\n", i);
        
        // Simula algum processamento
        for (int j = 0; j < 3; j++) {
            processosThreadsIncrementarTempoCPU(i);
            printf("Processo %d: tempo CPU = %d\n", i, processosThreadsObterTempoCPU(i));
        }
        
        processosThreadsDefinirEstado(i, EST_TERMINADO_THREADS);
        printf("Processo %d finalizado\n", i);
    }
    
    printf("Simulação finalizada\n");
    return 0;
} 