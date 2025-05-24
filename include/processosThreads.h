#ifndef PROCESSOS_THREADS_H
#define PROCESSOS_THREADS_H

#define MAX_PROCESSOS 100

// Inicializa o sistema de processos
void processosThreadsInicializar(void);

// Cria um novo processo
int processosThreadsCriar(int prioridade);

// Finaliza um processo
void processosThreadsFinalizar(int pid);

// Obtém o estado de um processo
int processosThreadsObterEstado(int pid);

// Define o estado de um processo
void processosThreadsDefinirEstado(int pid, int novo_estado);

// Obtém a prioridade de um processo
int processosThreadsObterPrioridade(int pid);

// Define a prioridade de um processo
void processosThreadsDefinirPrioridade(int pid, int nova_prioridade);

// Incrementa o tempo de CPU de um processo
void processosThreadsIncrementarTempoCPU(int pid);

// Obtém o tempo de CPU de um processo
int processosThreadsObterTempoCPU(int pid);

#endif // PROCESSOS_THREADS_H 