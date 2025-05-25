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
    free(p);
}

void psExecutarProximaInstrucao(ProcessoSimulado_t *p, long tempo_global_simulador, ProcessoSimulado_t **novo_processo_filho_ptr) {
    printf("[DEBUG] Iniciando execução de instrução para PID %d\n", p->pid);
    printf("[DEBUG] Estado atual: %d\n", p->estado_atual);
    printf("[DEBUG] PC atual: %d\n", p->pc);
    printf("[DEBUG] Total de instruções: %d\n", p->listaInstrucoes.tamanho);
    
    // Checagens iniciais de validade
    if (!p || novo_processo_filho_ptr == NULL) {
        printf("[DEBUG] Erro: Processo nulo ou ponteiro inválido\n");
        if (p) p->estado_atual = EST_TERMINADO;
        return;
    }
    *novo_processo_filho_ptr = NULL;

    // Processo só executa se estiver no estado de execução
    if (p->estado_atual != EST_EXECUCAO) {
        printf("[DEBUG] Processo não está em execução (estado: %d)\n", p->estado_atual);
        return;
    }

    // Verifica se o PC está dentro dos limites do programa
    if (p->pc < 0 || p->pc >= p->listaInstrucoes.tamanho) {
        printf("[DEBUG] PC fora dos limites (PC: %d, Tamanho: %d)\n", p->pc, p->listaInstrucoes.tamanho);
        p->estado_atual = EST_TERMINADO;
        return;
    }

    Instrucao_t *instr_atual_ptr = psObterInstrucaoNoPc(p);
    if (!instr_atual_ptr) {
        printf("[DEBUG] Erro ao obter instrução no PC %d\n", p->pc);
        p->estado_atual = EST_TERMINADO;
        return;
    }

    Instrucao_t instr = *instr_atual_ptr;
    printf("[DEBUG] Executando instrução: %c %d %d\n", 
           instr.tipoInstrucaoChar, instr.arg1, instr.arg2);
    
    int pc_foi_alterado_por_salto = 0;
    
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
            // Aumenta a prioridade se o processo foi bloqueado antes de consumir seu quantum
            if (p->tempo_usado_no_quantum_atual < instr.arg1) {
                if (p->prioridade > 0) {
                    p->prioridade--;
                    printf("[PID %d] Prioridade aumentada para %d por bloqueio antecipado\n", 
                           p->pid, p->prioridade);
                }
            }
            printf("[PID %d] Processo bloqueado por %d unidades de tempo\n", p->pid, p->tempo_restante_bloqueio);
            break;
            
        case 'T': // Termina o processo
            p->estado_atual = EST_TERMINADO;
            printf("[PID %d] Processo terminado\n", p->pid);
            break;
            
        case 'F': // Cria um processo filho (fork)
        {
            // Cria uma estrutura temporária para passar informações do filho para o gerenciador
            ProcessoSimulado_t *info_filho = psCriarNovo(-1, p->pid, p->prioridade, tempo_global_simulador);
            
            // Simula a criação do filho
            info_filho->pc = p->pc + 1; // Filho começa na próxima instrução
            
            // Copia o programa e a memória do pai para as informações do filho
            psCopiarListaInstrucoes(&info_filho->listaInstrucoes, &p->listaInstrucoes);
            memcpy(info_filho->memoria, p->memoria, sizeof(p->memoria));
            info_filho->num_variaveis_declaradas = p->num_variaveis_declaradas;
            
            // Avança o PC do pai conforme o argumento da instrução F
            p->pc = (p->pc + 1) + instr.arg1;
            pc_foi_alterado_por_salto = 1;
            
            // Retorna as informações do filho para o gerenciador
            *novo_processo_filho_ptr = info_filho;
            
            printf("[Pai] PID simulado: %d, criou filho simulado (PC inicial: %d)\n", 
                   p->pid, info_filho->pc);
            return;
        }
        case 'R': // Substitui o programa do processo atual
        {
            printf("[PID %d] Substituindo programa por %s\n", p->pid, instr.nome_arquivo_R);
            psCarregarProgramaDeArquivo(p, instr.nome_arquivo_R);
            return; // PC foi resetado para 0 (ou processo terminou)
        }
        break;
            
        default: // Instrução desconhecida
            printf("[PID %d] Instrução desconhecida '%c'\n", p->pid, instr.tipoInstrucaoChar);
            p->estado_atual = EST_TERMINADO;
            return;
    }

    // Avança o PC se não foi uma instrução de salto e o processo continua em execução
    if (!pc_foi_alterado_por_salto && (p->estado_atual == EST_EXECUCAO || p->estado_atual == EST_BLOQUEADO)) {
        p->pc++;
        printf("[DEBUG] PC incrementado para %d (tamanho lista: %d)\n", 
               p->pc, p->listaInstrucoes.tamanho);
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
                break;

            case 'T':
                // Termina o processo
                processo->estado_atual = EST_TERMINADO;
                printf("[PID %d] Processo terminou.\n", processo->pid);
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

}
