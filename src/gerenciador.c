#include "../include/gerenciador.h"
#include "../include/processoSimulado.h" // Para psCriarNovo, psCarregarProgramaDeArquivo, psLiberarMemoria, ThreadArgs_t
#include "../include/estados.h"         // Para estadosInicializar*, estadosAdicionar*, estadosRemover* e seus mutexes
#include "../include/cpu.h"             // Para cpuInicializar, CPU_OCIOSA (estrutura da "Ana")
#include "../include/processoImpressao.h" // Para processoImpressaoIniciar
#include "../include/config.h"          // Para globais (mutexes, constantes NUM_NIVEIS_PRIORIDADE, QUANTUM_*)
#include "../include/threads.h"         // Para protótipo de finalizarThreads (se mantido em threads.h)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <time.h> // Se time(NULL) for usado, mas geralmente passamos tempo_global_simulador
#include <unistd.h>  // Para usleep (se usado)
#include <pthread.h>

// Mutexes globais (tabela_processos_mutex, prontos_mutex, bloqueados_mutex)
// são declarados 'extern' em config.h e devem ser definidos em main.c.

// Função auxiliar para calcular quantum baseado na prioridade
static int calcular_quantum_para_prioridade(int prioridade) {
    switch (prioridade) {
        case 0: return QUANTUM_PRIORIDADE_0; // Estes QUANTUM_* devem estar definidos em config.h
        case 1: return QUANTUM_PRIORIDADE_1;
        case 2: return QUANTUM_PRIORIDADE_2;
        case 3: return QUANTUM_PRIORIDADE_3;
        default: 
            fprintf(stderr, "GERENCIADOR AVISO: Prioridade %d inválida para cálculo de quantum. Usando quantum da Prio %d.\n", prioridade, NUM_NIVEIS_PRIORIDADE -1);
            return QUANTUM_PRIORIDADE_3; // Fallback para o quantum da prioridade mais baixa
    }
}

// Função para configurar um novo processo na tabela do gerenciador (essencial para o novo modelo)
int configurarNovoProcessoNaTabela(GerenciadorDeProcessos_t *gerenciador,
                                     ProcessoSimulado_t *info_novo_processo, // Struct temporária com dados pré-carregados
                                     int pid_pai,
                                     int prioridade_sugerida, 
                                     long tempo_chegada) {
    if (!gerenciador || !info_novo_processo) {
        if(info_novo_processo) {
            // Se a struct temporária foi alocada, precisa ser liberada se não for usada.
            // psLiberarMemoria libera a struct e sua lista de instruções.
            psLiberarMemoria(info_novo_processo);
        }
        return -1;
    }

    pthread_mutex_lock(&tabela_processos_mutex);
    int novo_pid = -1;
    for (int i = 0; i < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; i++) {
        if (!gerenciador->slot_tabela_ocupado[i]) {
            novo_pid = i;
            break;
        }
    }

    if (novo_pid == -1) {
        pthread_mutex_unlock(&tabela_processos_mutex);
        fprintf(stderr, "GERENCIADOR ERRO: Limite de processos (%d) atingido. Não foi possível criar novo processo.\n", MAX_PROCESSOS_SIMULADOS_NO_SISTEMA);
        psLiberarMemoria(info_novo_processo); // Libera a struct temporária e sua lista de instruções
        return -1;
    }

    gerenciador->slot_tabela_ocupado[novo_pid] = 1;
    ProcessoSimulado_t *processo_alvo_na_tabela = &gerenciador->tabela_de_processos[novo_pid];
    
    // Copia os dados da struct temporária para a tabela.
    // A posse da lista de instruções é transferida.
    *processo_alvo_na_tabela = *info_novo_processo; 
    
    processo_alvo_na_tabela->pid = novo_pid; 
    processo_alvo_na_tabela->pid_pai = pid_pai;
    // O estado deve ser PRONTO para que o escalonador possa pegá-lo.
    // psCriarNovo define como EST_NOVO. Aqui, após configuração, vai para PRONTO.
    processo_alvo_na_tabela->estado_atual = EST_PRONTO; 
    processo_alvo_na_tabela->tempo_chegada_sistema = tempo_chegada;
    
    // Define a prioridade inicial
    if (novo_pid == 0 && pid_pai == -1) { // Processo inicial do sistema (PID 0)
        processo_alvo_na_tabela->prioridade = 0; // Prioridade mais alta
    } else { // Processo filho criado por 'F'
        processo_alvo_na_tabela->prioridade = prioridade_sugerida; // Herda prioridade do pai
    }
    // Garante que a prioridade esteja dentro dos limites válidos
    if (processo_alvo_na_tabela->prioridade < 0) processo_alvo_na_tabela->prioridade = 0;
    if (processo_alvo_na_tabela->prioridade >= NUM_NIVEIS_PRIORIDADE) processo_alvo_na_tabela->prioridade = NUM_NIVEIS_PRIORIDADE - 1;

    // Zera contadores e campos relacionados à execução para um processo recém-adicionado à tabela
    processo_alvo_na_tabela->tempo_total_cpu_usado = 0;
    processo_alvo_na_tabela->tempo_restante_bloqueio = 0;
    processo_alvo_na_tabela->quantum_alocado_atual = 0; // Será definido pelo escalonador ao despachar
    processo_alvo_na_tabela->tempo_usado_no_quantum_atual = 0;
    processo_alvo_na_tabela->thread = 0; // Nenhuma thread POSIX associada ainda

    // Importante: A lista de instruções de info_novo_processo foi "movida" (ponteiros copiados).
    // Para evitar double free quando info_novo_processo (a struct temporária) for liberada pelo chamador,
    // zeramos a lista aqui para que psLiberarMemoria em info_novo_processo não tente liberar a lista novamente.
    psInicializarListaInstrucoes(&info_novo_processo->listaInstrucoes);

    printf("GERENCIADOR: PID %d (Pai: %d) configurado na tabela. Prio: %d, PC: %d, Chegada: %ld, Instruções: %d.\n",
           novo_pid, pid_pai, processo_alvo_na_tabela->prioridade, processo_alvo_na_tabela->pc, 
           processo_alvo_na_tabela->tempo_chegada_sistema, processo_alvo_na_tabela->listaInstrucoes.tamanho);

    pthread_mutex_unlock(&tabela_processos_mutex);
    return novo_pid;
}

// Nova função de escalonamento que despacha threads para processos prontos
void escalonarProcessosThreads(GerenciadorDeProcessos_t *gerenciador) {
    if (!gerenciador) return;

    pthread_mutex_lock(&tabela_processos_mutex); 
    if (gerenciador->processo_ativo_pid != -1) {
        // Já existe uma thread de processo simulado ativa (ou que foi despachada e ainda não terminou/cedeu)
        pthread_mutex_unlock(&tabela_processos_mutex);
        return; 
    }
    // Se chegou aqui, processo_ativo_pid é -1 (CPU ociosa), então podemos tentar escalar.
    pthread_mutex_unlock(&tabela_processos_mutex); 

    // estadosRemoverPronto() já busca da fila de maior prioridade não vazia e deve ser thread-safe.
    int proximo_pid = estadosRemoverPronto(&gerenciador->processos_prontos);

    if (proximo_pid == -1) {
        // Nenhuma alteração no gerenciador->processo_ativo_pid (continua -1).
        gerenciador->cpu_sistema.indice_processo_na_tabela = CPU_OCIOSA; // Mantém CPU da "Ana" ociosa
        // printf("[Gerenciador Escalonador] Nenhuma processo pronto para execução.\n");
        return;
    }
    
    pthread_mutex_lock(&tabela_processos_mutex); // Protege acesso à tabela e estado do gerenciador

    // Valida o PID obtido da fila
    if (proximo_pid < 0 || proximo_pid >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA || !gerenciador->slot_tabela_ocupado[proximo_pid]) {
        fprintf(stderr, "ESCALONADOR ERRO: PID %d inválido ou slot não ocupado após remover da fila de prontos.\n", proximo_pid);
        gerenciador->processo_ativo_pid = -1; // Garante que CPU permaneça/fique ociosa
        gerenciador->cpu_sistema.indice_processo_na_tabela = CPU_OCIOSA;
        pthread_mutex_unlock(&tabela_processos_mutex);
        return;
    }

    ProcessoSimulado_t *processo_para_executar = &gerenciador->tabela_de_processos[proximo_pid];

    // Confirma se o processo está realmente pronto para ser executado
    if (processo_para_executar->estado_atual != EST_PRONTO) {
         fprintf(stderr, "ESCALONADOR AVISO: PID %d (Prio %d) estava na fila de prontos, mas seu estado atual é %s. Não será escalonado.\n",
                proximo_pid, processo_para_executar->prioridade, estadoParaString(processo_para_executar->estado_atual));
        gerenciador->processo_ativo_pid = -1; // Garante que CPU permaneça/fique ociosa
        gerenciador->cpu_sistema.indice_processo_na_tabela = CPU_OCIOSA;
        // Opcional: Readicionar à fila de prontos se o estado for recuperável ou logar erro.
        pthread_mutex_unlock(&tabela_processos_mutex);
        return;
    }
    
    // Configura o processo para execução
    processo_para_executar->estado_atual = EST_EXECUCAO;
    processo_para_executar->quantum_alocado_atual = calcular_quantum_para_prioridade(processo_para_executar->prioridade);
    processo_para_executar->tempo_usado_no_quantum_atual = 0; // Reseta ao ser despachado
    
    // Atualiza o estado do gerenciador e da CPU da "Ana"
    gerenciador->processo_ativo_pid = proximo_pid; 
    gerenciador->cpu_sistema.processo_atual = processo_para_executar; 
    gerenciador->cpu_sistema.indice_processo_na_tabela = proximo_pid;
    gerenciador->cpu_sistema.pc_registrador_cpu = processo_para_executar->pc; 
    gerenciador->cpu_sistema.quantum_total_alocado = processo_para_executar->quantum_alocado_atual; // Sincroniza com struct CPU da "Ana"
    gerenciador->cpu_sistema.tempo_executado_neste_quantum = 0; // Sincroniza com struct CPU da "Ana"

    printf("ESCALONADOR: Despachando PID %d (Prio %d, PC=%d, Quantum=%d) para sua thread.\n",
           proximo_pid, processo_para_executar->prioridade, processo_para_executar->pc, processo_para_executar->quantum_alocado_atual);
    
    // Prepara argumentos para a thread
    ThreadArgs_t *args = (ThreadArgs_t*)malloc(sizeof(ThreadArgs_t));
    if (!args) {
        perror("ESCALONADOR ERRO: malloc para ThreadArgs_t falhou");
        processo_para_executar->estado_atual = EST_TERMINADO; // Marca como terminado em caso de falha crítica
        gerenciador->processo_ativo_pid = -1; 
        gerenciador->cpu_sistema.indice_processo_na_tabela = CPU_OCIOSA;
        pthread_mutex_unlock(&tabela_processos_mutex);
        return;
    }
    args->processo = processo_para_executar;
    args->gerenciador = gerenciador;

    // Cria a thread para o processo
    if (pthread_create(&processo_para_executar->thread, NULL, psExecutarProcesso, (void*)args) != 0) {
        perror("ESCALONADOR ERRO: pthread_create falhou para PID");
        fprintf(stderr, " %d\n", processo_para_executar->pid);
        processo_para_executar->estado_atual = EST_TERMINADO; 
        gerenciador->processo_ativo_pid = -1;
        gerenciador->cpu_sistema.indice_processo_na_tabela = CPU_OCIOSA;
        free(args); // Libera args se a criação da thread falhar
    } else {
        // Destaca a thread para que seus recursos sejam liberados automaticamente ao terminar.
        // O gerenciador não precisará dar pthread_join nela, exceto no final da simulação.
        if (pthread_detach(processo_para_executar->thread) != 0) {
            perror("ESCALONADOR AVISO: pthread_detach falhou para PID");
            fprintf(stderr, " %d\n", processo_para_executar->pid);
            // Não é fatal, mas pode levar a recursos presos se o join não for feito.
        }
    }
    pthread_mutex_unlock(&tabela_processos_mutex);
}

// Nova função executarUnidadeTempo, focada no tempo e processos bloqueados
void executarUnidadeTempo(GerenciadorDeProcessos_t *gerenciador) {
    if (!gerenciador) return;

    // Avança o tempo global da simulação
    pthread_mutex_lock(&tabela_processos_mutex); // Protege tempo_simulacao_global e acesso à tabela
    gerenciador->tempo_simulacao_global++;
    printf("\n[Gerenciador] U -> Tempo Global da Simulação: %ld\n", gerenciador->tempo_simulacao_global);
    pthread_mutex_unlock(&tabela_processos_mutex);

    // --- Processar processos bloqueados (despertar) ---
    int pids_bloqueados_para_checar[MAX_PROCESSOS_SIMULADOS_NO_SISTEMA];
    int num_pids_bloqueados = 0;

    // 1. Coleta PIDs da fila de bloqueados de forma segura
    pthread_mutex_lock(&bloqueados_mutex);
    FilaProcessos_t *fila_bloqueados_original = &gerenciador->processos_bloqueados.fila_geral_bloqueados;
    if (fila_bloqueados_original->tamanho > 0) {
        int idx_temp = fila_bloqueados_original->inicio_fila;
        for (int i = 0; i < fila_bloqueados_original->tamanho; i++) {
            pids_bloqueados_para_checar[num_pids_bloqueados++] = fila_bloqueados_original->elementos[idx_temp];
            idx_temp = (idx_temp + 1) % fila_bloqueados_original->capacidade;
        }
    }
    pthread_mutex_unlock(&bloqueados_mutex);

    // 2. Itera sobre os PIDs coletados para processar o desbloqueio
    for (int i = 0; i < num_pids_bloqueados; i++) {
        int pid_bloqueado = pids_bloqueados_para_checar[i];
        int deve_desbloquear = 0;
        int prioridade_ao_desbloquear = -1; // Prioridade que o processo terá ao ir para pronto

        pthread_mutex_lock(&tabela_processos_mutex); // Protege acesso à tabela de processos
        // Verifica se o PID ainda é válido e se o processo existe e está realmente bloqueado
        if (pid_bloqueado < 0 || pid_bloqueado >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA || !gerenciador->slot_tabela_ocupado[pid_bloqueado]) {
            pthread_mutex_unlock(&tabela_processos_mutex);
            continue; // PID inválido ou slot não está mais ocupado
        }
        ProcessoSimulado_t *processo_em_verificacao = &gerenciador->tabela_de_processos[pid_bloqueado];
        
        if (processo_em_verificacao->estado_atual == EST_BLOQUEADO) { // Confirma que ainda está bloqueado
            if (processo_em_verificacao->tempo_restante_bloqueio > 0) {
                processo_em_verificacao->tempo_restante_bloqueio--;
            }
            if (processo_em_verificacao->tempo_restante_bloqueio == 0) { // Tempo de bloqueio expirou
                deve_desbloquear = 1;
                processo_em_verificacao->estado_atual = EST_PRONTO;
                // A prioridade já foi ajustada em psExecutarProcesso no momento do bloqueio (se aplicável).
                // Aqui, usamos a prioridade atual do processo.
                prioridade_ao_desbloquear = processo_em_verificacao->prioridade;
                processo_em_verificacao->tempo_usado_no_quantum_atual = 0; // Reseta para próxima execução
            }
        }
        pthread_mutex_unlock(&tabela_processos_mutex);

        if (deve_desbloquear) {
            printf("[Gerenciador] PID %d desbloqueado. Movendo para PRONTO com Prioridade %d.\n", 
                   pid_bloqueado, prioridade_ao_desbloquear);
            
            // Remove da fila de bloqueados (esta função deve ser thread-safe)
            estadosRemoverBloqueadoEspecifico(&gerenciador->processos_bloqueados, pid_bloqueado);
            
            // Adicionar à fila de prontos correta (esta função deve ser thread-safe)
            estadosAdicionarPronto(&gerenciador->processos_prontos, pid_bloqueado, prioridade_ao_desbloquear);
        }
    }
    
    // A execução de instruções agora é feita pelas threads dos processos.
    // O gerenciador apenas tenta escalar um novo processo se a CPU estiver ociosa.
    escalonarProcessosThreads(gerenciador); 
}

// Funções static para impressão, chamando a nova processoImpressaoIniciar
static void imprimirEstadoAtual(GerenciadorDeProcessos_t *gerenciador) {
    printf("[Gerenciador] I -> Solicitada impressão do estado atual do sistema.\n");
    processoImpressaoIniciar(gerenciador, 0); // 0 para impressão de estado atual
}

static void imprimirEstatisticasFinais(GerenciadorDeProcessos_t *gerenciador) {
    printf("[Gerenciador] M -> Solicitada impressão final e encerramento do simulador.\n");
    processoImpressaoIniciar(gerenciador, 1); // 1 para impressão de estatísticas finais

    // Pequena pausa para dar chance à thread de impressão (que é destacada) de executar.
    // Uma sincronização mais robusta (ex: semáforo ou join na thread de impressão se não fosse destacada)
    // seria ideal se o encerramento dependesse criticamente da conclusão da impressão.
    usleep(500000); // 0.5 segundos (ajuste conforme necessário)

    // Chama a função para finalizar (dar join) todas as threads de processo simulado.
    // O protótipo de finalizarThreads deve estar em threads.h (incluído).
    finalizarThreads(gerenciador); 
    
    printf("=== Simulador encerrado com sucesso pelo Gerenciador ===\n");
}

// Função principal do gerenciador de processos simulados
void gerenciadorProcessosSimulados(int fd_read, ProcessoSimulado_t *processo_inicial_info_main) {
    char comando[MAX_CMD_LEN];
    FILE *pipe_in = fdopen(fd_read, "r");

    if (!pipe_in) {
        perror("ERRO: fdopen no gerenciador falhou");
        if (processo_inicial_info_main) {
            // Se a struct temporária ainda existe, liberar
            psLiberarMemoria(processo_inicial_info_main);
        }
        exit(EXIT_FAILURE);
    }

    GerenciadorDeProcessos_t gerenciador;
    memset(&gerenciador, 0, sizeof(GerenciadorDeProcessos_t)); // Zera a estrutura do gerenciador

    // Mutexes globais (tabela_processos_mutex, prontos_mutex, bloqueados_mutex, impressao_mutex)
    // são inicializados em main.c.

    gerenciador.tempo_simulacao_global = 0;
    gerenciador.processo_ativo_pid = -1; // Indica que a CPU está conceitualmente ociosa

    cpuInicializar(&gerenciador.cpu_sistema); // Inicializa a estrutura CPU da "Ana"
    gerenciador.cpu_sistema.indice_processo_na_tabela = CPU_OCIOSA; // Sincroniza com o estado ocioso

    // Inicializa as filas de processos prontos e bloqueados
    estadosInicializarProntos(&gerenciador.processos_prontos);
    estadosInicializarBloqueados(&gerenciador.processos_bloqueados);

    // Configurar o processo inicial (PID 0)
    printf("[Gerenciador] Configurando processo inicial (PID 0) na tabela de processos...\n");
    // processo_inicial_info_main é uma struct temporária que contém o programa já carregado.
    // configurarNovoProcessoNaTabela irá copiá-lo para a tabela do gerenciador.
    int pid_inicial = configurarNovoProcessoNaTabela(&gerenciador, processo_inicial_info_main, -1, 0, 0L);
                                                // pid_pai=-1, prioridade_sugerida=0 (mais alta), tempo_chegada=0
    
    if (pid_inicial != -1) { // Se a configuração do processo inicial foi bem-sucedida
        if (processo_inicial_info_main) {
            // A struct temporária processo_inicial_info_main teve sua lista de instruções "movida" (ponteiros zerados)
            // pela configurarNovoProcessoNaTabela. Agora podemos liberar apenas o container da struct.
            free(processo_inicial_info_main); // Libera o container da struct temporária
            processo_inicial_info_main = NULL;
        }
        // Adiciona o processo inicial (agora na tabela do gerenciador) à fila de prontos.
        // estadosAdicionarPronto deve ser thread-safe.
        estadosAdicionarPronto(&gerenciador.processos_prontos, pid_inicial, gerenciador.tabela_de_processos[pid_inicial].prioridade);
    } else { 
        // Falha crítica ao configurar o processo inicial
        fprintf(stderr, "[Gerenciador] ERRO FATAL: Falha ao configurar o processo inicial na tabela.\n");
        if (processo_inicial_info_main) {
            // configurarNovoProcessoNaTabela já deve ter chamado psLiberarMemoria em caso de falha.
            // Se não, seria necessário aqui. Por segurança, podemos verificar se ainda é não nulo.
            // psLiberarMemoria(processo_inicial_info_main); 
            processo_inicial_info_main = NULL; 
        }
        fclose(pipe_in);
        // Os mutexes globais são destruídos em main.c no final do programa.
        exit(EXIT_FAILURE);
    }
    
    printf("=== Gerenciador de Processos (Modelo Threads com MLFQ) Iniciado ===\n");
    printf("[Gerenciador] Tempo Global da Simulação: %ld\n", gerenciador.tempo_simulacao_global);

    // Tenta escalonar o processo inicial (ou qualquer outro pronto)
    escalonarProcessosThreads(&gerenciador);

    // Loop principal de tratamento de comandos
    while (fgets(comando, sizeof(comando), pipe_in)) {
        comando[strcspn(comando, "\r\n")] = '\0'; // Remove newline
        if (strlen(comando) == 0) continue; // Ignora linhas vazias

        printf("[Gerenciador] Comando recebido do processo controle: '%s'\n", comando);

        switch (comando[0]) {
            case 'U':
                executarUnidadeTempo(&gerenciador);
                break;
            case 'I':
                imprimirEstadoAtual(&gerenciador);
                break;
            case 'M':
                imprimirEstatisticasFinais(&gerenciador); // Esta função agora também chama finalizarThreads
                
                // Liberação final de recursos do gerenciador
                pthread_mutex_lock(&tabela_processos_mutex);
                for (int i = 0; i < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; i++) {
                    if (gerenciador.slot_tabela_ocupado[i]) {
                        // A thread do processo já deve ter terminado e sido juntada por finalizarThreads.
                        // Aqui, liberamos a lista de instruções, que é o principal recurso do processo simulado.
                        psLiberarListaInstrucoes(&gerenciador.tabela_de_processos[i].listaInstrucoes);
                        gerenciador.slot_tabela_ocupado[i] = 0; // Marca o slot como livre
                    }
                }
                pthread_mutex_unlock(&tabela_processos_mutex);
                
                estadosLiberarProntos(&gerenciador.processos_prontos); // Libera memória das filas de prontos
                estadosLiberarBloqueados(&gerenciador.processos_bloqueados); // Libera memória da fila de bloqueados
                
                fclose(pipe_in); // Fecha o pipe de leitura
                // Mutexes globais são destruídos em main.c.
                return; // Encerra a função gerenciadorProcessosSimulados (e o processo gerenciador)

            default:
                printf("[Gerenciador] AVISO: Comando inválido recebido: '%s'\n", comando);
                break;
        }
    }

    // Se o loop terminar por EOF no pipe (sem um comando 'M' explícito)
    printf("[Gerenciador] Fim dos comandos (EOF no pipe). Iniciando encerramento da simulação...\n");
    imprimirEstatisticasFinais(&gerenciador); // Trata como 'M' para garantir limpeza e finalização de threads
    
    // Repete a limpeza de recursos para garantir (semelhante ao caso 'M')
    pthread_mutex_lock(&tabela_processos_mutex);
    for (int i = 0; i < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; i++) {
        if (gerenciador.slot_tabela_ocupado[i]) {
            psLiberarListaInstrucoes(&gerenciador.tabela_de_processos[i].listaInstrucoes);
            gerenciador.slot_tabela_ocupado[i] = 0;
        }
    }
    pthread_mutex_unlock(&tabela_processos_mutex);
    estadosLiberarProntos(&gerenciador.processos_prontos);
    estadosLiberarBloqueados(&gerenciador.processos_bloqueados);
    
    fclose(pipe_in);
    // Mutexes globais são destruídos em main.c.
}

// As seguintes funções da versão "Ana" original de src/gerenciador.c
// devem ser REMOVIDAS deste arquivo, pois suas responsabilidades foram
// absorvidas pelo novo modelo ou por novas funções:
// - criarProcessoSimulado (a antiga que estava aqui)
// - substituirImagemProcesso
// - gerenciarTransicoesEstados
// - escalonarProcessos (a antiga versão MLFQ não-thread)
// - escalonarProcessosFIFO (se não for adaptada e usada)
// - trocarContexto
// - atribuirPidAoProcesso