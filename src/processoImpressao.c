#include "processoImpressao.h"
#include "gerenciador.h"      // Para a definição completa de GerenciadorDeProcessos_t
#include "processoSimulado.h" // Para ProcessoSimulado_t e estadoParaString()
#include "config.h"           // Para NUM_NIVEIS_PRIORIDADE e mutexes extern
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>           // Para usleep (opcional)
#include <pthread.h>


static void* threadFuncaoImpressao(void* arg) {
    ImpressaoArgs_t *args = (ImpressaoArgs_t*) arg;
    if (!args || !args->gerenciador) {
        fprintf(stderr, "IMPRESSAO ERRO: Argumentos inválidos para thread de impressão.\n");
        if (args) free(args);
        pthread_exit(NULL);
    }

    GerenciadorDeProcessos_t *gerenciador = args->gerenciador;
    int tipo = args->tipo_impressao;
    free(args); // Libera a estrutura de argumentos alocada em processoImpressaoIniciar

    // Tenta adquirir o mutex de impressão para garantir que apenas uma impressão ocorra por vez.
    // printf("DEBUG IMPRESSAO: Thread de impressão (Tipo: %d) aguardando mutex...\n", tipo);
    if (pthread_mutex_lock(&impressao_mutex) != 0) {
        perror("IMPRESSAO ERRO: Falha ao bloquear impressao_mutex");
        pthread_exit(NULL);
    }
    // printf("DEBUG IMPRESSAO: Thread de impressão (Tipo: %d) obteve mutex.\n", tipo);

    if (tipo == 0) { // Impressão de estado atual
        processoImpressaoImprimirEstado(gerenciador);
    } else { // Impressão de estatísticas finais (tipo == 1)
        processoImpressaoImprimirEstatisticas(gerenciador);
    }

    // printf("DEBUG IMPRESSAO: Thread de impressão (Tipo: %d) liberando mutex.\n", tipo);
    if (pthread_mutex_unlock(&impressao_mutex) != 0) {
        perror("IMPRESSAO ERRO: Falha ao desbloquear impressao_mutex");
    }
    
    pthread_exit(NULL);
}

void processoImpressaoIniciar(GerenciadorDeProcessos_t *gerenciador, int tipo_impressao) {
    if (!gerenciador) {
        fprintf(stderr, "IMPRESSAO ERRO: Gerenciador nulo ao tentar iniciar impressão.\n");
        return;
    }
    pthread_t thread_id_impressao;
    ImpressaoArgs_t *args = (ImpressaoArgs_t*)malloc(sizeof(ImpressaoArgs_t));

    if (!args) {
        perror("IMPRESSAO ERRO: Falha ao alocar memória para ImpressaoArgs_t");
        return;
    }
    args->gerenciador = gerenciador;
    args->tipo_impressao = tipo_impressao;

    // printf("DEBUG IMPRESSAO CTRL: Tentando disparar thread de impressão (Tipo: %d).\n", tipo_impressao);
    if (pthread_create(&thread_id_impressao, NULL, threadFuncaoImpressao, (void*)args) != 0) {
        perror("IMPRESSAO ERRO: Falha ao criar thread de impressão");
        free(args); // Libera args se a criação da thread falhar
    } else {
        // Destaca a thread para que seus recursos sejam liberados automaticamente ao terminar,
        // e o gerenciador não precise fazer pthread_join nela.
        if (pthread_detach(thread_id_impressao) != 0) {
            perror("IMPRESSAO AVISO: Falha ao destacar thread de impressão");
        }
        // printf("DEBUG IMPRESSAO CTRL: Thread de impressão (Tipo: %d) disparada e destacada.\n", tipo_impressao);
    }
}

// Imprime o estado atual do sistema
void processoImpressaoImprimirEstado(GerenciadorDeProcessos_t *gerenciador_ptr) {
    if (!gerenciador_ptr) {
        printf("IMPRESSAO ERRO: Gerenciador nulo para imprimir estado.\n");
        return;
    }
    GerenciadorDeProcessos_t *gerenciador = gerenciador_ptr; // Para facilitar a leitura

    printf("\n==================== ESTADO ATUAL DO SISTEMA ====================\n");
    
    // Acesso seguro aos dados do gerenciador usando mutexes apropriados
    pthread_mutex_lock(&tabela_processos_mutex); // Protege tempo_global e tabela_de_processos
    printf(" TEMPO GLOBAL DA SIMULAÇÃO: %ld\n", gerenciador->tempo_simulacao_global);
    
    printf(" PROCESSO ATIVO (PID): ");
    if (gerenciador->processo_ativo_pid != -1 && 
        gerenciador->slot_tabela_ocupado[gerenciador->processo_ativo_pid] &&
        gerenciador->tabela_de_processos[gerenciador->processo_ativo_pid].estado_atual == EST_EXECUCAO) {
        
        ProcessoSimulado_t *proc_atual = &gerenciador->tabela_de_processos[gerenciador->processo_ativo_pid];
        printf("%d (Prio %d, PC=%d, Quantum Usado=%d/%d, CPU Time Acumulado=%ld)\n",
               proc_atual->pid, proc_atual->prioridade, proc_atual->pc, 
               proc_atual->tempo_usado_no_quantum_atual, proc_atual->quantum_alocado_atual,
               proc_atual->tempo_total_cpu_usado);
    } else {
        printf("Nenhum (CPU Ociosa)\n");
    }
    pthread_mutex_unlock(&tabela_processos_mutex);

    printf("------------------------------------------------------------------\n");
    printf(" FILAS DE PROCESSOS PRONTOS (Por Prioridade):\n");
    pthread_mutex_lock(&prontos_mutex); // Protege o acesso às filas de prontos
    int total_em_filas_prontos = 0;
    for (int i = 0; i < NUM_NIVEIS_PRIORIDADE; i++) {
        printf("  Prioridade %d: ", i);
        FilaProcessos_t *fila_prio = &gerenciador->processos_prontos.filas_prontos[i];
        if (!filaEstaVazia(fila_prio)) {
            printf("\n");
            int idx = fila_prio->inicio_fila;
            for (int count = 0; count < fila_prio->tamanho; count++) {
                int pid = fila_prio->elementos[idx];
                total_em_filas_prontos++;
                
                // Para acessar dados do processo na tabela, precisamos do mutex da tabela
                pthread_mutex_lock(&tabela_processos_mutex); 
                if (pid >= 0 && pid < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA && gerenciador->slot_tabela_ocupado[pid]) {
                    ProcessoSimulado_t *p = &gerenciador->tabela_de_processos[pid];
                    // Confirma se o estado é PRONTO, para consistência
                    if (p->estado_atual == EST_PRONTO) {
                         printf("    -> PID: %-3d (PC: %-3d, Chegada: %-3ld, CPU Time: %ld)\n",
                               p->pid, p->pc, p->tempo_chegada_sistema, p->tempo_total_cpu_usado);
                    } else {
                        printf("    -> PID: %-3d (Estado Inconsistente: %s, esperado PRONTO)\n", pid, estadoParaString(p->estado_atual));
                    }
                } else {
                    printf("    -> PID: %-3d (Inválido ou slot não ocupado na tabela)\n", pid);
                }
                pthread_mutex_unlock(&tabela_processos_mutex);
                idx = (idx + 1) % fila_prio->capacidade;
            }
        } else {
            printf("Vazia.\n");
        }
    }
    if(total_em_filas_prontos == 0) {
        // printf("  (Todas as filas de prontos estão vazias)\n");
    }
    pthread_mutex_unlock(&prontos_mutex);
    printf("------------------------------------------------------------------\n");

    printf(" FILA DE PROCESSOS BLOQUEADOS:\n");
    pthread_mutex_lock(&bloqueados_mutex); // Protege o acesso à fila de bloqueados
    FilaProcessos_t *fila_bloqueados = &gerenciador->processos_bloqueados.fila_geral_bloqueados;
    if (!filaEstaVazia(fila_bloqueados)) {
        int idx = fila_bloqueados->inicio_fila;
        for (int count = 0; count < fila_bloqueados->tamanho; count++) {
            int pid = fila_bloqueados->elementos[idx];
            
            pthread_mutex_lock(&tabela_processos_mutex); 
            if (pid >= 0 && pid < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA && gerenciador->slot_tabela_ocupado[pid]) {
                ProcessoSimulado_t *p = &gerenciador->tabela_de_processos[pid];
                // Confirma se o estado é BLOQUEADO
                if (p->estado_atual == EST_BLOQUEADO) {
                    printf("  -> PID: %-3d (Prio %d, Tempo Restante Bloqueio: %-3d)\n",
                           p->pid, p->prioridade, p->tempo_restante_bloqueio);
                } else {
                     printf("  -> PID: %-3d (Estado Inconsistente: %s, esperado BLOQUEADO)\n", pid, estadoParaString(p->estado_atual));
                }
            } else {
                 printf("    -> PID: %-3d (Inválido ou slot não ocupado na tabela)\n", pid);
            }
            pthread_mutex_unlock(&tabela_processos_mutex);
            idx = (idx + 1) % fila_bloqueados->capacidade;
        }
    } else {
        printf("  -> Vazia.\n");
    }
    pthread_mutex_unlock(&bloqueados_mutex);
    printf("==================================================================\n\n");
}

// Imprime as estatísticas finais do sistema
void processoImpressaoImprimirEstatisticas(GerenciadorDeProcessos_t *gerenciador_ptr) {
     if (!gerenciador_ptr) {
        printf("IMPRESSAO ERRO: Gerenciador nulo para imprimir estatísticas.\n");
        return;
    }
    GerenciadorDeProcessos_t *gerenciador = gerenciador_ptr;

    printf("\n==================== ESTATÍSTICAS FINAIS ====================\n");
    
    // Lock principal para dados gerais e iteração na tabela de processos
    pthread_mutex_lock(&tabela_processos_mutex);
    printf(" TEMPO TOTAL DE SIMULAÇÃO: %ld unidades\n", gerenciador->tempo_simulacao_global);
    printf("------------------------------------------------------------------\n");
    printf(" ESTATÍSTICAS INDIVIDUAIS DOS PROCESSOS:\n");

    int processos_listados = 0;
    for (int i = 0; i < MAX_PROCESSOS_SIMULADOS_NO_SISTEMA; i++) {
        // Para estatísticas, consideramos um processo se ele foi marcado como ocupado em algum momento
        // ou se é o PID 0 (que sempre existe).
        // Como slot_tabela_ocupado é resetado no final, precisamos de uma condição mais robusta
        // ou garantir que a impressão de estatísticas acesse uma cópia "final" dos dados.
        // Por agora, verificaremos se o processo teve um tempo de chegada ou é o PID 0,
        // ou se seu estado final é TERMINADO (indicando que ele rodou).
        
        ProcessoSimulado_t *p = &gerenciador->tabela_de_processos[i];
        // Imprime se o processo foi inicializado (PID corresponde ao índice e foi usado)
        // OU se o slot ainda está marcado como ocupado (menos provável aqui, pois já deve ter feito join)
        // OU se o estado é TERMINADO (garante que processos que rodaram e terminaram sejam listados).
        if (p->pid == i && (p->tempo_chegada_sistema > 0 || p->pid == 0 || p->estado_atual == EST_TERMINADO || p->tempo_total_cpu_usado > 0 )) {
             // Apenas lista processos que de fato participaram ou foram configurados.
        } else if (p->pid != i) { // Slot não corresponde a um processo inicializado com este PID.
            continue;
        } else if (gerenciador->slot_tabela_ocupado[i]) { // Slot ainda ocupado (improvável se joins ocorreram)
            // continua
        }
         else { // Se não atende aos critérios acima, provavelmente é um slot não usado ou já limpo.
            continue;
        }


        printf("  Processo PID: %d\n", p->pid);
        printf("    Estado Final         : %s\n", estadoParaString(p->estado_atual));
        printf("    PID Pai              : %d\n", p->pid_pai);
        printf("    Prioridade Final     : %d\n", p->prioridade);
        printf("    Tempo de Chegada     : %ld\n", p->tempo_chegada_sistema);
        printf("    Tempo Total CPU Usado: %ld\n", p->tempo_total_cpu_usado);
        printf("    PC Final             : %d\n", p->pc);
        if (p->estado_atual == EST_BLOQUEADO) {
            printf("    Tempo Rest. Bloqueio : %d\n", p->tempo_restante_bloqueio);
        }
        printf("    -----------------------------------\n");
        processos_listados++;
    }

    if (processos_listados == 0) {
        printf("  -> Nenhuma estatística de processo individual para mostrar (além do PID 0 se não rodou).\n");
    }
    pthread_mutex_unlock(&tabela_processos_mutex);
    printf("==================================================================\n\n");
}