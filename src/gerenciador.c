#include "gerenciador.h"

#include "processoSimulado.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//função para criar um novo processo
void criarProcessoSimulado(GerenciadorDeProcessos_t *gerenciador, char *nomeArquivo){
    // ========================= TESTAR E CONFERIR =========================
    int pid;
    for (pid = 0; pid < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; pid++) {
        if (!gerenciador->slot_tabela_ocupado[pid]) break;
    }
    if (pid == MAX_PROCESSOS_SIMULADOS_NO_SISTEMA) {
        printf("Limite de processos atingido.\n");
        return;
    }

    // inicializa a estrutura do processo
    ProcessoSimulado_t *proc = &gerenciador->tabela_de_processos[pid];
    memset(proc, 0, sizeof(ProcessoSimulado_t));    // Zera a estrutura

    // campos preenchidos com valores padrao
    proc->pid = pid;
    proc->pid_pai = -1;                             // Nenhum pai por padrão
    proc->estado_atual = EST_PRONTO;
    proc->prioridade = 0;
    proc->pc = 0;
    proc->tempo_chegada_sistema = time(NULL);
    proc->tempo_total_cpu_usado = 0;
    proc->tempo_restante_bloqueio = 0;
    proc->tempo_usado_no_quantum_atual = 0;
    proc->numProcessos = 0;
    proc->num_variaveis_declaradas = 0;

    // inicializa lista de instruções
    proc->listaInstrucoes.primeiro = NULL;
    proc->listaInstrucoes.ultimo = NULL;
    proc->listaInstrucoes.tamanho = 0;

    // abre o arquivo de instrucoes
    FILE *arquivo = fopen(nomeArquivo, "r");
    if(!arquivo){
        printf("Erro ao abrir o arquivo %s.\n", nomeArquivo);
        return;
    }

    // leitura das instruções do arquivo
    char linha[256];
    while(fgets(linha, sizeof(linha), arquivo)){
        //printf("\n\n entrou no while \n\n");
        // interpreta cada linha como uma instrucao
        Instrucao_t instrucao;
        memset(&instrucao, 0, sizeof(Instrucao_t));

        char tipo;
        int arg1 = 0;
        int arg2 = 0;
        char nomeArq[MAX_NOME_ARQUIVO_R] = "";
        
        // interpretação simples: tipo arg1 arg2 nomeArquivo
        int campos_lidos = sscanf(linha, "%c %d %d %s", &tipo, &arg1, &arg2, nomeArq);
        instrucao.tipoInstrucaoChar = tipo;
        instrucao.arg1 = arg1;
        instrucao.arg2 = arg2;

        // se a instrucao for do tipo R, armazena o nome do arquivo no campo correspondente
        if(tipo == 'R' && campos_lidos == 4){
            strncpy(instrucao.nome_arquivo_R, nomeArq, MAX_NOME_ARQUIVO_R - 1);
        }

        // adiciona a instrucao na lista encadeada
        CelulaInstrucao_t *nova = (CelulaInstrucao_t *) malloc(sizeof(CelulaInstrucao_t));
        nova->instrucao = instrucao;
        nova->prox = NULL;

        // se for a primeira instrucao, define como primeiro
        // caso contrario, liga ao final da lista
        // atualiza o ponteiro ultimo e o tamanho da lista
        if(proc->listaInstrucoes.primeiro == NULL){
            proc->listaInstrucoes.primeiro = nova;
        }else{
            proc->listaInstrucoes.ultimo->prox = nova;
        }
        proc->listaInstrucoes.ultimo = nova;
        proc->listaInstrucoes.tamanho++;
    }

    fclose(arquivo);

    // marca o slot do processo como ocupado
    gerenciador->slot_tabela_ocupado[pid] = 1;

    printf("Processo criado com PID %d a partir do arquivo %s.\n", pid, nomeArquivo);
}
//função paa substituir a imagem atual de um processo para uma nova
void substituirImagemProcesso(GerenciadorDeProcessos_t *gerenciador, int pid, char *novaImagem){
    // ========================= TESTAR E CONFERIR =========================
    //Verifica se o PID é válido e se o processo existe
    if(pid < 0 || pid >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA ||gerenciador ->slot_tabela_ocupado[pid]){
        if (pid < 0 || pid >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA || !gerenciador->slot_tabela_ocupado[pid]) { 
            printf("Processo não encontrado.\n");
            return;
        }
        //Substitui o programa do processo pelo novo programa
        ProcessoSimulado_t *processo = &gerenciador->tabela_de_processos[pid];

        // libera a lista de instruções antiga
        CelulaInstrucao_t *atual = processo->listaInstrucoes.primeiro;
        while(atual != NULL){
            CelulaInstrucao_t *temp = atual;
            atual = atual->prox;
            free(temp);
        }

        // zera os dados da lista
        processo->listaInstrucoes.primeiro = NULL;
        processo->listaInstrucoes.ultimo = NULL;
        processo->listaInstrucoes.tamanho = 0;

        // reinicia os valores de execução
        processo->pc = 0;
        processo->tempo_total_cpu_usado = 0;
        processo->tempo_restante_bloqueio = 0;
        processo->tempo_usado_no_quantum_atual = 0;
        processo->estado_atual = EST_PRONTO;

        // carrega a nova imagem do processo
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
    // ========================= TESTAR E CONFERIR =========================
    CPU_t *cpu = &gerenciador->cpu_sistema;

    // salva o contexto do processo atual (se houver)
    if(cpu->processo_atual != NULL){
        // atualiza o PC do processo simulado
        cpu->processo_atual->pc = cpu->pc_registrador_cpu;

        // atualiza tempo usado no quantum
        cpu->processo_atual->tempo_usado_no_quantum_atual = cpu->tempo_executado_neste_quantum;

        // atualiza estado do processo
        if(cpu->processo_atual->estado_atual == EST_EXECUCAO){
            cpu->processo_atual->estado_atual = EST_PRONTO;

            // reinsere na fila de prontos
            adicionar_processo_em_pronto(&gerenciador->processos_prontos, cpu->indice_processo_na_tabela);
        }
    }

    // seleciona o próximo processo pronto
    int novo_pid = remover_proximo_processo_pronto(&gerenciador->processos_prontos);

    // caso não haja processo pronto, deixa a CPU ociosa
    if (novo_pid == -1) {
        cpu->indice_processo_na_tabela = -1;
        cpu->pc_registrador_cpu = 0;
        cpu->quantum_total_alocado = 0;
        cpu->tempo_executado_neste_quantum = 0;
        cpu->processo_atual = NULL;
        return;
    }

    // carrega o novo processo na CPU
    ProcessoSimulado_t *novo_processo = &gerenciador->tabela_de_processos[novo_pid];

    cpu->indice_processo_na_tabela = novo_pid;
    cpu->pc_registrador_cpu = novo_processo->pc;
    cpu->quantum_total_alocado = "QUANTUM_PADRAO";
    cpu->tempo_executado_neste_quantum = 0;
    cpu->processo_atual = novo_processo;

    // atualiza estado do processo
    novo_processo->estado_atual = EST_EXECUCAO;

    // marca tempo de chegada se for a primeira vez que está entrando
    if(novo_processo->tempo_chegada_sistema == -1){
        novo_processo->tempo_chegada_sistema = gerenciador->tempo_simulacao_global;
    }
}