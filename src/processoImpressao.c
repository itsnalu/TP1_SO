#include "../include/processoImpressao.h" // Para ImpressaoArgs_t e protótipos
#include "../include/gerenciador.h"      // Para a definição completa de GerenciadorDeProcessos_t
#include "../include/processoSimulado.h" // Para ProcessoSimulado_t e estadoParaString()
#include "../include/config.h"           // Para NUM_NIVEIS_PRIORIDADE e mutexes globais (impressao_mutex, etc.)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // Para usleep 
#include <pthread.h> // Para todas as funções de thread e mutex

// Função wrapper que será executada pela thread de impressão.
// Ela lida com o mutex de impressão e chama a função de impressão apropriada.
static void* threadFuncaoImpressaoWrapper(void* arg) {
    ImpressaoArgs_t *args = (ImpressaoArgs_t*) arg;
    if (!args || !args->gerenciador) {
        fprintf(stderr, "IMPRESSAO ERRO: Argumentos inválidos recebidos pela thread de impressão.\n");
        if (args) free(args); // Libera args se foi alocado mas é inválido
        pthread_exit(NULL);
    }

    // Copia os dados dos argumentos para variáveis locais
    struct GerenciadorDeProcessos_s *gerenciador = args->gerenciador;
    int tipo = args->tipo_impressao;
    
    // Libera a memória da estrutura de argumentos, pois seus dados já foram copiados.
    free(args);

    // Adquire o mutex de impressão para garantir exclusividade na saída
    if (pthread_mutex_lock(&impressao_mutex) != 0) {
        perror("IMPRESSAO ERRO: Falha ao bloquear impressao_mutex na thread de impressão");
        pthread_exit(NULL);
    }

    // Chama a função de impressão apropriada com base no tipo
    if (tipo == 0) { // Impressão de estado atual
        processoImpressaoImprimirEstado(gerenciador);
    } else { // Impressão de estatísticas finais (tipo == 1)
        processoImpressaoImprimirEstatisticas(gerenciador);
    }

    // Libera o mutex de impressão
    if (pthread_mutex_unlock(&impressao_mutex) != 0) {
        perror("IMPRESSAO ERRO: Falha ao desbloquear impressao_mutex na thread de impressão");
    }
    // printf("[Impressao Thread] Mutex de impressão liberado.\n");
    
    pthread_exit(NULL); // Encerra a thread de impressão
}

// Inicia o processo de impressão criando uma thread dedicada.
void processoImpressaoIniciar(struct GerenciadorDeProcessos_s *gerenciador, int tipo_impressao) {
    if (!gerenciador) {
        fprintf(stderr, "IMPRESSAO ERRO: Gerenciador nulo ao tentar iniciar impressão.\n");
        return;
    }

    pthread_t thread_id_impressao;
    // Aloca memória para os argumentos da thread de impressão
    ImpressaoArgs_t *args = (ImpressaoArgs_t*)malloc(sizeof(ImpressaoArgs_t));

    if (!args) {
        perror("IMPRESSAO ERRO: Falha ao alocar memória para ImpressaoArgs_t");
        return;
    }
    // Preenche os argumentos
    args->gerenciador = gerenciador;
    args->tipo_impressao = tipo_impressao;

    // printf("[Impressao Iniciar] Tentando criar thread de impressão (Tipo: %d).\n", tipo_impressao);
    // Cria a thread de impressão
    if (pthread_create(&thread_id_impressao, NULL, threadFuncaoImpressaoWrapper, (void*)args) != 0) {
        perror("IMPRESSAO ERRO: Falha ao criar thread de impressão");
        free(args); // Libera args se a criação da thread falhar
    } else {
        // Destaca a thread para que seus recursos sejam liberados automaticamente ao terminar.
        // O gerenciador não precisará fazer pthread_join nela.
        if (pthread_detach(thread_id_impressao) != 0) {
            perror("IMPRESSAO AVISO: Falha ao destacar (detach) thread de impressão");
            // Não é fatal, mas pode levar a recursos presos se o join não for feito em algum momento.
        }
        // printf("[Impressao Iniciar] Thread de impressão (Tipo: %d) criada e destacada.\n", tipo_impressao);
    }
}

// Imprime o estado atual do sistema (chamada pela thread de impressão).
void processoImpressaoImprimirEstado(struct GerenciadorDeProcessos_s *gerenciador) {
    if (!gerenciador) {
        printf("\nIMPRESSAO ERRO: Ponteiro do gerenciador nulo para imprimir estado.\n");
        return;
    }

    // O impressao_mutex já foi adquirido pela threadFuncaoImpressaoWrapper.

    printf("\n╔═════════════════════════════════════════════════════════════════════════════╗");
    printf("\n║                        ESTADO ATUAL DO SISTEMA                              ║");
    printf("\n╚═════════════════════════════════════════════════════════════════════════════╝\n");

    // Acesso seguro ao tempo global e ao processo ativo
    pthread_mutex_lock(&tabela_processos_mutex); 
    printf("\n┌────────────────────────────────┐");
    printf("\n│ Tempo Global: %-16ld │", gerenciador->tempo_simulacao_global);
    printf("\n└────────────────────────────────┘");

    printf("\n\n┌───── CPU ATIVA (Processo Despachado pela Thread do Gerenciador) ───────────┐");
    int pid_ativo = gerenciador->processo_ativo_pid; 
    
    if (pid_ativo != -1 && gerenciador->slot_tabela_ocupado[pid_ativo]) {
        ProcessoSimulado_t *proc_atual = &gerenciador->tabela_de_processos[pid_ativo];
        if (proc_atual->estado_atual == EST_EXECUCAO) {
            // MODIFICAÇÃO AQUI: Remover (SysThreadID 0x%lx)
            printf("\n│ Processo Ativo: PID %-3d                              │", proc_atual->pid); // SysThreadID removido
            printf("\n│ PC: %-10d Prioridade Atual: %-1d          │", proc_atual->pc, proc_atual->prioridade);
            printf("\n│ Quantum Concedido: %-3d Usado: %-3d              │", proc_atual->quantum_alocado_atual, proc_atual->tempo_usado_no_quantum_atual);
            printf("\n│ Tempo Total de CPU Acumulado: %-10ld         │", proc_atual->tempo_total_cpu_usado);
        } else {
             printf("\n│ PID %-3d marcado como ativo, mas estado é %-10s ! │", pid_ativo, estadoParaString(proc_atual->estado_atual));
        }
    } else {
        printf("\n│                 CPU OCIOSA (Nenhuma thread de processo ativa)             │");
    }
    printf("\n└─────────────────────────────────────────────────────────────────────────────┘");
    pthread_mutex_unlock(&tabela_processos_mutex);


    // Imprime Filas de Processos Prontos
    printf("\n\nFILAS DE PROCESSOS PRONTOS (Escalonador MLFQ):\n");
    pthread_mutex_lock(&prontos_mutex); // Protege o acesso às filas de prontos
    int total_processos_prontos = 0;
    for (int i = 0; i < NUM_NIVEIS_PRIORIDADE; i++) {
        printf("  Prioridade %d: ", i);
        FilaProcessos_t *fila_prio_atual = &gerenciador->processos_prontos.filas_por_prioridade[i];
        if (!filaEstaVazia(fila_prio_atual)) {
            printf("(%d processos)\n", fila_prio_atual->tamanho);
            int idx_fila = fila_prio_atual->inicio_fila;
            for (int count = 0; count < fila_prio_atual->tamanho; count++) {
                int pid_na_fila = fila_prio_atual->elementos[idx_fila];
                total_processos_prontos++;
                
                pthread_mutex_lock(&tabela_processos_mutex); // Bloqueia para ler detalhes do processo
                if (pid_na_fila >= 0 && pid_na_fila < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA && gerenciador->slot_tabela_ocupado[pid_na_fila]) {
                    ProcessoSimulado_t *p_pronto = &gerenciador->tabela_de_processos[pid_na_fila];
                    printf("    -> PID %-3d (PC: %-3d, Chegada: %-3ld, CPU Time: %ld)\n",
                           p_pronto->pid, p_pronto->pc, p_pronto->tempo_chegada_sistema, p_pronto->tempo_total_cpu_usado);
                } else {
                     printf("    -> PID %-3d (INVÁLIDO ou slot não ocupado na tabela de processos)\n", pid_na_fila);
                }
                pthread_mutex_unlock(&tabela_processos_mutex); // Libera após ler detalhes
                idx_fila = (idx_fila + 1) % fila_prio_atual->capacidade;
            }
        } else {
            printf("Vazia.\n");
        }
    }
    if(total_processos_prontos == 0) {
        printf("  (Nenhum processo pronto em nenhuma das filas de prioridade)\n");
    }
    pthread_mutex_unlock(&prontos_mutex); // Libera o acesso às filas de prontos
    printf("-----------------------------------------------------------------------------------\n");

    // Imprime Fila de Processos Bloqueados
    printf("\nFILA DE PROCESSOS BLOQUEADOS:\n");
    pthread_mutex_lock(&bloqueados_mutex); // Protege o acesso à fila de bloqueados
    FilaProcessos_t *fila_de_bloqueados = &gerenciador->processos_bloqueados.fila_geral_bloqueados;
    if (!filaEstaVazia(fila_de_bloqueados)) {
        printf("  (%d processos bloqueados)\n", fila_de_bloqueados->tamanho);
        int idx_fila_b = fila_de_bloqueados->inicio_fila;
        for (int count = 0; count < fila_de_bloqueados->tamanho; count++) {
            int pid_bloq_fila = fila_de_bloqueados->elementos[idx_fila_b];
            
            pthread_mutex_lock(&tabela_processos_mutex); // Bloqueia para ler detalhes do processo
            if (pid_bloq_fila >= 0 && pid_bloq_fila < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA && gerenciador->slot_tabela_ocupado[pid_bloq_fila]) {
                ProcessoSimulado_t *p_bloq = &gerenciador->tabela_de_processos[pid_bloq_fila];
                printf("  -> PID %-3d (Prio: %d, Tempo Restante para Desbloqueio: %-3d)\n",
                       p_bloq->pid, p_bloq->prioridade, p_bloq->tempo_restante_bloqueio);
            } else {
                 printf("    -> PID %-3d (INVÁLIDO ou slot não ocupado na tabela de processos)\n", pid_bloq_fila);
            }
            pthread_mutex_unlock(&tabela_processos_mutex); // Libera após ler detalhes
            idx_fila_b = (idx_fila_b + 1) % fila_de_bloqueados->capacidade;
        }
    } else {
        printf("  -> Vazia.\n");
    }
    pthread_mutex_unlock(&bloqueados_mutex); // Libera o acesso à fila de bloqueados
    printf("\n===================================================================================\n\n");
}

// Imprime as estatísticas finais do sistema (chamada pela thread de impressão).
void processoImpressaoImprimirEstatisticas(struct GerenciadorDeProcessos_s *gerenciador) {
    if (!gerenciador) {
        printf("\nIMPRESSAO ERRO: Ponteiro do gerenciador nulo para imprimir estatísticas.\n");
        return;
    }
    // Cálculo do tempo médio de resposta
    long tempo_total_resposta_calculado = 0;
    int num_processos_terminados_calculado = 0;
    long tempo_simulacao_final = gerenciador->tempo_simulacao_global; // Tempo de término para todos os processos que terminaram até M
    
    // O impressao_mutex já foi adquirido pela threadFuncaoImpressaoWrapper.

    printf("\n╔═════════════════════════════════════════════════════════════════════════════╗");
    printf("\n║                       ESTATÍSTICAS FINAIS DA SIMULAÇÃO                      ║");
    printf("\n╚═════════════════════════════════════════════════════════════════════════════╝\n");
    
    pthread_mutex_lock(&tabela_processos_mutex); // Protege acesso ao tempo global e à tabela de processos
    printf("\n┌───────────────────────────────────────────┐");
    printf("\n│ Tempo Total de Simulação Global: %-10ld │", gerenciador->tempo_simulacao_global);
    printf("\n└───────────────────────────────────────────┘\n");
    
    printf("\n------------------- Estatísticas Individuais dos Processos --------------------\n");
    int processos_listados_stats = 0;
    for (int i = 0; i < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; i++) {
        // Para estatísticas, listamos processos que foram marcados como ocupados em algum momento
        // OU se é o PID 0 (que sempre existe no início) OU se seu estado final é TERMINADO.
        // A verificação de slot_tabela_ocupado[i] pode não ser suficiente se o slot foi limpo
        // antes desta impressão. Portanto, verificamos se p->pid == i e alguma atividade.
        
        ProcessoSimulado_t *p_stat = &gerenciador->tabela_de_processos[i];
        // Listar se o PID no slot corresponde ao índice E (foi o processo inicial OU teve tempo de CPU OU terminou)
        if (p_stat->pid == i && (p_stat->pid == 0 || p_stat->tempo_total_cpu_usado > 0 || p_stat->estado_atual == EST_TERMINADO || gerenciador->slot_tabela_ocupado[i])) {
            processos_listados_stats++;
            printf("\n  Processo PID: %d\n", p_stat->pid);
            printf("    Estado Final Alcançado : %s\n", estadoParaString(p_stat->estado_atual));
            printf("    PID do Processo Pai    : %d\n", p_stat->pid_pai);
            printf("    Prioridade Final       : %d\n", p_stat->prioridade);
            printf("    Tempo de Chegada       : %ld\n", p_stat->tempo_chegada_sistema);
            printf("    Tempo Total CPU Usado  : %ld unidades de tempo\n", p_stat->tempo_total_cpu_usado);
            printf("    PC (Contador Programa) Final: %d\n", p_stat->pc);
            if (p_stat->estado_atual == EST_BLOQUEADO) { // Improvável no final, mas para consistência
                printf("    Tempo Restante Bloqueio: %d\n", p_stat->tempo_restante_bloqueio);
            }
            printf("    ---------------------------------------------------\n");
        }
    }

     for (int i = 0; i < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; i++) {
        if (gerenciador->slot_tabela_ocupado[i]) {
            ProcessoSimulado_t *p_stat = &gerenciador->tabela_de_processos[i];
            if (p_stat->estado_atual == EST_TERMINADO) {
                // Consideramos o tempo de término como o tempo global da simulação no momento do comando 'M'
                // ou, se quisermos ser mais precisos, precisaríamos de um campo "tempo_de_termino" no ProcessoSimulado_t
                // que seria preenchido quando o processo realmente termina.
                // Para simplificar, e dado que 'M' é o fim, tempo_simulacao_final é uma boa aproximação
                // se o processo terminou ANTES do comando 'M'.
                // Se a especificação pede o tempo exato de término, a abordagem anterior (atualizar em psExecutarProcesso) é melhor.
                // Assumindo que tempo_simulacao_final como tempo de término para processos já terminados é aceitável:

                long tempo_de_resposta_individual = tempo_simulacao_final - p_stat->tempo_chegada_sistema;
                if (tempo_de_resposta_individual < 0) tempo_de_resposta_individual = 0; // Sanity check

                tempo_total_resposta_calculado += tempo_de_resposta_individual;
                num_processos_terminados_calculado++;
            }
        }
    }

    // Agora use as variáveis calculadas para a impressão
    printf("\n\n---------------------- Tempo Médio de Resposta ------------------------\n");
    if (num_processos_terminados_calculado > 0) {
        double tempo_medio_resposta = (double)tempo_total_resposta_calculado / num_processos_terminados_calculado;
        printf("┌───────────────────────────────────────────┐\n");
        printf("│ Tempo Médio de Resposta: %-17.2f │\n", tempo_medio_resposta);
        printf("└───────────────────────────────────────────┘\n");
    } else {
        printf("┌───────────────────────────────────────────┐\n");
        printf("│ Nenhum processo terminado para calcular TMR │\n");
        printf("└───────────────────────────────────────────┘\n");
    }

    // O unlock de tabela_processos_mutex ocorre após esta seção no seu código original.
    pthread_mutex_unlock(&tabela_processos_mutex);
    printf("\n===================================================================================\n\n");
}