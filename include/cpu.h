#ifndef CPU_H
#define CPU_H

#define CPU_OCIOSA -1 

// Estrutura mínima para o gerenciador rastrear o processo "ativo"
// Não simula registradores, pois cada thread tem seu contexto.
typedef struct {
    int indice_processo_ativo_na_tabela; 
} CPU_Simulada_t;

void cpuSimuladaInicializar(CPU_Simulada_t *cpu);

#endif // CPU_H