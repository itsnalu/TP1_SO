#include "../include/processoSimulado.h"
#include <unistd.h> // Para getcwd em debug, pode ser removido se não usado

// --- Implementação Funções de Lista de Instruções ---
void psInicializarListaInstrucoes(ListaInstrucoes_t *lista) {
    lista->primeiro = (ApontadorInstrucao_t)malloc(sizeof(CelulaInstrucao_t));
    if (!lista->primeiro) { perror("malloc lista.primeiro"); exit(EXIT_FAILURE); }
    lista->ultimo = lista->primeiro;
    lista->primeiro->prox = NULL;
    lista->tamanho = 0;
}
void psLiberarListaInstrucoes(ListaInstrucoes_t *lista) {
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
    lista->ultimo->prox = (ApontadorInstrucao_t)malloc(sizeof(CelulaInstrucao_t));
    if (!lista->ultimo->prox) { perror("malloc lista.ultimo->prox"); exit(EXIT_FAILURE); }
    lista->ultimo = lista->ultimo->prox;
    lista->ultimo->instrucao = inst;
    lista->ultimo->prox = NULL;
    lista->tamanho++;
}
void psCopiarListaInstrucoes(ListaInstrucoes_t *destino, const ListaInstrucoes_t *origem) {
    if (!origem || !destino) {
        printf("[DEBUG] Erro: origem ou destino nulo na cópia de lista\n");
        return;
    }
    
    printf("[DEBUG] Iniciando cópia de lista de instruções (tamanho origem: %d)\n", origem->tamanho);
    
    // Limpa a lista de destino
    psLiberarListaInstrucoes(destino);
    psInicializarListaInstrucoes(destino);
    
    // Verifica se a lista de origem está vazia
    if (!origem->primeiro || !origem->primeiro->prox) {
        printf("[DEBUG] Lista de origem vazia ou inválida\n");
        return;
    }
    
    // Copia cada instrução
    ApontadorInstrucao_t atual_origem = origem->primeiro->prox; // Pula célula cabeça
    int contador = 0;
    
    while (atual_origem != NULL) {
        psInserirInstrucao(destino, atual_origem->instrucao);
        printf("[DEBUG] Instrução %d copiada: %c %d %d\n", 
               contador,
               atual_origem->instrucao.tipoInstrucaoChar,
               atual_origem->instrucao.arg1,
               atual_origem->instrucao.arg2);
        atual_origem = atual_origem->prox;
        contador++;
    }
    
    printf("[DEBUG] Lista copiada com sucesso (tamanho destino: %d, instruções copiadas: %d)\n", 
           destino->tamanho, contador);
}
// Função auxiliar interna para obter um ponteiro para a instrução no PC atual.
static Instrucao_t* psObterInstrucaoNoPc(const ProcessoSimulado_t *p) {
    if (!p) {
        printf("[DEBUG] Erro: Processo nulo ao obter instrução no PC\n");
        return NULL;
    }
    
    if (p->pc < 0 || p->pc >= p->listaInstrucoes.tamanho) {
        printf("[DEBUG] PC fora dos limites (PC: %d, Tamanho lista: %d)\n", 
               p->pc, p->listaInstrucoes.tamanho);
        return NULL;
    }
    
    printf("[DEBUG] Tentando obter instrução no PC %d (tamanho lista: %d)\n", 
           p->pc, p->listaInstrucoes.tamanho);
    
    ApontadorInstrucao_t atual = p->listaInstrucoes.primeiro->prox;
    if (!atual) {
        printf("[DEBUG] Erro: Lista de instruções vazia\n");
        return NULL;
    }
    
    for (int i = 0; i < p->pc && atual != NULL; ++i) {
        atual = atual->prox;
    }
    
    if (!atual) {
        printf("[DEBUG] Erro: Não foi possível encontrar instrução no PC %d\n", p->pc);
        return NULL;
    }
    
    printf("[DEBUG] Instrução encontrada no PC %d: %c %d %d\n", 
           p->pc, atual->instrucao.tipoInstrucaoChar, 
           atual->instrucao.arg1, atual->instrucao.arg2);
    
    return &(atual->instrucao);
}

// --- Implementação Funções Principais do Processo Simulado ---
ProcessoSimulado_t* psCriarNovo(int pid_sugerido, int pid_pai, int prioridade_inicial, long tempo_criacao) {
    ProcessoSimulado_t *p = (ProcessoSimulado_t*)malloc(sizeof(ProcessoSimulado_t));
    if (!p) { perror("malloc ProcessoSimulado_t"); exit(EXIT_FAILURE); }

    p->pid = pid_sugerido; // O Gerenciador deve validar/atribuir o PID final
    p->pid_pai = pid_pai;
    p->estado_atual = EST_PRONTO; // Processos iniciam no estado PRONTO
    p->prioridade = prioridade_inicial;
    p->pc = 0;
    psInicializarListaInstrucoes(&p->listaInstrucoes);
    memset(p->memoria, 0, sizeof(p->memoria)); // Memória inicia zerada
    p->num_variaveis_declaradas = 0;
    p->tempo_chegada_sistema = tempo_criacao;
    p->tempo_total_cpu_usado = 0;
    p->tempo_restante_bloqueio = 0;
    p->tempo_usado_no_quantum_atual = 0;

    pthread_mutex_init(&p->proc_mutex, NULL);
    pthread_cond_init(&p->proc_cond, NULL);
    p->is_scheduled_to_run = 0;
    p->thread_id = 0;
    return p;
}

void psCarregarProgramaDeArquivo(ProcessoSimulado_t *p, const char* nome_arquivo_programa) {
    if (!p || !nome_arquivo_programa) {
        printf("[DEBUG] Erro: Processo ou nome do arquivo nulo\n");
        return;
    }
    
    printf("[DEBUG] Carregando programa do arquivo: %s para PID %d\n", nome_arquivo_programa, p->pid);
    
    FILE *arquivo = fopen(nome_arquivo_programa, "r");
    if (!arquivo) {
        printf("[DEBUG] Falha ao abrir arquivo %s para PID %d\n", nome_arquivo_programa, p->pid);
        p->estado_atual = EST_TERMINADO;
        if(p->listaInstrucoes.tamanho > 0) psLiberarListaInstrucoes(&p->listaInstrucoes);
        psInicializarListaInstrucoes(&p->listaInstrucoes);
        return;
    }

    // Limpa estado anterior para carga de novo programa (importante para instrução 'R')
    psLiberarListaInstrucoes(&p->listaInstrucoes);
    psInicializarListaInstrucoes(&p->listaInstrucoes);
    memset(p->memoria, 0, sizeof(p->memoria));
    p->num_variaveis_declaradas = 0;
    p->pc = 0;

    char linha[MAX_NOME_ARQUIVO_R + 30]; // Buffer para ler linha do arquivo
    Instrucao_t nova_inst;
    int num_instrucoes = 0;

    while (fgets(linha, sizeof(linha), arquivo)) {
        if (linha[0] == '\n' || linha[0] == '\0' || linha[0] == '#') continue;

        memset(&nova_inst, 0, sizeof(Instrucao_t));
        int itens_lidos = sscanf(linha, " %c", &nova_inst.tipoInstrucaoChar);

        if (itens_lidos < 1) continue;

        if (nova_inst.tipoInstrucaoChar == 'R') {
            sscanf(linha, " %*c %s", nova_inst.nome_arquivo_R);
            printf("[DEBUG] Instrução R carregada: %s\n", nova_inst.nome_arquivo_R);
        } else {
            sscanf(linha, " %*c %d %d", &nova_inst.arg1, &nova_inst.arg2);
            printf("[DEBUG] Instrução %c carregada: arg1=%d, arg2=%d\n", 
                   nova_inst.tipoInstrucaoChar, nova_inst.arg1, nova_inst.arg2);
        }
        psInserirInstrucao(&p->listaInstrucoes, nova_inst);
        num_instrucoes++;
    }
    
    printf("[DEBUG] Total de instruções carregadas: %d\n", num_instrucoes);
    fclose(arquivo);
}

void psLiberarMemoria(ProcessoSimulado_t *p) {
    if (!p) return;
    psLiberarListaInstrucoes(&p->listaInstrucoes);

    pthread_mutex_destroy(&p->proc_mutex);
    pthread_cond_destroy(&p->proc_cond);
    free(p);
}

void psExecutarProximaInstrucao(ProcessoSimulado_t *p, long tempo_global_simulador, ProcessoSimulado_t **novo_processo_filho_ptr) {
    printf("[DEBUG] Iniciando execução de instrução para PID %d\n", p->pid);
    printf("[DEBUG] Estado atual: %d\n", p->estado_atual);
    printf("[DEBUG] PC atual: %d\n", p->pc);
    printf("[DEBUG] Total de instruções: %d\n", p->listaInstrucoes.tamanho);
    *novo_processo_filho_ptr = NULL;

    // Checagens iniciais de validade
    if (!p || novo_processo_filho_ptr == NULL) {
        printf("[DEBUG] Erro: Processo nulo ou ponteiro inválido\n");
        if (p) p->estado_atual = EST_TERMINADO;
        return;
    }
    *novo_processo_filho_ptr = NULL;

        if (p->estado_atual != EST_EXECUCAO) {
        printf("[PID %d] Erro: Tentativa de executar instrução sem estar em EST_EXECUCAO. Estado: %d\n", p->pid, p->estado_atual);
        return;
    }

    if (p->pc < 0 || p->pc >= p->listaInstrucoes.tamanho) {
        printf("[PID %d] ERRO: PC (%d) fora dos limites! Terminando processo.\n", p->pid, p->pc);
        p->estado_atual = EST_TERMINADO; // PC inválido termina o processo
        return;
    }

    Instrucao_t *instr_atual_ptr = psObterInstrucaoNoPc(p); // Sua função para obter a instrução
    if (!instr_atual_ptr) {
        printf("[PID %d] ERRO: Falha ao obter instrução no PC (%d). Terminando processo.\n", p->pid, p->pc);
        p->estado_atual = EST_TERMINADO;
        return;
    }

    Instrucao_t instr = *instr_atual_ptr;
    int proximo_pc_candidato = p->pc + 1; // PC padrão para a próxima instrução
    printf("[DEBUG] Executando instrução: %c %d %d\n", 
           instr.tipoInstrucaoChar, instr.arg1, instr.arg2);
    
    switch (instr.tipoInstrucaoChar) {
        case 'N': // Define o número de variáveis utilizáveis
            p->num_variaveis_declaradas = instr.arg1;
            printf("[PID %d] Declaradas %d variáveis\n", p->pid, instr.arg1);
            if (p->num_variaveis_declaradas < 0) p->num_variaveis_declaradas = 0;
            if (p->num_variaveis_declaradas > MAX_MEMORIA_PROCESSO_SIMULADO) {
                p->num_variaveis_declaradas = MAX_MEMORIA_PROCESSO_SIMULADO;
            }
            break;
            
        case 'D': // Declara uma variável
            if (instr.arg1 >= 0 && instr.arg1 < p->num_variaveis_declaradas) {
                p->memoria[instr.arg1] = 0;
                printf("[PID %d] Variável %d declarada com valor 0\n", p->pid, instr.arg1);
            } else { 
                printf("[PID %d] Erro: Tentativa de declarar variável inválida %d\n", p->pid, instr.arg1);
                p->estado_atual = EST_TERMINADO;
                return;
            }
            break;
            
        case 'V': // Atribui valor a uma variável
            if (instr.arg1 >= 0 && instr.arg1 < p->num_variaveis_declaradas) {
                p->memoria[instr.arg1] = instr.arg2;
                printf("[PID %d] Variável %d atribuída com valor %d\n", p->pid, instr.arg1, instr.arg2);
            } else { 
                printf("[PID %d] Erro: Tentativa de atribuir valor a variável inválida %d\n", p->pid, instr.arg1);
                p->estado_atual = EST_TERMINADO;
                return;
            }
            break;
            
        case 'A': // Adiciona valor a uma variável
            if (instr.arg1 >= 0 && instr.arg1 < p->num_variaveis_declaradas) {
                p->memoria[instr.arg1] += instr.arg2;
                printf("[PID %d] Adicionado %d à variável %d. Novo valor: %d\n", 
                       p->pid, instr.arg2, instr.arg1, p->memoria[instr.arg1]);
            } else { 
                printf("[PID %d] Erro: Tentativa de adicionar valor a variável inválida %d\n", p->pid, instr.arg1);
                p->estado_atual = EST_TERMINADO;
                return;
            }
            break;
            
        case 'S': // Subtrai valor de uma variável
            if (instr.arg1 >= 0 && instr.arg1 < p->num_variaveis_declaradas) {
                p->memoria[instr.arg1] -= instr.arg2;
                printf("[PID %d] Subtraído %d da variável %d. Novo valor: %d\n", 
                       p->pid, instr.arg2, instr.arg1, p->memoria[instr.arg1]);
            } else { 
                printf("[PID %d] Erro: Tentativa de subtrair valor de variável inválida %d\n", p->pid, instr.arg1);
                p->estado_atual = EST_TERMINADO;
                return;
            }
            break;
            
        case 'B': // Bloqueia o processo
            p->estado_atual = EST_BLOQUEADO;
            p->tempo_restante_bloqueio = (instr.arg1 > 0) ? instr.arg1 : 1;
            printf("[PID %d] Processo bloqueado por %d unidades de tempo (PC irá para %d).\n", p->pid, p->tempo_restante_bloqueio, proximo_pc_candidato);
            p->pc = proximo_pc_candidato; // Salva o próximo PC ANTES de retornar
            return; 
            
        case 'T': // Termina o processo
            p->estado_atual = EST_TERMINADO;
            printf("[PID %d] Processo terminado (PC seria %d).\n", p->pid, proximo_pc_candidato);
            p->pc = proximo_pc_candidato; // Salva o próximo PC (conceitual) ANTES de retornar
            return;
            
        case 'F': // Cria um processo filho (fork)
{
    // Cria uma estrutura temporária para passar informações do filho para o gerenciador
                ProcessoSimulado_t *info_filho = psCriarNovo(-1, p->pid, p->prioridade, tempo_global_simulador);
                if (!info_filho) {
                    printf("[PID %d] ERRO: Falha ao alocar memória para processo filho.\n", p->pid);
                    p->pc = proximo_pc_candidato; // Pai continua na próxima instrução
                    return;
                }
                // Copia estado do pai para o filho
                psCopiarListaInstrucoes(&info_filho->listaInstrucoes, &p->listaInstrucoes);
                memcpy(info_filho->memoria, p->memoria, sizeof(p->memoria));
                info_filho->num_variaveis_declaradas = p->num_variaveis_declaradas;
                info_filho->estado_atual = EST_PRONTO; // Filho começa pronto

                info_filho->pc = proximo_pc_candidato; // Filho começa na instrução seguinte à 'F'
                p->pc = proximo_pc_candidato + instr.arg1; // Pai avança PC conforme argumento de 'F'

                *novo_processo_filho_ptr = info_filho;
                printf("[Pai PID %d] Criou filho. Pai PC -> %d, Filho PC -> %d.\n", p->pid, p->pc, info_filho->pc);
                return; 
}
        case 'R': // Substitui o programa do processo atual
            {
                                printf("[PID %d] Substituindo programa por %s.\n", p->pid, instr.nome_arquivo_R);
                char nome_arq_temp[MAX_NOME_ARQUIVO_R]; // Salva nome antes de psCarregar
                strncpy(nome_arq_temp, instr.nome_arquivo_R, MAX_NOME_ARQUIVO_R - 1);
                nome_arq_temp[MAX_NOME_ARQUIVO_R - 1] = '\0';

                psCarregarProgramaDeArquivo(p, nome_arq_temp); // Reseta PC para 0, memória, etc.
                                                          // Estado e prioridade não mudam
                // Se psCarregarProgramaDeArquivo falhar, ele pode setar estado para TERMINADO
                printf("[PID %d] Programa substituído. Novo PC: %d.\n",p->pid, p->pc);
                return; // PC e programa alterados, retorna
            }
            break;
            
        default: // Instrução desconhecida
           printf("[PID %d] Instrução desconhecida '%c'. Terminando.\n", p->pid, instr.tipoInstrucaoChar);
            p->estado_atual = EST_TERMINADO;
            p->pc = proximo_pc_candidato; // Salva o próximo PC ANTES de retornar
            return;
    }

    if (p->estado_atual == EST_EXECUCAO) { // Garante que o processo ainda está "executando"
        p->pc = proximo_pc_candidato;
    }
}

void psImprimirInstrucoes(const ProcessoSimulado_t *p) { // Para debug
    if (!p) return;
    printf("------ Instrucoes PID: %d (PC=%d) Estado: %d (Total: %d) ------\n",
           p->pid, p->pc, p->estado_atual, p->listaInstrucoes.tamanho);
    ApontadorInstrucao_t aux = p->listaInstrucoes.primeiro->prox;
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

// Essa função simula um interpretador de instruções.
void* psExecutarProcesso(void* arg){
    ProcessoSimulado_t* processo = (ProcessoSimulado_t*) arg;
    
    printf("[PID %d] Iniciando execução do processo.\n", processo->pid);
    
    processo->estado_atual = EST_EXECUCAO;

    ApontadorInstrucao_t atual = processo->listaInstrucoes.primeiro;
    int pc = 0;

    while(atual != NULL){
        Instrucao_t instrucao = atual->instrucao;

        printf("[PID %d] Executando instrução %c (arg1: %d, arg2: %d)\n",
               processo->pid,
               instrucao.tipoInstrucaoChar,
               instrucao.arg1,
               instrucao.arg2);

        switch(instrucao.tipoInstrucaoChar){
            case 'N':
                // Declarar variável (reserva espaço na memória)
                if(processo->num_variaveis_declaradas < MAX_MEMORIA_PROCESSO_SIMULADO){
                    processo->memoria[processo->num_variaveis_declaradas++] = 0;
                }
                break;

            case 'D':
                // Atribuir valor direto
                if(instrucao.arg1 < processo->num_variaveis_declaradas){
                    processo->memoria[instrucao.arg1] = instrucao.arg2;
                }
                break;

            case 'V':
                // Visualiza valor 
                if(instrucao.arg1 < processo->num_variaveis_declaradas){
                    printf("[PID %d] VAR[%d] = %d\n", processo->pid, instrucao.arg1, processo->memoria[instrucao.arg1]);
                }
                break;

            case 'B':
                // Bloqueia o processo
                processo->estado_atual = EST_BLOQUEADO;
                processo->tempo_restante_bloqueio = instrucao.arg1;
                printf("[PID %d] Processo bloqueado por %d unidades de tempo.\n",
                       processo->pid, instrucao.arg1);
                pthread_exit(NULL); // Finaliza execução por enquanto
                break;

            case 'T':
                // Termina o processo
                processo->estado_atual = EST_TERMINADO;
                printf("[PID %d] Processo terminou.\n", processo->pid);
                pthread_exit(NULL);
                break;

            default:
                printf("[PID %d] Instrução desconhecida: %c\n", processo->pid, instrucao.tipoInstrucaoChar);
        }

        atual = atual->prox;
        pc++;
        processo->pc = pc;

        // Simula tempo de CPU gasto por instrução
        sleep(1);
        processo->tempo_total_cpu_usado += 1;
    }

    processo->estado_atual = EST_TERMINADO;
    printf("[PID %d] Fim da execução do processo.\n", processo->pid);

    pthread_exit(NULL);
}
