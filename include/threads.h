#ifndef THREADS_H
#define THREADS_H

// Para pthread_t, se necessário aqui, mas GerenciadorDeProcessos_t já inclui processoSimulado.h que tem pthread.h
// #include <pthread.h> 
struct GerenciadorDeProcessos_s;

// Declaração da função para finalizar (dar join) todas as threads de processos simulados.
// Esta função será chamada pelo gerenciador no final da simulação.
void finalizarThreads(struct GerenciadorDeProcessos_s* gerenciador);

#endif // THREADS_H