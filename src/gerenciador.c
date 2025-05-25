#include "gerenciador.h"
#include "processoImpressao.h" // Para processoImpressaoIniciar
#include "config.h"            // Para constantes e declarações extern de mutexes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>             // Para close (pipe_in), usleep
#include <pthread.h>            // Para mutexes (já incluído via config.h)

// Definição dos mutexes globais (declarados extern em config.h)
pthread_mutex_t prontos_mutex;
pthread_mutex_t bloqueados_mutex;
pthread_mutex_t tabela_processos_mutex;

static int calcular_quantum_para_prioridade(int prioridade) {
    switch (prioridade) {
        case 0: return QUANTUM_PRIO_0;
        case 1: return QUANTUM_PRIO_1;
        case 2: return QUANTUM_PRIO_2;
        case 3: return QUANTUM_PRIO_3;
        default: 
            fprintf(stderr, "GERENCIADOR AVISO: Prioridade inválida %d para cálculo de quantum. Usando quantum da prioridade mais baixa.\n", prioridade);
            return QUANTUM_PRIO_3; // Fallback
    }
}


static void escalonarProximoProcessoThread(GerenciadorDeProcessos_t *gerenciador) {
    if (!gerenciador) return;

    pthread_mutex_lock(&tabela_processos_mutex);
    if (gerenciador->processo_ativo_pid != -1) {
        // CPU já está ocupada por uma thread de processo ativo.
        pthread_mutex_unlock(&tabela_processos_mutex);
        return;
    }
    pthread_mutex_unlock(&tabela_processos_mutex); 

    // estadosRemoverPronto() já busca da fila de maior prioridade não vazia.
    int proximo_pid = estadosRemoverPronto(&gerenciador->processos_prontos);

    if (proximo_pid == -1) {
        // printf("ESCALONADOR: Nenhuma thread nas filas de prontos.\n");
        return; // Nenhuma processo pronto para executar
    }

    pthread_mutex_lock(&tabela_processos_mutex);
    if (proximo_pid < 0 || proximo_pid >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA || !gerenciador->slot_tabela_ocupado[proximo_pid]) {
        fprintf(stderr, "ESCALONADOR ERRO: PID %d inválido ou slot não ocupado, removido da(s) fila(s) de prontos.\n", proximo_pid);
        pthread_mutex_unlock(&tabela_processos_mutex);
        return;
    }

    ProcessoSimulado_t *processo_para_executar = &gerenciador->tabela_de_processos[proximo_pid];

    if (processo_para_executar->estado_atual != EST_PRONTO) {
         fprintf(stderr, "ESCALONADOR AVISO: PID %d (Prio %d) em estado %s, esperado PRONTO. Não escalonando.\n",
                proximo_pid, processo_para_executar->prioridade, estadoParaString(processo_para_executar->estado_atual));
        pthread_mutex_unlock(&tabela_processos_mutex);
        return; // Evita escalonar processo que não está realmente pronto
    }
    
    processo_para_executar->estado_atual = EST_EXECUCAO;
    processo_para_executar->quantum_alocado_atual = calcular_quantum_para_prioridade(processo_para_executar->prioridade);
    processo_para_executar->tempo_usado_no_quantum_atual = 0; // Reseta ao ser despachado
    
    gerenciador->processo_ativo_pid = proximo_pid; // Marca este PID como ativo

    printf("ESCALONADOR: Despachando PID %d (Prio %d, PC=%d, Quantum=%d).\n",
           proximo_pid, processo_para_executar->prioridade, processo_para_executar->pc, processo_para_executar->quantum_alocado_atual);
    
    ThreadArgs_t *args = (ThreadArgs_t*)malloc(sizeof(ThreadArgs_t));
    if (!args) {
        perror("ESCALONADOR ERRO: malloc para ThreadArgs_t falhou");
        processo_para_executar->estado_atual = EST_TERMINADO; // Falha crítica
        gerenciador->processo_ativo_pid = -1; // Libera CPU
        pthread_mutex_unlock(&tabela_processos_mutex);
        return;
    }
    args->processo = processo_para_executar;
    args->gerenciador = gerenciador;

    if (pthread_create(&processo_para_executar->thread_id, NULL, psExecutarProcesso, (void*)args) != 0) {
        perror("ESCALONADOR ERRO: pthread_create falhou");
        processo_para_executar->estado_atual = EST_TERMINADO; // Marca como terminado para não tentar de novo
        gerenciador->processo_ativo_pid = -1; // Libera CPU
        free(args); // Libera args se a thread não foi criada
    }
    // A thread foi criada (ou falhou). O mutex é liberado.
    // Se a criação da thread falhar, o processo_ativo_pid já foi resetado.
    pthread_mutex_unlock(&tabela_processos_mutex);
}


static void executarUnidadeTempo(GerenciadorDeProcessos_t *gerenciador) {
    if (!gerenciador) return;

    pthread_mutex_lock(&tabela_processos_mutex);
    gerenciador->tempo_simulacao_global++;
    printf("\nGERENCIADOR: Comando 'U'. Tempo Global: %ld\n", gerenciador->tempo_simulacao_global);
    pthread_mutex_unlock(&tabela_processos_mutex);

    // Processar processos bloqueados (despertar)
    int pids_bloqueados_para_checar[MAX_PROCESSOS_SIMULADOS_NO_SISTEMA];
    int num_pids_bloqueados = 0;

    pthread_mutex_lock(&bloqueados_mutex);
    FilaProcessos_t *fila_bloq = &gerenciador->processos_bloqueados.fila_geral_bloqueados;
    if (fila_bloq->tamanho > 0) {
        int current_idx_bloq = fila_bloq->inicio_fila;
        for (int i = 0; i < fila_bloq->tamanho; i++) {
            pids_bloqueados_para_checar[num_pids_bloqueados++] = fila_bloq->elementos[current_idx_bloq];
            current_idx_bloq = (current_idx_bloq + 1) % fila_bloq->capacidade;
        }
    }
    pthread_mutex_unlock(&bloqueados_mutex);

    for (int i = 0; i < num_pids_bloqueados; i++) {
        int pid_bloqueado = pids_bloqueados_para_checar[i];
        int deve_desbloquear = 0;
        int prioridade_final_desbloqueio = -1;

        pthread_mutex_lock(&tabela_processos_mutex);
        if (pid_bloqueado < 0 || pid_bloqueado >= MAX_PROCESSOS_SIMULADOS_NO_SISTEMA || !gerenciador->slot_tabela_ocupado[pid_bloqueado]) {
            pthread_mutex_unlock(&tabela_processos_mutex);
            continue; // PID inválido ou slot não mais ocupado
        }
        ProcessoSimulado_t *proc = &gerenciador->tabela_de_processos[pid_bloqueado];
        
        if (proc->estado_atual == EST_BLOQUEADO) {
            if (proc->tempo_restante_bloqueio > 0) {
                proc->tempo_restante_bloqueio--;
            }
            if (proc->tempo_restante_bloqueio == 0) {
                deve_desbloquear = 1;
                // Aumentar prioridade ao desbloquear (antes de expirar quantum), conforme especificação [cite: 198]
                proc->prioridade = (proc->prioridade > 0) ? (proc->prioridade - 1) : 0;
                proc->estado_atual = EST_PRONTO;
                proc->tempo_usado_no_quantum_atual = 0; // Resetar quantum usado
                prioridade_final_desbloqueio = proc->prioridade;
            }
        }
        pthread_mutex_unlock(&tabela_processos_mutex);

        if (deve_desbloquear) {
            printf("GERENCIADOR: PID %d desbloqueado. Nova Prio %d. Movendo para PRONTO.\n", 
                   pid_bloqueado, prioridade_final_desbloqueio);
            
            // Remover da fila de bloqueados (thread-safe)
            estadosRemoverBloqueadoEspecifico(&gerenciador->processos_bloqueados, pid_bloqueado);
            
            // Adicionar à fila de prontos correta (thread-safe)
            estadosAdicionarPronto(&gerenciador->processos_prontos, pid_bloqueado, prioridade_final_desbloqueio);
        }
    }
    // Tentar escalonar um novo processo se a "CPU" estiver conceitualmente livre.
    escalonarProximoProcessoThread(gerenciador);
}


static void imprimirEstadoAtual(GerenciadorDeProcessos_t *gerenciador) {
    if (!gerenciador) return;
    printf("GERENCIADOR: Comando 'I'. Solicitando impressão do estado atual.\n");
    processoImpressaoIniciar(gerenciador, 0); // 0 para impressão de estado atual
}

static void finalizarSimulacao(GerenciadorDeProcessos_t *gerenciador) {
    if (!gerenciador) return;
    printf("GERENCIADOR: Comando 'M'. Iniciando finalização da simulação.\n");
    processoImpressaoIniciar(gerenciador, 1); // 1 para impressão de estatísticas finais

    // Aguardar um tempo para a thread de impressão (assíncrona) ter chance de rodar.
    // Uma solução mais robusta envolveria um mecanismo de sinalização ou join na thread de impressão.
    usleep(200000); // 200ms (ajuste conforme necessário)

    printf("GERENCIADOR: Aguardando todas as threads de processos simulados terminarem...\n");
    pthread_mutex_lock(&tabela_processos_mutex);
    for (int i = 0; i < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; i++) {
        if (gerenciador->slot_tabela_ocupado[i] && gerenciador->tabela_de_processos[i].thread_id != 0) {
            // printf("GERENCIADOR: Tentando join na thread do PID %d (Estado: %s)...\n", 
            //        i, estadoParaString(gerenciador->tabela_de_processos[i].estado_atual));
            int ret_join = pthread_join(gerenciador->tabela_de_processos[i].thread_id, NULL);
            if (ret_join != 0) {
                // ESRCH (No such process) é comum se a thread já saiu. Outros erros podem ser mais sérios.
                // fprintf(stderr, "GERENCIADOR AVISO: pthread_join para PID %d falhou com código %d.\n", i, ret_join);
            } else {
                // printf("GERENCIADOR: Join bem-sucedido para PID %d.\n", i);
            }
            gerenciador->tabela_de_processos[i].thread_id = 0; // Marca que a thread foi juntada
        }
        // Libera a lista de instruções, pois o processo não será mais usado.
        // slot_tabela_ocupado é marcado como 0 aqui para indicar que o slot está livre
        // e os dados podem ser sobrescritos se a simulação continuasse (o que não é o caso aqui).
        if (gerenciador->slot_tabela_ocupado[i]) { // Checa novamente, pois o estado pode ter mudado
             psLiberarListaInstrucoes(&gerenciador->tabela_de_processos[i].listaInstrucoes);
             gerenciador->slot_tabela_ocupado[i] = 0; 
        }
    }
    pthread_mutex_unlock(&tabela_processos_mutex);

    estadosLiberarProntos(&gerenciador->processos_prontos);
    estadosLiberarBloqueados(&gerenciador->processos_bloqueados);

    printf("GERENCIADOR: Simulação encerrada.\n");
}


// Configura um novo processo na tabela de processos do gerenciador.
int configurarNovoProcessoNaTabela(GerenciadorDeProcessos_t *gerenciador,
                                     ProcessoSimulado_t *info_novo_processo,
                                     int pid_pai,
                                     int prioridade_sugerida, // Para filhos, é a do pai. Para PID 0, é 0.
                                     long tempo_chegada) {
    if (!gerenciador || !info_novo_processo) return -1;

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
        // A struct info_novo_processo e sua lista de instruções devem ser liberadas pelo chamador
        // se a configuração falhar aqui, ou por esta função.
        psLiberarMemoria(info_novo_processo); 
        return -1;
    }

    gerenciador->slot_tabela_ocupado[novo_pid] = 1;
    ProcessoSimulado_t *processo_alvo_na_tabela = &gerenciador->tabela_de_processos[novo_pid];

    // Copia os dados da estrutura temporária. A posse da lista de instruções é transferida.
    *processo_alvo_na_tabela = *info_novo_processo; 
    
    processo_alvo_na_tabela->pid = novo_pid; 
    processo_alvo_na_tabela->pid_pai = pid_pai;
    processo_alvo_na_tabela->estado_atual = EST_PRONTO; // Novo processo entra como PRONTO
    processo_alvo_na_tabela->tempo_chegada_sistema = tempo_chegada;
    
    // Definir prioridade inicial
    if (novo_pid == 0 && pid_pai == -1) { // Processo inicial (PID 0)
        processo_alvo_na_tabela->prioridade = 0; // Mais alta prioridade [cite: 189]
    } else { // Processo filho
        processo_alvo_na_tabela->prioridade = prioridade_sugerida; // Herda do pai [cite: 181]
    }
    // Garante que a prioridade esteja dentro dos limites válidos
    if (processo_alvo_na_tabela->prioridade < 0) processo_alvo_na_tabela->prioridade = 0;
    if (processo_alvo_na_tabela->prioridade >= NUM_NIVEIS_PRIORIDADE) processo_alvo_na_tabela->prioridade = NUM_NIVEIS_PRIORIDADE - 1;


    // Reseta campos relacionados à execução e MLFQ
    processo_alvo_na_tabela->tempo_total_cpu_usado = 0;
    processo_alvo_na_tabela->tempo_restante_bloqueio = 0;
    processo_alvo_na_tabela->quantum_alocado_atual = 0; // Será definido pelo escalonador ao despachar
    processo_alvo_na_tabela->tempo_usado_no_quantum_atual = 0;
    processo_alvo_na_tabela->thread_id = 0; // Nenhuma thread associada ainda

    // Zera a lista na struct temporária para evitar double free, pois os ponteiros foram movidos.
    psInicializarListaInstrucoes(&info_novo_processo->listaInstrucoes);


    printf("GERENCIADOR: PID %d configurado (Pai: %d, Prio: %d, PC: %d, Chegada: %ld, Instruções: %d).\n",
           novo_pid, pid_pai, processo_alvo_na_tabela->prioridade, processo_alvo_na_tabela->pc, 
           processo_alvo_na_tabela->tempo_chegada_sistema, processo_alvo_na_tabela->listaInstrucoes.tamanho);

    pthread_mutex_unlock(&tabela_processos_mutex);
    return novo_pid;
}


// Função principal do gerenciador de processos simulados
void gerenciadorProcessosSimulados(int fd_read_pipe, ProcessoSimulado_t *info_processo_inicial_main) {
    char comando[MAX_CMD_LEN];
    FILE *pipe_in = fdopen(fd_read_pipe, "r");

    if (!pipe_in) {
        perror("GERENCIADOR ERRO: fdopen no pipe de entrada falhou");
        if (info_processo_inicial_main) psLiberarMemoria(info_processo_inicial_main); 
        exit(EXIT_FAILURE);
    }

    GerenciadorDeProcessos_t gerenciador;
    memset(&gerenciador, 0, sizeof(GerenciadorDeProcessos_t)); // Zera a estrutura do gerenciador

    // Inicializar mutexes
    if (pthread_mutex_init(&prontos_mutex, NULL) != 0 ||
        pthread_mutex_init(&bloqueados_mutex, NULL) != 0 ||
        pthread_mutex_init(&tabela_processos_mutex, NULL) != 0) {
        perror("GERENCIADOR ERRO: Falha ao inicializar mutexes");
        fclose(pipe_in);
        if (info_processo_inicial_main) psLiberarMemoria(info_processo_inicial_main);
        exit(EXIT_FAILURE);
    }

    gerenciador.tempo_simulacao_global = 0;
    gerenciador.processo_ativo_pid = -1; // CPU inicialmente ociosa
    
    estadosInicializarProntos(&gerenciador.processos_prontos);
    estadosInicializarBloqueados(&gerenciador.processos_bloqueados);

    // Configurar o processo inicial (PID 0)
    printf("GERENCIADOR: Configurando processo inicial (PID 0)...\n");
    // O processo inicial (PID 0) começa com prioridade 0 e tempo de chegada 0.
    int pid_inicial = configurarNovoProcessoNaTabela(&gerenciador, info_processo_inicial_main, -1, 0, 0L);
    
    // A struct temporária info_processo_inicial_main teve seus dados (incluindo lista de instruções)
    // movidos para a tabela do gerenciador. A lista em info_processo_inicial_main foi zerada.
    // Agora podemos liberar a struct temporária.
    if (info_processo_inicial_main) { // Checagem de segurança
        free(info_processo_inicial_main); 
        info_processo_inicial_main = NULL;
    }

    if (pid_inicial == -1) {
        fprintf(stderr, "GERENCIADOR ERRO FATAL: Falha ao configurar processo inicial.\n");
        // Limpeza de mutexes e pipe
        pthread_mutex_destroy(&prontos_mutex);
        pthread_mutex_destroy(&bloqueados_mutex);
        pthread_mutex_destroy(&tabela_processos_mutex);
        fclose(pipe_in);
        exit(EXIT_FAILURE);
    }
    // Adiciona o processo inicial à fila de prontos com sua prioridade definida
    // (configurarNovoProcessoNaTabela já define o estado como PRONTO e a prioridade).
    estadosAdicionarPronto(&gerenciador.processos_prontos, pid_inicial, gerenciador.tabela_de_processos[pid_inicial].prioridade);
    
    printf("GERENCIADOR: === Gerenciador de Processos (Modelo Threads com MLFQ) Iniciado ===\n");
    printf("GERENCIADOR: Tempo Global: %ld\n", gerenciador.tempo_simulacao_global);

    // Primeira tentativa de escalonamento para iniciar o processo inicial, se houver
    escalonarProximoProcessoThread(&gerenciador);

    while (fgets(comando, sizeof(comando), pipe_in)) {
        comando[strcspn(comando, "\r\n")] = '\0'; // Remove newline
        if (strlen(comando) == 0) continue; // Ignora linhas vazias

        // printf("GERENCIADOR DBG: Comando recebido do pipe: '%s'\n", comando);

        switch (comando[0]) {
            case 'U':
                executarUnidadeTempo(&gerenciador);
                break;
            case 'I':
                imprimirEstadoAtual(&gerenciador);
                break;
            case 'M':
                finalizarSimulacao(&gerenciador);
                // Destruir mutexes após garantir que não serão mais usados
                pthread_mutex_destroy(&prontos_mutex);
                pthread_mutex_destroy(&bloqueados_mutex);
                pthread_mutex_destroy(&tabela_processos_mutex);
                fclose(pipe_in);
                printf("GERENCIADOR: Finalizado após comando 'M'.\n");
                return; // Sai da função gerenciadorProcessosSimulados
            default:
                fprintf(stderr, "GERENCIADOR AVISO: Comando inválido recebido: '%s'\n", comando);
                break;
        }
    }

    // Se o loop terminar por EOF no pipe (sem comando 'M')
    printf("GERENCIADOR: Fim dos comandos (EOF no pipe). Encerrando simulação...\n");
    finalizarSimulacao(&gerenciador); 
    pthread_mutex_destroy(&prontos_mutex);
    pthread_mutex_destroy(&bloqueados_mutex);
    pthread_mutex_destroy(&tabela_processos_mutex);
    fclose(pipe_in);
}