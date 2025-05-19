#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_CMD_LEN 100

// Constantes do Simulador
#define MAX_MEMORIA_PROCESSO_SIMULADO 100
#define MAX_NOME_ARQUIVO_R 256

#define MAX_PROCESSOS_SIMULADOS_NO_SISTEMA 100
#define NUM_NIVEIS_PRIORIDADE 4 // Prioridades 0 (mais alta) a 3
#define ARQUIVO_INIT_PROGRAMA "init" // Arquivo do primeiro processo

// Quantum por nível de prioridade (usado pelo Gerenciador)
#define QUANTUM_PRIORIDADE_0 1
#define QUANTUM_PRIORIDADE_1 2
#define QUANTUM_PRIORIDADE_2 4
#define QUANTUM_PRIORIDADE_3 8

#endif // CONFIG_H