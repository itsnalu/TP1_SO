#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h> 

#define MAX_CMD_LEN 100 

// --- Constantes do Simulador de Processos ---
#define MAX_MEMORIA_PROCESSO_SIMULADO 100 
#define MAX_NOME_ARQUIVO_R 256            

#define MAX_PROCESSOS_SIMULADOS_NO_SISTEMA 10 
#define ARQUIVO_INIT_PROGRAMA "init.txt"       

// --- Configurações de Escalonamento MLFQ ---
#define NUM_NIVEIS_PRIORIDADE 4 // Prioridades 0 (mais alta) a 3 (mais baixa)
#define QUANTUM_PRIO_0 1        // Quantum para prioridade 0
#define QUANTUM_PRIO_1 2        // Quantum para prioridade 1
#define QUANTUM_PRIO_2 4        // Quantum para prioridade 2
#define QUANTUM_PRIO_3 8        // Quantum para prioridade 3


// Mutex global para a impressão
extern pthread_mutex_t impressao_mutex;

// Mutexes para as filas de estados e tabela de processos
extern pthread_mutex_t prontos_mutex; // Protegerá o acesso ao array de filas de prontos
extern pthread_mutex_t bloqueados_mutex;
extern pthread_mutex_t tabela_processos_mutex;

#endif // CONFIG_H