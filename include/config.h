#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_CMD_LEN 100
#define MAX_THREADS 100

// Constantes do Simulador
#define MAX_MEMORIA_PROCESSO_SIMULADO 100
#define MAX_NOME_ARQUIVO_R 256
#define MAX_PROCESSOS_SIMULADOS_NO_SISTEMA 100

// Configurações de Thread
#define QUANTUM_PADRAO 1  // Quantum padrão para todas as threads
#define ARQUIVO_INIT_PROGRAMA "init.txt"

// Níveis de prioridade para threads
#define NUM_NIVEIS_PRIORIDADE 4
#define QUANTUM_PRIORIDADE_0 4  // Maior prioridade
#define QUANTUM_PRIORIDADE_1 3
#define QUANTUM_PRIORIDADE_2 2
#define QUANTUM_PRIORIDADE_3 1  // Menor prioridade

#endif // CONFIG_H