#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
// #include <semaphore.h> // Removido, pois sem_impressao foi substituído por impressao_mutex

#define MAX_CMD_LEN 100
// #define MAX_THREADS 100 // Menos relevante com o novo modelo de 1 thread por processo.
                         // MAX_PROCESSOS_SIMULADOS_NO_SISTEMA agora é o limite prático.

// Constantes do Simulador
#define MAX_MEMORIA_PROCESSO_SIMULADO 100    
#define MAX_NOME_ARQUIVO_R 256               
#define MAX_PROCESSOS_SIMULADOS_NO_SISTEMA 100 // Limita processos e, portanto, threads de processo

// #define QUANTUM_PADRAO 1 // Removido, usaremos quantums específicos por prioridade para MLFQ
#define ARQUIVO_INIT_PROGRAMA "init.txt"     

// Níveis de prioridade e Quantums para MLFQ (mantendo os valores da "Ana")
#define NUM_NIVEIS_PRIORIDADE 4              
#define QUANTUM_PRIORIDADE_0 4                // Maior prioridade
#define QUANTUM_PRIORIDADE_1 3               
#define QUANTUM_PRIORIDADE_2 2               
#define QUANTUM_PRIORIDADE_3 1                // Menor prioridade

// Mutex global para a impressão (substitui sem_impressao da versão "Ana")
extern pthread_mutex_t impressao_mutex;

// Mutexes globais para as filas de estados e tabela de processos
extern pthread_mutex_t prontos_mutex;
extern pthread_mutex_t bloqueados_mutex;
extern pthread_mutex_t tabela_processos_mutex;

#endif // CONFIG_H