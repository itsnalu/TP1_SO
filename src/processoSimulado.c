#include "../include/processoSimulado.h" 
#include "../include/gerenciador.h"      
#include "../include/estados.h"          
#include "../include/config.h"           
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <unistd.h>  

// --- Implementação Funções de Lista de Instruções ---
void psInicializarListaInstrucoes(ListaInstrucoes_t *lista) {
    if (!lista) { return; } 
    lista->primeiro = (ApontadorInstrucao_t)malloc(sizeof(CelulaInstrucao_t));
    if (!lista->primeiro) { perror("malloc lista.primeiro em psInicializarListaInstrucoes"); exit(EXIT_FAILURE); }
    lista->ultimo = lista->primeiro;
    lista->primeiro->prox = NULL;
    lista->tamanho = 0;
}

void psLiberarListaInstrucoes(ListaInstrucoes_t *lista) {
    if (!lista) { return; } 
    ApontadorInstrucao_t atual = lista->primeiro;
    ApontadorInstrucao_t temp;
    while (atual != NULL) {
        temp = atual->prox;
        free(atual);
        atual = temp;
    }
    lista->primeiro = lista->ultimo = NULL;
    lista->tamanho = 0;
}

void psInserirInstrucao(ListaInstrucoes_t *lista, Instrucao_t inst) {
    if (!lista || !lista->ultimo) { return; } 
    lista->ultimo->prox = (ApontadorInstrucao_t)malloc(sizeof(CelulaInstrucao_t));
    if (!lista->ultimo->prox) { perror("malloc lista.ultimo->prox em psInserirInstrucao"); exit(EXIT_FAILURE); }
    lista->ultimo = lista->ultimo->prox;
    lista->ultimo->instrucao = inst;
    lista->ultimo->prox = NULL;
    lista->tamanho++;
}

void psCopiarListaInstrucoes(ListaInstrucoes_t *destino, const ListaInstrucoes_t *origem) {
    if (!origem || !destino) {
        return;
    }
    psLiberarListaInstrucoes(destino);
    psInicializarListaInstrucoes(destino);
    if (!origem->primeiro || !origem->primeiro->prox) { 
        return;
    }
    ApontadorInstrucao_t atual_origem = origem->primeiro->prox; 
    while (atual_origem != NULL) {
        psInserirInstrucao(destino, atual_origem->instrucao);
        atual_origem = atual_origem->prox;
    }
}

// Função auxiliar INTERNA para obter um ponteiro para a instrução no PC atual.
// Mantida como static, pois só é usada por psExecutarProcesso neste arquivo.
static Instrucao_t* psObterInstrucaoNoPc(const ProcessoSimulado_t *p) {
    if (!p) {
        return NULL;
    }
    if (p->pc < 0 || p->pc >= p->listaInstrucoes.tamanho) {
        return NULL;
    }
    ApontadorInstrucao_t atual = p->listaInstrucoes.primeiro;
    if (!atual) { 
        return NULL;
    }
    atual = atual->prox; // Pula célula cabeça
    if (!atual && p->listaInstrucoes.tamanho > 0) {
        return NULL;
    } else if (!atual && p->listaInstrucoes.tamanho == 0) { 
         return NULL;
    }

    for (int i = 0; i < p->pc && atual != NULL; ++i) {
        atual = atual->prox;
    }
    if (!atual) {
        return NULL;
    }
    return &(atual->instrucao);
}

// --- Implementação Funções Principais do Processo Simulado ---
const char* estadoParaString(EstadoProcesso_e estado) {
    switch (estado) {
        case EST_NOVO: return "NOVO";
        case EST_PRONTO: return "PRONTO";
        case EST_EXECUCAO: return "EXECUCAO";
        case EST_BLOQUEADO: return "BLOQUEADO";
        case EST_TERMINADO: return "TERMINADO";
        default: return "DESCONHECIDO";
    }
}

ProcessoSimulado_t* psCriarNovo(int pid_pai, int prioridade_sugerida, long tempo_criacao) {
    ProcessoSimulado_t *p = (ProcessoSimulado_t*)malloc(sizeof(ProcessoSimulado_t));
    if (!p) { 
        perror("Falha ao alocar ProcessoSimulado_t em psCriarNovo");
        exit(EXIT_FAILURE); 
    }

    p->pid = -1; 
    p->pid_pai = pid_pai;
    p->estado_atual = EST_NOVO; 
    p->prioridade = prioridade_sugerida; 
    p->pc = 0; 
    psInicializarListaInstrucoes(&p->listaInstrucoes);
    memset(p->memoria, 0, sizeof(p->memoria));
    p->num_variaveis_declaradas = 0;
    p->tempo_chegada_sistema = tempo_criacao;
    p->tempo_total_cpu_usado = 0;
    p->tempo_restante_bloqueio = 0;
    
    p->quantum_alocado_atual = 0;      
    p->tempo_usado_no_quantum_atual = 0; 
    // p->numProcessos = 0; // Removido para resolver o Erro 3 (a menos que você decida mantê-lo)
    p->thread = 0; 

    return p;
}

void psCarregarProgramaDeArquivo(ProcessoSimulado_t *p, const char* nome_arquivo_programa) {
    if (!p || !nome_arquivo_programa) {
        if (p) p->estado_atual = EST_TERMINADO; 
        return;
    }
    
    FILE *arquivo = fopen(nome_arquivo_programa, "r");
    if (!arquivo) {
        perror("fopen em psCarregarProgramaDeArquivo");
        fprintf(stderr, " -> Arquivo não encontrado: %s\n", nome_arquivo_programa);
        p->estado_atual = EST_TERMINADO;
        psLiberarListaInstrucoes(&p->listaInstrucoes); 
        psInicializarListaInstrucoes(&p->listaInstrucoes); 
        return;
    }

    psLiberarListaInstrucoes(&p->listaInstrucoes);
    psInicializarListaInstrucoes(&p->listaInstrucoes);
    memset(p->memoria, 0, sizeof(p->memoria));
    p->num_variaveis_declaradas = 0;
    p->pc = 0;

    char linha[MAX_NOME_ARQUIVO_R + 30]; 
    Instrucao_t nova_inst;
    int num_instrucoes = 0;

    while (fgets(linha, sizeof(linha), arquivo)) {
        if (linha[0] == '\n' || linha[0] == '\0' || linha[0] == '#') continue;

        memset(&nova_inst, 0, sizeof(Instrucao_t));
        int itens_lidos = sscanf(linha, " %c", &nova_inst.tipoInstrucaoChar);

        if (itens_lidos < 1) continue;

        if (nova_inst.tipoInstrucaoChar == 'R') {
            sscanf(linha, " %*c %255s", nova_inst.nome_arquivo_R); 
            nova_inst.nome_arquivo_R[MAX_NOME_ARQUIVO_R - 1] = '\0'; 
        } else {
            sscanf(linha, " %*c %d %d", &nova_inst.arg1, &nova_inst.arg2);
        }
        psInserirInstrucao(&p->listaInstrucoes, nova_inst);
        num_instrucoes++;
    }
    
    fclose(arquivo);

    if (num_instrucoes == 0) { 
        p->estado_atual = EST_TERMINADO;
    }
}

void psLiberarMemoria(ProcessoSimulado_t *p) {
    if (!p) return;
    psLiberarListaInstrucoes(&p->listaInstrucoes);
    free(p);
}

void psImprimirInstrucoes(const ProcessoSimulado_t *p) {
    if (!p) return;
    printf("------ Instrucoes PID: %d (PC=%d, Prio=%d) Estado: %s (Total: %d) ------\n",
           p->pid, p->pc, p->prioridade, estadoParaString(p->estado_atual), p->listaInstrucoes.tamanho);
    ApontadorInstrucao_t aux = p->listaInstrucoes.primeiro;
    if (aux) aux = aux->prox; 
    int i = 0;
    while (aux != NULL) {
        printf("[%d]%s %c ", i, (i == p->pc ? "->" : "  "), aux->instrucao.tipoInstrucaoChar);
        if (aux->instrucao.tipoInstrucaoChar == 'R') {
            printf("%s\n", aux->instrucao.nome_arquivo_R);
        } else {
            printf("%d %d\n", aux->instrucao.arg1, aux->instrucao.arg2);
        }
        aux = aux->prox;
        i++;
    }
    printf("---------------------------------------------------------------\n");
}

// Função principal da thread do processo
void* psExecutarProcesso(void* arg_ptr) {
    ThreadArgs_t *thread_args = (ThreadArgs_t*) arg_ptr;
    ProcessoSimulado_t* processo = thread_args->processo;
    struct GerenciadorDeProcessos_s *gerenciador = thread_args->gerenciador; 

    free(thread_args); 

    printf("INFO: PID %d (Prio %d) iniciou execução com quantum %d.\n", // Removido SysThreadID
           processo->pid, processo->prioridade, processo->quantum_alocado_atual);

    while (1) { 
        pthread_mutex_lock(&tabela_processos_mutex);

        if (processo->estado_atual != EST_EXECUCAO) {
            printf("AVISO: PID %d em estado %s (esperado EXECUCAO). Saindo da thread.\n", // Removido SysThreadID
                   processo->pid, estadoParaString(processo->estado_atual));
            if(gerenciador->processo_ativo_pid == processo->pid) { 
                gerenciador->processo_ativo_pid = -1;
            }
            pthread_mutex_unlock(&tabela_processos_mutex);
            pthread_exit(NULL);
        }

        int pc_foi_explicitamente_definido_pela_instrucao = 0;

        if (processo->pc >= processo->listaInstrucoes.tamanho) {
            processo->estado_atual = EST_TERMINADO;
            pc_foi_explicitamente_definido_pela_instrucao = 1; 
        } else {
            Instrucao_t *instr_atual_ptr = psObterInstrucaoNoPc(processo);
            if (!instr_atual_ptr) {
                fprintf(stderr, "ERRO FATAL: PID %d não obteve instrução no PC %d. Terminando.\n", // Removido SysThreadID
                        processo->pid, processo->pc);
                processo->estado_atual = EST_TERMINADO;
                pc_foi_explicitamente_definido_pela_instrucao = 1;
            } else {
                Instrucao_t instr = *instr_atual_ptr; 
                int pc_original_para_fork_ou_r = processo->pc;
                
                // MODIFICAÇÃO 4: Linha de execução da instrução
                printf("EXEC: PID %2d (Prio %d, Q %d/%d) PC %2d: %c ",  // Removido SysThr
                       processo->pid, processo->prioridade, 
                       processo->tempo_usado_no_quantum_atual + 1, 
                       processo->quantum_alocado_atual,
                       processo->pc, instr.tipoInstrucaoChar);
                if (instr.tipoInstrucaoChar == 'R') printf("%s\n", instr.nome_arquivo_R);
                else printf("%d %d\n", instr.arg1, instr.arg2);

                 switch (instr.tipoInstrucaoChar) {
                    case 'N': 
                        processo->num_variaveis_declaradas = instr.arg1; 
                        if (processo->num_variaveis_declaradas < 0) processo->num_variaveis_declaradas = 0;
                        if (processo->num_variaveis_declaradas > MAX_MEMORIA_PROCESSO_SIMULADO) processo->num_variaveis_declaradas = MAX_MEMORIA_PROCESSO_SIMULADO;
                        break;
                    case 'D': 
                        if (instr.arg1 >= 0 && instr.arg1 < processo->num_variaveis_declaradas) processo->memoria[instr.arg1] = 0; 
                        else { fprintf(stderr, "ERRO: PID %d, D: Var %d inválida (declaradas: %d). Terminando.\n", processo->pid, instr.arg1, processo->num_variaveis_declaradas); processo->estado_atual = EST_TERMINADO; pc_foi_explicitamente_definido_pela_instrucao=1;}
                        break;
                     case 'V': 
                        if (instr.arg1 >= 0 && instr.arg1 < processo->num_variaveis_declaradas) processo->memoria[instr.arg1] = instr.arg2;
                        else { fprintf(stderr, "ERRO: PID %d, V: Var %d inválida (declaradas: %d). Terminando.\n", processo->pid, instr.arg1, processo->num_variaveis_declaradas); processo->estado_atual = EST_TERMINADO; pc_foi_explicitamente_definido_pela_instrucao=1;}
                        break;
                    case 'A': 
                        if (instr.arg1 >= 0 && instr.arg1 < processo->num_variaveis_declaradas) processo->memoria[instr.arg1] += instr.arg2;
                        else { fprintf(stderr, "ERRO: PID %d, A: Var %d inválida (declaradas: %d). Terminando.\n", processo->pid, instr.arg1, processo->num_variaveis_declaradas); processo->estado_atual = EST_TERMINADO; pc_foi_explicitamente_definido_pela_instrucao=1;}
                        break;
                    case 'S': 
                        if (instr.arg1 >= 0 && instr.arg1 < processo->num_variaveis_declaradas) processo->memoria[instr.arg1] -= instr.arg2;
                        else { fprintf(stderr, "ERRO: PID %d, S: Var %d inválida (declaradas: %d). Terminando.\n", processo->pid, instr.arg1, processo->num_variaveis_declaradas); processo->estado_atual = EST_TERMINADO; pc_foi_explicitamente_definido_pela_instrucao=1;}
                        break;
                    case 'B': 
                        processo->estado_atual = EST_BLOQUEADO;
                        processo->tempo_restante_bloqueio = (instr.arg1 > 0) ? instr.arg1 : 1;
                        if (processo->tempo_usado_no_quantum_atual < processo->quantum_alocado_atual) {
                            if (processo->prioridade > 0) {
                                processo->prioridade--; 
                                printf("INFO: PID %d (Prio %d -> %d) bloqueado antes do fim do quantum, prioridade aumentada.\n", processo->pid, processo->prioridade +1, processo->prioridade);
                            }
                        }
                        break;
                    case 'T': 
                        processo->estado_atual = EST_TERMINADO; 
                        pc_foi_explicitamente_definido_pela_instrucao = 1; 
                        break;
                    case 'F': 
                        {
                            ProcessoSimulado_t *filho_info_temp = psCriarNovo(processo->pid, processo->prioridade, gerenciador->tempo_simulacao_global); 
                            if (!filho_info_temp) {
                                fprintf(stderr, "ERRO CRÍTICO: PID %d, F: Falha ao alocar info do filho. Terminando pai.\n", processo->pid);
                                processo->estado_atual = EST_TERMINADO; pc_foi_explicitamente_definido_pela_instrucao=1; break;
                            }
                            filho_info_temp->pc = pc_original_para_fork_ou_r + 1; 
                            psCopiarListaInstrucoes(&filho_info_temp->listaInstrucoes, &processo->listaInstrucoes); 
                            memcpy(filho_info_temp->memoria, processo->memoria, sizeof(processo->memoria)); 
                            filho_info_temp->num_variaveis_declaradas = processo->num_variaveis_declaradas; 

                            pthread_mutex_unlock(&tabela_processos_mutex);
                            int pid_filho = configurarNovoProcessoNaTabela(gerenciador, filho_info_temp, processo->pid, filho_info_temp->prioridade, gerenciador->tempo_simulacao_global);
                            pthread_mutex_lock(&tabela_processos_mutex); 

                            if (pid_filho != -1) { 
                                free(filho_info_temp); 
                                
                                pthread_mutex_unlock(&tabela_processos_mutex);
                                estadosAdicionarPronto(&gerenciador->processos_prontos, pid_filho, gerenciador->tabela_de_processos[pid_filho].prioridade);
                                pthread_mutex_lock(&tabela_processos_mutex); 
                                printf("INFO: PID %d (Prio %d) criou filho PID %d (Prio %d).\n", processo->pid, processo->prioridade, pid_filho, gerenciador->tabela_de_processos[pid_filho].prioridade);
                            } else {
                                fprintf(stderr, "ERRO: PID %d, F: Falha ao configurar filho na tabela.\n", processo->pid);
                                psLiberarMemoria(filho_info_temp); 
                            }
                            processo->pc = (pc_original_para_fork_ou_r + 1) + instr.arg1; 
                            pc_foi_explicitamente_definido_pela_instrucao = 1;
                        }
                        break;
                    case 'R': 
                        {
                            char nome_arq_prog[MAX_NOME_ARQUIVO_R];
                            strncpy(nome_arq_prog, instr.nome_arquivo_R, MAX_NOME_ARQUIVO_R -1);
                            nome_arq_prog[MAX_NOME_ARQUIVO_R-1] = '\0';
                            
                            printf("INFO: PID %d (Prio %d) substituindo programa por: %s\n", processo->pid, processo->prioridade, nome_arq_prog);
                            psCarregarProgramaDeArquivo(processo, nome_arq_prog); 
                            
                            if (processo->estado_atual == EST_TERMINADO || processo->listaInstrucoes.tamanho == 0) {
                                fprintf(stderr, "ERRO: PID %d, R: Falha ao carregar '%s' ou arquivo vazio. Processo será terminado.\n", processo->pid, nome_arq_prog);
                            } else {
                                processo->pc = 0; 
                            }
                            pc_foi_explicitamente_definido_pela_instrucao = 1;
                        }
                        break;
                    default: // MODIFICAÇÃO 5: Mensagem de instrução desconhecida
                        fprintf(stderr, "ERRO: PID %d (Prio %d) instrução desconhecida '%c' no PC %d. Terminando.\n", // Removido SysThreadID
                                processo->pid, processo->prioridade, instr.tipoInstrucaoChar, processo->pc);
                        processo->estado_atual = EST_TERMINADO;
                        pc_foi_explicitamente_definido_pela_instrucao = 1;
                        break;
                } 
            } 
        } 
        
        if (processo->estado_atual == EST_EXECUCAO) { 
            processo->tempo_usado_no_quantum_atual++;
        }
        processo->tempo_total_cpu_usado++;

        int deve_sair_da_thread = 0;
        char* motivo_saida_str = "";

        if (processo->estado_atual == EST_TERMINADO) {
            motivo_saida_str = "TERMINADO"; deve_sair_da_thread = 1;
        } else if (processo->estado_atual == EST_BLOQUEADO) {
            motivo_saida_str = "BLOQUEADO"; deve_sair_da_thread = 1;
        } else if (processo->tempo_usado_no_quantum_atual >= processo->quantum_alocado_atual) {
            // MODIFICAÇÃO 6: Mensagem de quantum expirado
            printf("INFO: PID %d (Prio %d) usou todo o quantum (%d/%d).\n", // Removido SysThreadID
                   processo->pid, processo->prioridade,
                   processo->tempo_usado_no_quantum_atual, processo->quantum_alocado_atual);
            if (processo->prioridade < NUM_NIVEIS_PRIORIDADE - 1) {
                processo->prioridade++;
            }
            processo->estado_atual = EST_PRONTO;
            motivo_saida_str = "QUANTUM EXPIRADO"; deve_sair_da_thread = 1;
        }

        if (!pc_foi_explicitamente_definido_pela_instrucao && processo->estado_atual != EST_TERMINADO) {
            processo->pc++; 
        }
        if (!pc_foi_explicitamente_definido_pela_instrucao && (processo->estado_atual == EST_EXECUCAO || processo->estado_atual == EST_BLOQUEADO)) {
            processo->pc++;
        }
        if (deve_sair_da_thread) {
            processo->tempo_usado_no_quantum_atual = 0; 
            
            EstadoProcesso_e estado_ao_sair = processo->estado_atual;
            int pid_ao_sair = processo->pid;
            int prio_ao_sair = processo->prioridade;

            if(gerenciador->processo_ativo_pid == processo->pid) { 
                gerenciador->processo_ativo_pid = -1; 
            }
            pthread_mutex_unlock(&tabela_processos_mutex); 

            if (estado_ao_sair == EST_PRONTO) {
                 estadosAdicionarPronto(&gerenciador->processos_prontos, pid_ao_sair, prio_ao_sair);
            } else if (estado_ao_sair == EST_BLOQUEADO) { 
                 estadosAdicionarBloqueado(&gerenciador->processos_bloqueados, pid_ao_sair);
            }
            
            // MODIFICAÇÃO 7: Mensagem de saída da thread
            printf("INFO: PID %d %s (Nova Prio %d). Saindo da thread.\n", // Removido SysThreadID
                   pid_ao_sair, motivo_saida_str, prio_ao_sair);
            pthread_exit(NULL);
        }
        
        pthread_mutex_unlock(&tabela_processos_mutex); 
        // usleep(10000); // Opcional para debug ou simular trabalho real
    }

    return NULL; 
}