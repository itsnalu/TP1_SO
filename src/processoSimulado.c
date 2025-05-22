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
    
    printf("[DEBUG] Copiando lista de instruções (tamanho origem: %d)\n", origem->tamanho);
    
    // Limpa a lista de destino
    psLiberarListaInstrucoes(destino);
    psInicializarListaInstrucoes(destino);
    
    // Copia cada instrução
    ApontadorInstrucao_t atual_origem = origem->primeiro->prox; // Pula célula cabeça
    while (atual_origem != NULL) {
        psInserirInstrucao(destino, atual_origem->instrucao);
        printf("[DEBUG] Instrução copiada: %c %d %d\n", 
               atual_origem->instrucao.tipoInstrucaoChar,
               atual_origem->instrucao.arg1,
               atual_origem->instrucao.arg2);
        atual_origem = atual_origem->prox;
    }
    
    printf("[DEBUG] Lista copiada com sucesso (tamanho destino: %d)\n", destino->tamanho);
}
// Função auxiliar interna para obter um ponteiro para a instrução no PC atual.
static Instrucao_t* psObterInstrucaoNoPc(const ProcessoSimulado_t *p) {
    if (!p || p->pc < 0 || p->pc >= p->listaInstrucoes.tamanho) return NULL;
    ApontadorInstrucao_t atual = p->listaInstrucoes.primeiro->prox;
    for (int i = 0; i < p->pc && atual != NULL; ++i) {
        atual = atual->prox;
    }
    return (atual ? &(atual->instrucao) : NULL);
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
            printf("[PID %d] Processo bloqueado por %d unidades de tempo\n", p->pid, p->tempo_restante_bloqueio);
            return;
            
        case 'T': // Termina o processo
            p->estado_atual = EST_TERMINADO;
            printf("[PID %d] Processo terminado\n", p->pid);
            return;
            
        case 'F': // Cria um processo filho (fork)
            /* {
                ProcessoSimulado_t *filho = psCriarNovo(
                    -1, // PID será definido pelo Gerenciador
                    p->pid,
                    p->prioridade, // Filho herda prioridade
                    tempo_global_simulador
                );
                
                // Copia o programa e a memória do pai para o filho
                psCopiarListaInstrucoes(&filho->listaInstrucoes, &p->listaInstrucoes);
                memcpy(filho->memoria, p->memoria, sizeof(p->memoria));
                filho->num_variaveis_declaradas = p->num_variaveis_declaradas;

                // Define PCs conforme especificação
                filho->pc = p->pc + 1;                  // Filho começa na instrução seguinte a 'F'
                p->pc = (p->pc + 1) + instr.arg1;     // Pai avança (PC_atual + 1) + n instruções

                filho->tempo_total_cpu_usado = 0; // Filho inicia com tempo de CPU zerado

                *novo_processo_filho_ptr = filho; // Retorna o filho para o Gerenciador
                pc_foi_alterado_por_salto = 1;    // PC do pai foi modificado diretamente
                printf("[PID %d] Criado processo filho (PC pai=%d, PC filho=%d)\n", 
                       p->pid, p->pc, filho->pc);
            } */
           // FAVOR NAO MEXER NESTE FORK, A FUNÇÃO F PRECISA DE FORK.
           // SE FOR MEXER, APENAS MODIFIQUE PARA FAZER FUNCIONAR, CASO NÃO FUNCIONE, PELO AMOR DE DEUS.
            {
                pid_t pid_filho = fork();
                if (pid_filho == -1) {
                    perror("Erro ao criar processo filho");
                    p->estado_atual = EST_TERMINADO; // Falha na criação do processo
                    return;
                }
                if (pid_filho == 0) { // Processo filho
                    p->pid_pai = p->pid; // Define o PID do pai
                    p->pid = -1; // Define o PID do filho (o gerenciador irá atribuir) // Não sei se isso ta definindo o PID do filho, ou do pai, mas usando fork isso pode estar dando paia. Bom verificar depois.
                    p->pc++; // O filho começa na instrução seguinte a 'F'
                    printf("[Filho] PID: %d, Pai: %d \n", p->pid, p->pid_pai);
                } else { // Processo pai
                    p->pc = (p->pc + 1) + instr.arg1; // O pai avança (PC_atual + 1) + n instruções
                    printf("[Pai] PID: %d, criou filho PID: %d \n", p->pid, pid_filho);
                    pc_foi_alterado_por_salto = 1; // PC do pai foi modificado diretamente
                    return; // Pai retorna e segue execução
                }
                break;    
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
    if (!pc_foi_alterado_por_salto && p->estado_atual == EST_EXECUCAO) {
        p->pc++;
        printf("[DEBUG] PC incrementado para %d\n", p->pc);
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