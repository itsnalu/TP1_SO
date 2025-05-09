#include "gerenciador.h"

#include "processoSimulado.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//função para criar um novo processo
void criarProcessoSimulado(GerenciadorDeProcessos_t *gerenciador, char *nomeArquivo){
    int pid;
    for (pid = 0; pid < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; pid++) {
        if (!gerenciador->slot_tabela_ocupado[pid]) break;
    }
    if (pid == MAX_PROCESSOS_SIMULADOS_NO_SISTEMA) {
        printf("Limite de processos atingido.\n");
        return;
    }

    ProcessoSimulado_t *proc = &gerenciador->tabela_de_processos[pid];
    //Tem q terminar
    
}
//função paa substituir a imagem atual de um processo para uma novea
void substituirImagemProcesso(GerenciadorDeProcessos_t *gerenciador, int pid, char *novaImagem){
    //Verifica se o PID é válido e se o processo existe
    if(pid < 0 || pid >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA ||gerenciador ->slot_tabela_ocupado[pid]){
        if (pid < 0 || pid >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA || !gerenciador->slot_tabela_ocupado[pid]) { 
            printf("Processo não encontrado.\n");
            return;
        }
        //Substitui o programa do processo pelo novo programa
        ProcessoSimulado_t *processo = &gerenciador->tabela_de_processos[pid];
        //Carrega a nova imagem do processo
        //Acho que a lógica de carregar a imagem do processo deve ser implementada na função psCarregarProgramaDeArquivo
        //Ajustar esse codigo. Com certeza está inc
        psCarregarProgramaDeArquivo(processo, novaImagem);
        printf("Imagem do processo %d substituída com sucesso.\n", pid);
    }

}
//Gerenciar a transição de estados de um processo
void gerenciarTransicoesEstados(GerenciadorDeProcessos_t *gerenciador, int pid, int novoEstado){
    //Verifica se o PID é válido e se o processo existe
    if (pid < 0 || pid >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA || !gerenciador->slot_tabela_ocupado[pid]) {
        printf("Erro: Processo %d não encontrado.\n", pid);
        return;
    }
    //Substitui o programa do processo pelo novo programa
    ProcessoSimulado_t *processo = &gerenciador->tabela_de_processos[pid];
    gerenciador->cpu_sistema.processo_atual = processo;
    processo->estado_atual = EST_EXECUCAO;

    printf("Processo %d escalonado para execução.\n", pid);
}
//Função de escalonamento de processos
void escalonarProcessos(GerenciadorDeProcessos_t *gerenciador){
    //Obtém o próximo pronto para execução
    int pid = obterProximoProcesso(&gerenciador->processos_prontos);
    //verifica se há processos prontos
    if (pid == -1) {
        printf("Nenhum processo pronto para execução.\n");
        return;
    }
    //Define o processo como atual na CPU e altera seu estado para EXECUCAO
    ProcessoSimulado_t *processo = &gerenciador->tabela_de_processos[pid];
    gerenciador->cpu_sistema.processo_atual = processo;
    processo->estado_atual = EST_EXECUCAO;

    printf("Processo %d escalonado para execução.\n", pid);
}   
// Função para realizar a troca de contexto entre processos
void trocarContexto(GerenciadorDeProcessos_t *gerenciador){
    // Implementação da troca de contexto entre processos
}