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
//Função de escalonamento de processos
void escalonarProcessos(GerenciadorDeProcessos_t *gerenciador) {
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
    printf("[Gerenciador] U → fim de unidade de tempo. Executando próxima instrução, incrementando contador e escalonando.\n");

    // Primeiro, escalona um processo para a CPU
#ifdef USE_FIFO
    escalonarProcessosFIFO(gerenciador);
#else
    escalonarProcessos(gerenciador);
#endif

    // Executa próxima instrução do processo atual
    if (gerenciador->cpu_sistema.processo_atual) {
        ProcessoSimulado_t *novo_filho = NULL;
        ProcessoSimulado_t *processo_atual = gerenciador->cpu_sistema.processo_atual;
        
        // Incrementa o tempo de CPU usado
        processo_atual->tempo_total_cpu_usado++;
        gerenciador->cpu_sistema.tempo_executado_neste_quantum++;

        printf("[DEBUG] Executando instrução para processo %d (PC=%d)\n", 
               processo_atual->pid, processo_atual->pc);
        
        psExecutarProximaInstrucao(processo_atual, 
                                gerenciador->tempo_simulacao_global,
                                &novo_filho);
                            
        // Se um novo processo filho foi criado, adiciona-o ao gerenciador
        if (novo_filho) {
            // Atribui um PID ao processo filho
            atribuirPidAoProcesso(gerenciador, novo_filho);
            
            // Faz uma cópia do processo filho para a tabela
            ProcessoSimulado_t *processo_tabela = &gerenciador->tabela_de_processos[novo_filho->pid];
            memcpy(processo_tabela, novo_filho, sizeof(ProcessoSimulado_t));
            
            // Inicializa a lista de instruções do processo na tabela
            psInicializarListaInstrucoes(&processo_tabela->listaInstrucoes);
            
            // Copia profundamente a lista de instruções
            psCopiarListaInstrucoes(&processo_tabela->listaInstrucoes, &novo_filho->listaInstrucoes);
            
            // Adiciona à fila de prontos
#ifdef USE_FIFO
            estadosAdicionarProntoFIFO(&gerenciador->processos_prontos, novo_filho->pid);
            printf("[Gerenciador FIFO] Novo processo filho %d adicionado à fila FIFO\n", novo_filho->pid);
#else
            estadosAdicionarPronto(&gerenciador->processos_prontos,
                novo_filho->pid,
                novo_filho->prioridade);
            printf("[Gerenciador] Novo processo filho %d adicionado à fila de prontos\n", novo_filho->pid);
#endif
            
            // Libera a memória do processo temporário
            psLiberarMemoria(novo_filho);
        }
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
