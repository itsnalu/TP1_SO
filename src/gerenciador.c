#include "gerenciador.h"
#include "processoSimulado.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

//função para criar um novo processo
void criarProcessoSimulado(GerenciadorDeProcessos_t *gerenciador, char *nomeArquivo){
    int pid = 0;
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
        //
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
//função para substituir a imagem atual de um processo para uma nova
void substituirImagemProcesso(GerenciadorDeProcessos_t *gerenciador, int pid, char *novaImagem){
    // Verifica se o PID é válido e se o processo existe
    if (pid < 0 || pid >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA || !gerenciador->slot_tabela_ocupado[pid]) {
        printf("Processo não encontrado.\n");
        return;
    }
    // Substitui o programa do processo pelo novo programa
    ProcessoSimulado_t *processo = &gerenciador->tabela_de_processos[pid];
    // Libera a lista de instruções antiga
    CelulaInstrucao_t *atual = processo->listaInstrucoes.primeiro;
    while(atual != NULL){
        CelulaInstrucao_t *temp = atual;
        atual = atual->prox;
        free(temp);
    }
    // Zera os dados da lista
    processo->listaInstrucoes.primeiro = NULL;
    processo->listaInstrucoes.ultimo = NULL;
    processo->listaInstrucoes.tamanho = 0;
    // Reinicia os valores de execução
    processo->pc = 0;
    processo->tempo_total_cpu_usado = 0;
    processo->tempo_restante_bloqueio = 0;
    processo->tempo_usado_no_quantum_atual = 0;
    processo->estado_atual = EST_PRONTO;
    // Carrega a nova imagem do processo
    psCarregarProgramaDeArquivo(processo, novaImagem);
    printf("Imagem do processo %d substituída com sucesso.\n", pid);
}

//Gerenciar a transição de estados de um processo
void gerenciarTransicoesEstados(GerenciadorDeProcessos_t *gerenciador, int pid, int novoEstado) {
    if (pid < 0 || pid >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA || !gerenciador->slot_tabela_ocupado[pid]) {
        printf("[Gerenciador] Erro: PID %d inválido para transição de estado\n", pid);
        return;
    }

    ProcessoSimulado_t *processo = &gerenciador->tabela_de_processos[pid];
    EstadoProcesso_e estado_anterior = processo->estado_atual;
    
    // Atualiza o estado do processo
    processo->estado_atual = novoEstado;
    
    printf("[Gerenciador] Processo %d: %d -> %d\n", pid, estado_anterior, novoEstado);
}
// Função de escalonamento de processos
void escalonarProcessos(GerenciadorDeProcessos_t *gerenciador) {
    ProcessoSimulado_t *processo_que_estava_na_cpu = gerenciador->cpu_sistema.processo_atual;
    int reescalonar_mesmo_processo_com_nova_prioridade = 0;
    int pid_do_processo_que_estava_na_cpu = -1;

    // 1. AVALIAR O PROCESSO QUE ESTAVA NA CPU (se houver)
    if (processo_que_estava_na_cpu != NULL) {
        pid_do_processo_que_estava_na_cpu = processo_que_estava_na_cpu->pid;

        // Salva o tempo que o processo usou na CPU nesta última execução
        // O incremento de gerenciador->cpu_sistema.tempo_executado_neste_quantum
        // foi feito em executarUnidadeTempo, ANTES de chamar escalonarProcessos.
        processo_que_estava_na_cpu->tempo_usado_no_quantum_atual = gerenciador->cpu_sistema.tempo_executado_neste_quantum;

        // Verifica se o processo que estava na CPU consumiu seu quantum
        if (gerenciador->cpu_sistema.tempo_executado_neste_quantum >= gerenciador->cpu_sistema.quantum_total_alocado) {
            if (processo_que_estava_na_cpu->prioridade < NUM_NIVEIS_PRIORIDADE - 1) {
                processo_que_estava_na_cpu->prioridade++; // Diminui a prioridade
                printf("[Gerenciador] Processo %d teve prioridade diminuida para %d (quantum estourado)\n",
                       processo_que_estava_na_cpu->pid, processo_que_estava_na_cpu->prioridade);
                // Se a prioridade mudou, ele vai receber um novo quantum, então o tempo usado "neste quantum" para ele zera.
                processo_que_estava_na_cpu->tempo_usado_no_quantum_atual = 0;
                reescalonar_mesmo_processo_com_nova_prioridade = 1; // Marca para resetar o tempo da CPU se ele for o próximo
            } else {
                 // Já está na prioridade mais baixa e estourou o quantum,
                 // simplesmente volta para o fim da fila da sua prioridade.
                 // O tempo_usado_no_quantum_atual dele também zera para um novo ciclo.
                 processo_que_estava_na_cpu->tempo_usado_no_quantum_atual = 0;
                 reescalonar_mesmo_processo_com_nova_prioridade = 1; // Marca para resetar o tempo da CPU se ele for o próximo
            }
        }

        // Se o processo não terminou nem bloqueou, ele volta para a fila de prontos
        if (processo_que_estava_na_cpu->estado_atual != EST_BLOQUEADO &&
            processo_que_estava_na_cpu->estado_atual != EST_TERMINADO) {
            processo_que_estava_na_cpu->estado_atual = EST_PRONTO;
            estadosAdicionarPronto(&gerenciador->processos_prontos,
                                   processo_que_estava_na_cpu->pid,
                                   processo_que_estava_na_cpu->prioridade);
            printf("[Gerenciador] Processo %d reinserido na fila de prontos (Prioridade: %d)\n",
                   processo_que_estava_na_cpu->pid, processo_que_estava_na_cpu->prioridade);
        }
    }

    // 2. SELECIONAR O PRÓXIMO PROCESSO PARA EXECUTAR
    int proximo_pid = estadosRemoverPronto(&gerenciador->processos_prontos);

    if (proximo_pid == -1) { // Nenhum processo pronto
        gerenciador->cpu_sistema.processo_atual = NULL;
        gerenciador->cpu_sistema.indice_processo_na_tabela = CPU_OCIOSA;
        gerenciador->cpu_sistema.pc_registrador_cpu = 0;
        gerenciador->cpu_sistema.quantum_total_alocado = 0;
        gerenciador->cpu_sistema.tempo_executado_neste_quantum = 0;
        printf("[Gerenciador] Nenhum processo pronto para execução. CPU Ociosa.\n");
        return;
    }

    if (proximo_pid < 0 || proximo_pid >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA ||
        !gerenciador->slot_tabela_ocupado[proximo_pid]) {
        gerenciador->cpu_sistema.processo_atual = NULL;
        gerenciador->cpu_sistema.indice_processo_na_tabela = CPU_OCIOSA;
        gerenciador->cpu_sistema.pc_registrador_cpu = 0;
        gerenciador->cpu_sistema.quantum_total_alocado = 0;
        gerenciador->cpu_sistema.tempo_executado_neste_quantum = 0;
        printf("[Gerenciador] ERRO: PID %d inválido (%s) da fila de prontos. CPU ociosa.\n",
                proximo_pid,
                (proximo_pid < 0 || proximo_pid >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA) ? "índice fora dos limites" : "slot não ocupado");
        // Tenta pegar o próximo, se houver, para não parar a simulação por um PID inválido na fila.
        escalonarProcessos(gerenciador);
        return;
    }


    ProcessoSimulado_t *proximo_processo_a_executar = &gerenciador->tabela_de_processos[proximo_pid];

    // 3. CONFIGURAR A CPU PARA O NOVO PROCESSO
    proximo_processo_a_executar->estado_atual = EST_EXECUCAO;
    gerenciador->cpu_sistema.processo_atual = proximo_processo_a_executar;
    gerenciador->cpu_sistema.indice_processo_na_tabela = proximo_pid;
    gerenciador->cpu_sistema.pc_registrador_cpu = proximo_processo_a_executar->pc;

    // Define o novo quantum baseado na prioridade do processo que VAI ENTRAR na CPU
    switch (proximo_processo_a_executar->prioridade) {
        case 0: gerenciador->cpu_sistema.quantum_total_alocado = QUANTUM_PRIORIDADE_0; break;
        case 1: gerenciador->cpu_sistema.quantum_total_alocado = QUANTUM_PRIORIDADE_1; break;
        case 2: gerenciador->cpu_sistema.quantum_total_alocado = QUANTUM_PRIORIDADE_2; break;
        case 3: gerenciador->cpu_sistema.quantum_total_alocado = QUANTUM_PRIORIDADE_3; break;
        default: gerenciador->cpu_sistema.quantum_total_alocado = QUANTUM_PRIORIDADE_0;
    }

    // Zera o contador de tempo da CPU se:
    // 1. O processo escalonado é DIFERENTE do que estava antes.
    // 2. OU é o MESMO processo, mas sua prioridade mudou (ou ele estourou o quantum na mesma prioridade mais baixa),
    if (pid_do_processo_que_estava_na_cpu != proximo_pid || reescalonar_mesmo_processo_com_nova_prioridade) {
        gerenciador->cpu_sistema.tempo_executado_neste_quantum = 0;
        proximo_processo_a_executar->tempo_usado_no_quantum_atual = 0; // Também zera o do processo
    }


    printf("[Gerenciador] Processo %d escalonado para execução (PC=%d, Prioridade=%d, Quantum=%d)\n",
           proximo_processo_a_executar->pid, proximo_processo_a_executar->pc,
           proximo_processo_a_executar->prioridade, gerenciador->cpu_sistema.quantum_total_alocado);
}

// Funcao de escalonamento de processos usando FIFO
void escalonarProcessosFIFO(GerenciadorDeProcessos_t *gerenciador){
    // Se há um processo em execução, verifica se ele deve ser reinserido na fila
    if(gerenciador->cpu_sistema.processo_atual){
        ProcessoSimulado_t *processo_atual = gerenciador->cpu_sistema.processo_atual;
        if(processo_atual->estado_atual != EST_BLOQUEADO && 
            processo_atual->estado_atual != EST_TERMINADO){
            processo_atual->estado_atual = EST_PRONTO;
            estadosAdicionarProntoFIFO(&gerenciador->processos_prontos, processo_atual->pid);
            printf("[FIFO] Processo %d reinserido na fila FIFO\n", processo_atual->pid);
        }
    }
    // Obtem o próximo processo da fila FIFO
    int proximo_pid = estadosRemoverProntoFIFO(&gerenciador->processos_prontos);
    if(proximo_pid == -1){
        // Nenhum processo pronto
        gerenciador->cpu_sistema.processo_atual = NULL;
        gerenciador->cpu_sistema.indice_processo_na_tabela = CPU_OCIOSA;
        printf("[FIFO] Nenhum processo pronto para execução\n");
        return;
    }
    // Aponta para o processo selecionado
    ProcessoSimulado_t *proximo_processo = &gerenciador->tabela_de_processos[proximo_pid];
    proximo_processo->estado_atual = EST_EXECUCAO;
    gerenciador->cpu_sistema.processo_atual = proximo_processo;
    gerenciador->cpu_sistema.indice_processo_na_tabela = proximo_pid;
    gerenciador->cpu_sistema.pc_registrador_cpu = proximo_processo->pc;
    // FIFO nao usa quantum, entao zera os campos relacionados a quantum
    gerenciador->cpu_sistema.quantum_total_alocado = 0; 
    // quantum total alocado é 0 para FIFO
    gerenciador->cpu_sistema.tempo_executado_neste_quantum = 0;
    printf("[FIFO] Processo %d escalonado (PC=%d, Quantum=%d)\n",
           proximo_processo->pid,
           proximo_processo->pc,
           gerenciador->cpu_sistema.quantum_total_alocado);
}
//Função de escalonamento de processos com THREADS
void escalonarProcessosThreads(GerenciadorDeProcessos_t *gerenciador){
    // Se há um processo em execução, verifica se precisa ser reinserido na fila de prontos
    if (gerenciador->cpu_sistema.processo_atual) {
        ProcessoSimulado_t *processo_atual = gerenciador->cpu_sistema.processo_atual;
        
        // Se o processo não está bloqueado ou terminado, reinsere na fila de prontos
        if (processo_atual->estado_atual != EST_BLOQUEADO && 
            processo_atual->estado_atual != EST_TERMINADO) {
            
            // Verifica se o quantum foi consumido
            if (gerenciador->cpu_sistema.tempo_executado_neste_quantum >= gerenciador->cpu_sistema.quantum_total_alocado) {
                // Diminui a prioridade se o quantum foi consumido
                if (processo_atual->prioridade < NUM_NIVEIS_PRIORIDADE - 1) {
                    processo_atual->prioridade++;
                    printf("[Gerenciador] Processo %d teve prioridade aumentada para %d\n",
                           processo_atual->pid, processo_atual->prioridade);
                }
            }
            
            processo_atual->estado_atual = EST_PRONTO;
            estadosAdicionarPronto(&gerenciador->processos_prontos, 
                                   processo_atual->pid, 
                                   processo_atual->prioridade);
            
            printf("[Gerenciador] Processo %d reinserido na fila de prontos (Prioridade: %d)\n",
                   processo_atual->pid, processo_atual->prioridade);
        }
    }

    // Tenta obter o próximo processo da fila de prontos
    int proximo_pid = estadosRemoverPronto(&gerenciador->processos_prontos);
    if (proximo_pid < 0 || proximo_pid >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA || 
        !gerenciador->slot_tabela_ocupado[proximo_pid]) {
        // PID inválido ou slot não ocupado
        gerenciador->cpu_sistema.processo_atual = NULL;
        gerenciador->cpu_sistema.indice_processo_na_tabela = CPU_OCIOSA;
        printf("[Gerenciador] Nenhum processo válido para execução (PID=%d)\n", proximo_pid);
        return;
    }

    // Encontra o processo na tabela de processos
    ProcessoSimulado_t *proximo_processo = &gerenciador->tabela_de_processos[proximo_pid];
    
    // Atualiza o estado do processo para execução
    proximo_processo->estado_atual = EST_EXECUCAO;
    
    // Atualiza a CPU com o novo processo
    gerenciador->cpu_sistema.processo_atual = proximo_processo;
    gerenciador->cpu_sistema.indice_processo_na_tabela = proximo_pid;
    gerenciador->cpu_sistema.pc_registrador_cpu = proximo_processo->pc;
    
    // Define o quantum baseado na prioridade
    switch (proximo_processo->prioridade) {
        case 0:
            gerenciador->cpu_sistema.quantum_total_alocado = QUANTUM_PRIORIDADE_0;
            break;
        case 1:
            gerenciador->cpu_sistema.quantum_total_alocado = QUANTUM_PRIORIDADE_1;
            break;
        case 2:
            gerenciador->cpu_sistema.quantum_total_alocado = QUANTUM_PRIORIDADE_2;
            break;
        case 3:
            gerenciador->cpu_sistema.quantum_total_alocado = QUANTUM_PRIORIDADE_3;
            break;
        default:
            gerenciador->cpu_sistema.quantum_total_alocado = QUANTUM_PRIORIDADE_0;
    }
    
    gerenciador->cpu_sistema.tempo_executado_neste_quantum = 0;
    
    printf("[Gerenciador] Processo %d escalonado para execução (PC=%d, Prioridade=%d, Quantum=%d)\n",
           proximo_processo->pid, proximo_processo->pc, proximo_processo->prioridade,
           gerenciador->cpu_sistema.quantum_total_alocado);

    // Cria a thread para executar o processo simulado
    int resultado_thread = pthread_create(&proximo_processo->thread_id, NULL, psExecutarProcesso, (void*)proximo_processo);
    if (resultado_thread != 0) {
        fprintf(stderr, "[Erro] Falha ao criar thread para o processo %d\n", proximo_processo->pid);
        proximo_processo->estado_atual = EST_TERMINADO;
        gerenciador->cpu_sistema.processo_atual = NULL;
        return;
    }
}
//Função de escalonamento com FIFO usando THREADS
void escalonarProcessosFIFOThreads(GerenciadorDeProcessos_t *gerenciador){

}
// Função para realizar a troca de contexto entre processos
void trocarContexto(GerenciadorDeProcessos_t *gerenciador){
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
            estadosAdicionarPronto(&gerenciador->processos_prontos, cpu->indice_processo_na_tabela, cpu->processo_atual->prioridade);
        }
    }
    // seleciona o próximo processo pronto
    int novo_pid = estadosRemoverPronto(&gerenciador->processos_prontos);
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
    novo_processo->pc = cpu->pc_registrador_cpu; // Sincroniza o PC do processo com o da CPU
    cpu->quantum_total_alocado = QUANTUM_PRIORIDADE_0; // Usa o quantum da prioridade mais alta
    cpu->tempo_executado_neste_quantum = 0;
    cpu->processo_atual = novo_processo;
    // atualiza estado do processo
    novo_processo->estado_atual = EST_EXECUCAO;
    // marca tempo de chegada se for a primeira vez que está entrando
    if(novo_processo->tempo_chegada_sistema == -1){
        novo_processo->tempo_chegada_sistema = gerenciador->tempo_simulacao_global;
    }
}

int get_quantum_for_priority(int priority) {
    switch (priority) {
        case 0: return QUANTUM_PRIORIDADE_0;
        case 1: return QUANTUM_PRIORIDADE_1;
        case 2: return QUANTUM_PRIORIDADE_2;
        case 3: return QUANTUM_PRIORIDADE_3;
        default: return QUANTUM_PADRAO; // Ou um valor padrão de config.h
    }
}
// Função auxiliar para executar uma unidade de tempo
void executarUnidadeTempo(GerenciadorDeProcessos_t *gerenciador) {
    pthread_mutex_lock(&gerenciador->mutex_geral_gerenciador);
    printf("\n[Gerenciador UTE %ld] << INICIO UNIDADE DE TEMPO >>\n", gerenciador->tempo_simulacao_global);

    // 1. Processar processos bloqueados (despertar)
    // (Adapte sua lógica existente para ser segura com mutex_fila_bloqueados e mutex_fila_prontos)
    FilaProcessos_t *fila_bloq = &gerenciador->processos_bloqueados.fila_geral_bloqueados;
    int n_bloqueados = fila_bloq->tamanho;
    for (int k = 0; k < n_bloqueados; ++k) {
        int pid_bloqueado = filaDesenfileirar(fila_bloq); // Remove da frente
        if (pid_bloqueado == -1) break; // Fila ficou vazia

        if (pid_bloqueado < 0 || pid_bloqueado >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA || !gerenciador->slot_tabela_ocupado[pid_bloqueado]) {
            continue;
        }
        ProcessoSimulado_t *proc_b = &gerenciador->tabela_de_processos[pid_bloqueado];

        pthread_mutex_lock(&proc_b->proc_mutex); // Trava o processo específico
        if (proc_b->estado_atual == EST_BLOQUEADO) {
            proc_b->tempo_restante_bloqueio--;
            if (proc_b->tempo_restante_bloqueio <= 0) {
                printf("[Gerenciador UTE %ld] Processo PID %d desbloqueado.\n", gerenciador->tempo_simulacao_global, proc_b->pid);
                proc_b->estado_atual = EST_PRONTO;
                proc_b->tempo_usado_no_quantum_atual = 0;
                // Lógica de aumento de prioridade se bloqueou antes de estourar quantum
                // Esta lógica está em psExecutarProximaInstrucao para 'B', ou pode ser reforçada aqui.
                // Se a instrução 'B' já aumentou a prioridade, apenas adicione à fila de prontos.
                #ifdef USE_FIFO
                    estadosAdicionarProntoFIFO(&gerenciador->processos_prontos, proc_b->pid);
                #else
                    estadosAdicionarPronto(&gerenciador->processos_prontos, proc_b->pid, proc_b->prioridade);
                #endif
            } else {
                filaEnfileirar(fila_bloq, pid_bloqueado); // Ainda bloqueado, volta para o fim da fila de bloqueados
            }
        } else { // Não deveria estar na fila de bloqueados se não está EST_BLOQUEADO
             filaEnfileirar(fila_bloq, pid_bloqueado); // Adiciona de volta por segurança, mas investigue
        }
        pthread_mutex_unlock(&proc_b->proc_mutex);
    }

    // 2. Lidar com o processo que estava na CPU (se houver)
    ProcessoSimulado_t *processo_anterior_cpu = gerenciador->cpu_sistema.processo_atual;
    if (processo_anterior_cpu != NULL) {
        pthread_mutex_lock(&processo_anterior_cpu->proc_mutex);
        // Se a thread do processo sinalizou que terminou sua instrução e ainda está "em execução"
        // (ou seja, não se bloqueou/terminou sozinha), agora o Gerenciador decide o que fazer.
        if (processo_anterior_cpu->estado_atual == EST_EXECUCAO) { // <--- PONTO CRÍTICO AQUI
            processo_anterior_cpu->estado_atual = EST_PRONTO; // Volta para pronto
            printf("[Gerenciador UTE %ld] PID %d (anterior CPU) volta para PRONTO.\n", gerenciador->tempo_simulacao_global, processo_anterior_cpu->pid);

            #ifndef USE_FIFO
                // Lógica de quantum e rebaixamento de prioridade
                int quantum_alocado = get_quantum_for_priority(processo_anterior_cpu->prioridade);
                if (processo_anterior_cpu->tempo_usado_no_quantum_atual >= quantum_alocado) {
                    processo_anterior_cpu->tempo_usado_no_quantum_atual = 0;
                }
            #endif
            #ifdef USE_FIFO
            estadosAdicionarProntoFIFO(&gerenciador->processos_prontos, processo_anterior_cpu->pid);
            #else
            estadosAdicionarPronto(&gerenciador->processos_prontos, processo_anterior_cpu->pid, processo_anterior_cpu->prioridade);
            #endif
        }
        // Se ele se bloqueou ou terminou, sua thread já mudou o estado e o Gerenciador não precisa fazer nada aqui.
        pthread_mutex_unlock(&processo_anterior_cpu->proc_mutex);
        gerenciador->cpu_sistema.processo_atual = NULL; // CPU está livre
    }

    // 3. Selecionar próximo processo da fila de prontos
    int pid_proximo = -1;
    #ifdef USE_FIFO
        pid_proximo = estadosRemoverProntoFIFO(&gerenciador->processos_prontos);
    #else
        pid_proximo = estadosRemoverPronto(&gerenciador->processos_prontos);
    #endif

    gerenciador->cpu_sistema.processo_atual = NULL; // Default: CPU ociosa

    if (pid_proximo != -1 && gerenciador->slot_tabela_ocupado[pid_proximo]) {
        ProcessoSimulado_t *proc_para_rodar = &gerenciador->tabela_de_processos[pid_proximo];

        pthread_mutex_lock(&proc_para_rodar->proc_mutex);
        if (proc_para_rodar->estado_atual == EST_PRONTO) {
            proc_para_rodar->estado_atual = EST_EXECUCAO;
            gerenciador->cpu_sistema.processo_atual = proc_para_rodar;
            gerenciador->cpu_sistema.indice_processo_na_tabela = proc_para_rodar->pid;
            proc_para_rodar->tempo_total_cpu_usado++; // Incrementa tempo de CPU usado pelo processo

            #ifndef USE_FIFO
                // Cada "U" conta como uma unidade para o quantum atual.
                // Se for um novo processo ou um que estourou quantum, tempo_usado_no_quantum_atual é 0.
                gerenciador->cpu_sistema.tempo_executado_neste_quantum = proc_para_rodar->tempo_usado_no_quantum_atual;
                gerenciador->cpu_sistema.quantum_total_alocado = get_quantum_for_priority(proc_para_rodar->prioridade);
            #else
                gerenciador->cpu_sistema.tempo_executado_neste_quantum = 0;
                gerenciador->cpu_sistema.quantum_total_alocado = 0; // FIFO não tem quantum preemptivo
            #endif

            proc_para_rodar->is_scheduled_to_run = 1;
            printf("[Gerenciador UTE %ld] PID %d (Thread %lu) escalonado para EXECUCAO. Quantum: %d (Usado neste turno: %d)\n",
                   gerenciador->tempo_simulacao_global, proc_para_rodar->pid, (unsigned long)proc_para_rodar->thread_id,
                   gerenciador->cpu_sistema.quantum_total_alocado, proc_para_rodar->tempo_usado_no_quantum_atual);
            pthread_cond_signal(&proc_para_rodar->proc_cond); // Acorda a thread do processo
        } else {
            if (proc_para_rodar->estado_atual != EST_TERMINADO) {
                #ifdef USE_FIFO
                    estadosAdicionarProntoFIFO(&gerenciador->processos_prontos, pid_proximo);
                #else
                    estadosAdicionarPronto(&gerenciador->processos_prontos, pid_proximo, proc_para_rodar->prioridade);
                #endif
            }
        }
        pthread_mutex_unlock(&proc_para_rodar->proc_mutex);

        // Se um processo foi despachado, esperar a sua thread sinalizar de volta
        if (gerenciador->cpu_sistema.processo_atual) {
            pthread_mutex_lock(&gerenciador->comunicacao_gerenciador_processos.mutex);
            // printf("[Gerenciador UTE %ld] Aguardando PID %d (Thread) completar fatia...\n", gerenciador->tempo_simulacao_global, gerenciador->cpu_sistema.processo_atual->pid);
            struct timespec timeout;
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += 2; // Timeout de 2 segundos

            int wait_ret = 0;
            // Loop para tratar spurious wakeups e verificar a condição real
            while(gerenciador->comunicacao_gerenciador_processos.comando_recebido == 0 && wait_ret != ETIMEDOUT) {
                wait_ret = pthread_cond_timedwait(&gerenciador->comunicacao_gerenciador_processos.cond,
                                                &gerenciador->comunicacao_gerenciador_processos.mutex,
                                                &timeout);
            }

            if (wait_ret == ETIMEDOUT) {
                printf("[Gerenciador UTE %ld] TIMEOUT esperando PID %d! Processo pode estar travado.\n", gerenciador->tempo_simulacao_global, gerenciador->cpu_sistema.processo_atual ? gerenciador->cpu_sistema.processo_atual->pid : -1);
            } else {
                // printf("[Gerenciador UTE %ld] PID %d (Thread) sinalizou conclusão.\n", gerenciador->tempo_simulacao_global, gerenciador->comunicacao_gerenciador_processos.comando_recebido -1);
            }
            gerenciador->comunicacao_gerenciador_processos.comando_recebido = 0; // Resetar sinal
            pthread_mutex_unlock(&gerenciador->comunicacao_gerenciador_processos.mutex);

            // Após a thread do processo rodar, verificar seu estado e atualizar o quantum usado
            ProcessoSimulado_t *proc_que_rodou = gerenciador->cpu_sistema.processo_atual;
            if (proc_que_rodou) { // Se ainda está marcado como na CPU
                pthread_mutex_lock(&proc_que_rodou->proc_mutex);
                if (proc_que_rodou->estado_atual == EST_EXECUCAO) { // Thread completou instrução, mas não bloqueou/terminou
                    proc_que_rodou->tempo_usado_no_quantum_atual++;
                    printf("[Gerenciador UTE %ld] PID %d completou instrução, permanece em EXECUCAO (será PRONTO no prox UTE).\n", gerenciador->tempo_simulacao_global, proc_que_rodou->pid);
                } else if (proc_que_rodou->estado_atual == EST_BLOQUEADO) {
                    printf("[Gerenciador UTE %ld] PID %d auto-bloqueou.\n", gerenciador->tempo_simulacao_global, proc_que_rodou->pid);
                    proc_que_rodou->tempo_usado_no_quantum_atual = 0;
                    gerenciador->cpu_sistema.processo_atual = NULL; 
                    // Não precisa adicionar à fila de bloqueados aqui, a thread ou psExecutar já fez
                } else if (proc_que_rodou->estado_atual == EST_TERMINADO) {
                    printf("[Gerenciador UTE %ld] PID %d auto-terminou.\n", gerenciador->tempo_simulacao_global, proc_que_rodou->pid);
                    gerenciador->cpu_sistema.processo_atual = NULL;
                }
                pthread_mutex_unlock(&proc_que_rodou->proc_mutex);
            }
        }
    } else {
        printf("[Gerenciador UTE %ld] CPU Ociosa (nenhum processo pronto).\n", gerenciador->tempo_simulacao_global);
        gerenciador->cpu_sistema.processo_atual = NULL;
    }

    gerenciador->tempo_simulacao_global++;
    printf("[Gerenciador UTE %ld] << FIM UNIDADE DE TEMPO >>\n\n", gerenciador->tempo_simulacao_global -1);
    pthread_mutex_unlock(&gerenciador->mutex_geral_gerenciador);
}


// Função auxiliar para imprimir estado atual
static void imprimirEstadoAtual(GerenciadorDeProcessos_t *gerenciador) {
    printf("[Gerenciador] I → solicitada impressão do estado atual.\n");
    ProcessoImpressao_t *impressao = (ProcessoImpressao_t *)malloc(sizeof(ProcessoImpressao_t));
    if (impressao) {
        impressao->gerenciador = gerenciador;
        impressao->tipo_impressao = 0;
        processoImpressaoImprimirEstado(impressao);
        free(impressao);
    }
}

// Função auxiliar para imprimir estatísticas finais
static void imprimirEstatisticasFinais(GerenciadorDeProcessos_t *gerenciador) {
    printf("[Gerenciador] M → impressão final e encerramento do simulador.\n");
    ProcessoImpressao_t *impressao = (ProcessoImpressao_t *)malloc(sizeof(ProcessoImpressao_t));
    if (impressao) {
        impressao->gerenciador = gerenciador;
        impressao->tipo_impressao = 1;
        processoImpressaoImprimirEstatisticas(impressao);
        free(impressao);
    }
    
    // Finaliza todas as threads antes de encerrar
    finalizarThreads(gerenciador);
    
    printf("=== Simulador encerrado com sucesso ===\n");
}

// Função principal do gerenciador de processos simulados
void gerenciadorProcessosSimulados(int fd_read, ProcessoSimulado_t *processo_inicial) {
    char comando[MAX_CMD_LEN];
    FILE *pipe_in = fdopen(fd_read, "r");

    if (!pipe_in) {
        perror("fdopen");
        exit(EXIT_FAILURE);
    }

    // Inicializa as estruturas do gerenciador
    GerenciadorDeProcessos_t gerenciador;
    memset(&gerenciador, 0, sizeof(GerenciadorDeProcessos_t));

    // Inicializa a CPU
    inicializarThreads(&gerenciador);
    cpuInicializar(&gerenciador.cpu_sistema);
    estadosInicializarProntos(&gerenciador.processos_prontos);
    estadosInicializarBloqueados(&gerenciador.processos_bloqueados);

    // Copia o processo inicial para a tabela do gerenciador
    gerenciador.tabela_de_processos[0] = *processo_inicial;
    ProcessoSimulado_t* processo_inicial_na_tabela = &gerenciador.tabela_de_processos[0];
    gerenciador.slot_tabela_ocupado[0] = 1;

    ThreadArgs_t *args_init = (ThreadArgs_t*)malloc(sizeof(ThreadArgs_t));
        if (!args_init) {
            perror("Falha ao alocar memória para args_init");
            // Limpeza antes de sair
            fclose(pipe_in); // fd_read foi aberto como pipe_in
            // Libera estruturas do gerenciador que foram inicializadas
            estadosLiberarProntos(&gerenciador.processos_prontos);
            estadosLiberarBloqueados(&gerenciador.processos_bloqueados);
            // Destruir mutexes e semáforos inicializados em inicializarThreads
            pthread_mutex_destroy(&gerenciador.mutex_geral_gerenciador);
            pthread_mutex_destroy(&gerenciador.mutex_fila_prontos); // Se inicializado em inicializarThreads
            pthread_mutex_destroy(&gerenciador.mutex_fila_bloqueados); // Se inicializado em inicializarThreads
            sem_destroy(&gerenciador.sem_impressao); // Se inicializado em inicializarThreads
            pthread_mutex_destroy(&gerenciador.comunicacao_gerenciador_processos.mutex);
            pthread_cond_destroy(&gerenciador.comunicacao_gerenciador_processos.cond);
            exit(EXIT_FAILURE);
        }
        args_init->processo = processo_inicial_na_tabela;
        args_init->gerenciador = &gerenciador;

        if (pthread_create(&processo_inicial_na_tabela->thread_id, NULL, executarProcessoThread, args_init) != 0) {
            perror("Falha ao criar thread para processo inicial");
            free(args_init); // Libera args_init, pois a thread não foi criada com sucesso

            // Limpeza mais completa antes de sair
            fclose(pipe_in);
            estadosLiberarProntos(&gerenciador.processos_prontos);
            estadosLiberarBloqueados(&gerenciador.processos_bloqueados);
            
            pthread_mutex_destroy(&gerenciador.mutex_geral_gerenciador);
            pthread_mutex_destroy(&gerenciador.mutex_fila_prontos);
            pthread_mutex_destroy(&gerenciador.mutex_fila_bloqueados);
            sem_destroy(&gerenciador.sem_impressao);
            pthread_mutex_destroy(&gerenciador.comunicacao_gerenciador_processos.mutex);
            pthread_cond_destroy(&gerenciador.comunicacao_gerenciador_processos.cond);
            
            pthread_mutex_destroy(&processo_inicial_na_tabela->proc_mutex);
            pthread_cond_destroy(&processo_inicial_na_tabela->proc_cond);

            exit(EXIT_FAILURE);
        }

    // Adiciona o processo inicial à fila de prontos
#ifdef USE_FIFO
    estadosAdicionarProntoFIFO(&gerenciador.processos_prontos, processo_inicial->pid);
    printf("[DEBUG FIFO] Processo inicial %d adicionado à fila FIFO\n", processo_inicial->pid);
#else
    estadosAdicionarPronto(&gerenciador.processos_prontos, processo_inicial->pid, processo_inicial->prioridade);
    // Removido o printf duplicado que estava aqui, pois ele já existe na função estadosAdicionarPronto
#endif

    printf("=== Gerenciador de Processos Iniciado ===\n");
    printf("Aguardando comandos (U, I, M) do processo controle...\n\n");

    while (fgets(comando, sizeof(comando), pipe_in)) {
        comando[strcspn(comando, "\r\n")] = '\0';
        if (strlen(comando) == 0) continue;

        printf("[Gerenciador] Comando recebido: '%s'\n", comando);

        switch (comando[0]) {
            case 'U':
                executarUnidadeTempo(&gerenciador);
                break;

            case 'I':
                imprimirEstadoAtual(&gerenciador);
                break;

            case 'M':
                imprimirEstatisticasFinais(&gerenciador);
                // Libera a memória dos processos na tabela
                for (int i = 0; i < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; i++) {
                    if (gerenciador.slot_tabela_ocupado[i]) {
                        psLiberarListaInstrucoes(&gerenciador.tabela_de_processos[i].listaInstrucoes);
                    }
                }
                // Libera a memória das estruturas de estado
                estadosLiberarProntos(&gerenciador.processos_prontos);
                estadosLiberarBloqueados(&gerenciador.processos_bloqueados);
                fclose(pipe_in);
                return;

            default:
                printf("[Gerenciador] Comando inválido: '%s'\n", comando);
                break;
        }
    }

    fclose(pipe_in);
}

void atribuirPidAoProcesso(GerenciadorDeProcessos_t *gerenciador, ProcessoSimulado_t *processo) {
    int pid = 0; // Começa a busca do PID 0
    for (pid = 0; pid < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; pid++) {
        if (!gerenciador->slot_tabela_ocupado[pid]) {
            break;
        }
    }
    if (pid == MAX_PROCESSOS_SIMULADOS_NO_SISTEMA) {
        printf("[Gerenciador] ERRO: Limite de processos (%d) atingido. Não é possível criar novo processo.\n", MAX_PROCESSOS_SIMULADOS_NO_SISTEMA);
        processo->pid = -1; // Indicar falha
        return;
    }
    processo->pid = pid; // Atribui o PID encontrado ao processo
    gerenciador->slot_tabela_ocupado[pid] = 1; // Marca o slot como ocupado
    printf("[Gerenciador] PID %d atribuído ao novo processo.\n", pid);
}
