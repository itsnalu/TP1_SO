#include "cpu.h"

void cpuSimuladaInicializar(CPU_Simulada_t *cpu) {
    if (cpu) {
        cpu->indice_processo_ativo_na_tabela = CPU_OCIOSA;
    }
}