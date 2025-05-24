#include "gerenciador.h"
#include "processoSimulado.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    int resultado_thread = pthread_create(&proximo_processo->thread, NULL, psExecutarProcesso, (void*)proximo_processo);
    if (resultado_thread != 0) {
        fprintf(stderr, "[Erro] Falha ao criar thread para o processo %d\n", proximo_processo->pid);
        proximo_processo->estado_atual = EST_TERMINADO;
        gerenciador->cpu_sistema.processo_atual = NULL;
        return;
    }
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
// Função auxiliar para executar uma unidade de tempo
static void executarUnidadeTempo(GerenciadorDeProcessos_t *gerenciador) {
    printf("[Gerenciador] U → fim de unidade de tempo.\n"); // Mensagem mais concisa

    // --- Processar processos bloqueados (despertar) ---
    int pids_bloqueados_para_checar[MAX_PROCESSOS_SIMULADOS_NO_SISTEMA];
    int num_pids_bloqueados = 0;
    FilaProcessos_t *fila_bloqueados_original = &gerenciador->processos_bloqueados.fila_geral_bloqueados;

    // Coleta PIDs da fila de bloqueados para processamento seguro
    int idx_temp_bloqueado = fila_bloqueados_original->inicio_fila;
    for (int i = 0; i < fila_bloqueados_original->tamanho; i++) {
        pids_bloqueados_para_checar[num_pids_bloqueados++] = fila_bloqueados_original->elementos[idx_temp_bloqueado];
        idx_temp_bloqueado = (idx_temp_bloqueado + 1) % fila_bloqueados_original->capacidade;
    }

    FilaProcessos_t nova_fila_bloqueados; // Fila temporária para os que continuam bloqueados
    filaInicializar(&nova_fila_bloqueados);

    for (int i = 0; i < num_pids_bloqueados; i++) {
        int pid_bloqueado = pids_bloqueados_para_checar[i];
        // Validação do PID antes de acessar a tabela
        if (pid_bloqueado < 0 || pid_bloqueado >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA || !gerenciador->slot_tabela_ocupado[pid_bloqueado]) {
            printf("[Gerenciador] AVISO: PID %d inválido encontrado na lista de checagem de bloqueados.\n", pid_bloqueado);
            continue;
        }
        ProcessoSimulado_t *processo_bloqueado = &gerenciador->tabela_de_processos[pid_bloqueado];

        if (processo_bloqueado->estado_atual == EST_BLOQUEADO) { // Checa se ainda está de fato bloqueado
            if (processo_bloqueado->tempo_restante_bloqueio > 0) {
                processo_bloqueado->tempo_restante_bloqueio--;
            }

            if (processo_bloqueado->tempo_restante_bloqueio == 0) {
                printf("[Gerenciador] PID %d desbloqueado. Movendo para pronto.\n", pid_bloqueado);
                processo_bloqueado->estado_atual = EST_PRONTO;
                #ifdef USE_FIFO
                    estadosAdicionarProntoFIFO(&gerenciador->processos_prontos, pid_bloqueado);
                #else
                    // A prioridade pode ter sido aumentada quando ele bloqueou
                    estadosAdicionarPronto(&gerenciador->processos_prontos, pid_bloqueado, processo_bloqueado->prioridade);
                #endif
            } else {
                // Se ainda estiver bloqueado (tempo_restante > 0), adiciona à nova fila de bloqueados
                filaEnfileirar(&nova_fila_bloqueados, pid_bloqueado);
            }
        }
        // Se o estado não for EST_BLOQUEADO, ele não é adicionado de volta à nova_fila_bloqueados.
    }
    // Libera a fila de bloqueados antiga e substitui pela nova (que contém os ainda bloqueados)
    filaLiberarMemoria(fila_bloqueados_original);
    gerenciador->processos_bloqueados.fila_geral_bloqueados = nova_fila_bloqueados;
#ifdef USE_FIFO
    escalonarProcessosFIFO(gerenciador);
#else
    escalonarProcessos(gerenciador); // Esta função foi ajustada para o rebaixamento correto
#endif

    // --- Executar próxima instrução do processo atual na CPU ---
    if (gerenciador->cpu_sistema.processo_atual) {
        ProcessoSimulado_t *novo_filho = NULL;
        ProcessoSimulado_t *processo_atual_na_cpu = gerenciador->cpu_sistema.processo_atual;

        // Só executa se o processo na CPU estiver de fato em estado de execução
        if (processo_atual_na_cpu->estado_atual == EST_EXECUCAO) {
            // Incrementa o tempo de CPU usado PELO PROCESSO e o tempo NA CPU NESTE QUANTUM
            processo_atual_na_cpu->tempo_total_cpu_usado++;
            gerenciador->cpu_sistema.tempo_executado_neste_quantum++; // Este é usado pelo escalonador por prioridade

            printf("[DEBUG Gerenciador] Executando instrução para PID %d (PC=%d)\n",
                    processo_atual_na_cpu->pid, processo_atual_na_cpu->pc);

            psExecutarProximaInstrucao(processo_atual_na_cpu,
                                    gerenciador->tempo_simulacao_global,
                                    &novo_filho);

            // Se um novo processo filho foi criado
            if (novo_filho) {
                atribuirPidAoProcesso(gerenciador, novo_filho); // Define novo_filho->pid e marca slot
                ProcessoSimulado_t *processo_filho_na_tabela = &gerenciador->tabela_de_processos[novo_filho->pid];

                // Copia dados do processo temporário 'novo_filho' para a tabela de processos
                processo_filho_na_tabela->pid = novo_filho->pid;
                processo_filho_na_tabela->pid_pai = novo_filho->pid_pai;
                processo_filho_na_tabela->estado_atual = novo_filho->estado_atual;
                processo_filho_na_tabela->prioridade = novo_filho->prioridade;
                processo_filho_na_tabela->pc = novo_filho->pc;
                memcpy(processo_filho_na_tabela->memoria, novo_filho->memoria, sizeof(processo_filho_na_tabela->memoria));
                processo_filho_na_tabela->num_variaveis_declaradas = novo_filho->num_variaveis_declaradas;
                processo_filho_na_tabela->tempo_chegada_sistema = novo_filho->tempo_chegada_sistema;
                processo_filho_na_tabela->tempo_total_cpu_usado = 0;
                processo_filho_na_tabela->tempo_restante_bloqueio = 0;
                processo_filho_na_tabela->tempo_usado_no_quantum_atual = 0;

                psInicializarListaInstrucoes(&processo_filho_na_tabela->listaInstrucoes);
                psCopiarListaInstrucoes(&processo_filho_na_tabela->listaInstrucoes, &novo_filho->listaInstrucoes);

                #ifdef USE_FIFO
                    estadosAdicionarProntoFIFO(&gerenciador->processos_prontos, processo_filho_na_tabela->pid);
                #else
                    estadosAdicionarPronto(&gerenciador->processos_prontos, processo_filho_na_tabela->pid, processo_filho_na_tabela->prioridade);
                #endif
                printf("[Gerenciador] Novo processo filho %d adicionado.\n", processo_filho_na_tabela->pid);

                psLiberarMemoria(novo_filho); // Libera a estrutura temporária 'novo_filho'
            }

            // Tratar se o processo_atual_na_cpu terminou ou bloqueou APÓS a execução da instrução
            if (processo_atual_na_cpu->estado_atual == EST_TERMINADO) {
                printf("[Gerenciador] Processo PID %d terminou.\n", processo_atual_na_cpu->pid);
                // Recursos (como lista de instruções) são liberados no comando 'M' ou quando o slot é reutilizado.
            } else if (processo_atual_na_cpu->estado_atual == EST_BLOQUEADO) {
                printf("[Gerenciador] Processo PID %d bloqueou (tempo para despertar: %d).\n",
                       processo_atual_na_cpu->pid, processo_atual_na_cpu->tempo_restante_bloqueio);
                // Adiciona à fila de bloqueados para ser gerenciado pela lógica de despertar
                estadosAdicionarBloqueado(&gerenciador->processos_bloqueados, processo_atual_na_cpu->pid);
            }
        } else {
            printf("[Gerenciador] AVISO: Processo %d na CPU não está em estado de EXECUCAO (estado: %d). Não executará instrução.\n",
                    processo_atual_na_cpu->pid, processo_atual_na_cpu->estado_atual);
        }
    } else {
        // Ninguém na CPU após o escalonamento (fila de prontos estava vazia)
        printf("[Gerenciador] CPU Ociosa nesta unidade de tempo.\n");
    }

    // Incrementa o tempo global da simulação
    gerenciador->tempo_simulacao_global++;
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
    cpuInicializar(&gerenciador.cpu_sistema);

    // Inicializa os estados
    estadosInicializarProntos(&gerenciador.processos_prontos);
    estadosInicializarBloqueados(&gerenciador.processos_bloqueados);

    // Copia o processo inicial para a tabela do gerenciador
    gerenciador.tabela_de_processos[0] = *processo_inicial;
    gerenciador.slot_tabela_ocupado[0] = 1;

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
    // Procura o primeiro slot livre na tabela de processos
    int pid = 0;
    for (pid = 0; pid < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; pid++) {
        if (!gerenciador->slot_tabela_ocupado[pid]) {
            break;
        }
    }
    if (pid == MAX_PROCESSOS_SIMULADOS_NO_SISTEMA) {
        printf("Limite de processos atingido.\n");
        return;
    }
    // Atribui o PID ao processo
    processo->pid = pid;
    // Marca o slot como ocupado
    gerenciador->slot_tabela_ocupado[pid] = 1;
    printf("[DEBUG] PID %d atribuído ao processo\n", pid);
}
