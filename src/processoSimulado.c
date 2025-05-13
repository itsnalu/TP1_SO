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
    if (!origem || !destino) return;
    psInicializarListaInstrucoes(destino); // Limpa e prepara a lista de destino
    ApontadorInstrucao_t atual_origem = origem->primeiro->prox; // Pula célula cabeça
    while (atual_origem != NULL) {
        psInserirInstrucao(destino, atual_origem->instrucao);
        atual_origem = atual_origem->prox;
    }
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
    if (!p || !nome_arquivo_programa) return;
    FILE *arquivo = fopen(nome_arquivo_programa, "r");
    if (!arquivo) {
        // Falha ao abrir arquivo, processo não pode carregar programa.
        printf("Falha ao abrir %s para PID %d\n", nome_arquivo_programa, p->pid);
        p->estado_atual = EST_TERMINADO; // Sinaliza falha crítica no carregamento
        if(p->listaInstrucoes.tamanho > 0) psLiberarListaInstrucoes(&p->listaInstrucoes); // Garante que está vazia
        psInicializarListaInstrucoes(&p->listaInstrucoes); // Deixa a lista vazia e válida
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

    while (fgets(linha, sizeof(linha), arquivo)) {
        if (linha[0] == '\n' || linha[0] == '\0' || linha[0] == '#') continue;

        memset(&nova_inst, 0, sizeof(Instrucao_t));
        int itens_lidos = sscanf(linha, " %c", &nova_inst.tipoInstrucaoChar);

        if (itens_lidos < 1) continue;

        if (nova_inst.tipoInstrucaoChar == 'R') {
            sscanf(linha, " %*c %s", nova_inst.nome_arquivo_R);
        } else {
            sscanf(linha, " %*c %d %d", &nova_inst.arg1, &nova_inst.arg2);
        }
        psInserirInstrucao(&p->listaInstrucoes, nova_inst);
    }
    fclose(arquivo);
}

void psLiberarMemoria(ProcessoSimulado_t *p) {
    if (!p) return;
    psLiberarListaInstrucoes(&p->listaInstrucoes);
    free(p);
}

void psExecutarProximaInstrucao(ProcessoSimulado_t *p, long tempo_global_simulador, ProcessoSimulado_t **novo_processo_filho_ptr) {
    // Checagens iniciais de validade
    if (!p || novo_processo_filho_ptr == NULL) {
        if (p) p->estado_atual = EST_TERMINADO;
        return;
    }
    *novo_processo_filho_ptr = NULL; // Garante que o ponteiro de retorno está limpo

    // Processo só executa se estiver no estado de execução
    // (Gerenciador deve garantir isso antes de chamar)//A FAZER
    if (p->estado_atual != EST_EXECUCAO) {
        return;
    }

    // Verifica se o PC está dentro dos limites do programa
    if (p->pc < 0 || p->pc >= p->listaInstrucoes.tamanho) {
        p->estado_atual = EST_TERMINADO; // Fim do programa ou PC inválido
        return;
    }

    Instrucao_t *instr_atual_ptr = psObterInstrucaoNoPc(p);
    if (!instr_atual_ptr) {
        p->estado_atual = EST_TERMINADO;
        return;
    }

    Instrucao_t instr = *instr_atual_ptr; // Trabalha com uma cópia da instrução(Garatir mais segurança)
    int pc_foi_alterado_por_salto = 0;  // Flag para F e R que manipulam PC diretamente

    switch (instr.tipoInstrucaoChar) {
        case 'N': // Define o número de variáveis utilizáveis
            p->num_variaveis_declaradas = instr.arg1;
            // Trunca se exceder o limite físico da memória
            if (p->num_variaveis_declaradas < 0) p->num_variaveis_declaradas = 0;
            if (p->num_variaveis_declaradas > MAX_MEMORIA_PROCESSO_SIMULADO) {
                p->num_variaveis_declaradas = MAX_MEMORIA_PROCESSO_SIMULADO;
            }
            break;
        case 'D':
            if (instr.arg1 >= 0 && instr.arg1 < p->num_variaveis_declaradas) {
                p->memoria[instr.arg1] = 0;
            } else { p->estado_atual = EST_TERMINADO;}
            break;
        case 'V': // Atribui valor a uma variável
            if (instr.arg1 >= 0 && instr.arg1 < p->num_variaveis_declaradas) {
                p->memoria[instr.arg1] = instr.arg2;
            } else { p->estado_atual = EST_TERMINADO; }
            break;
        case 'A': // Adiciona valor a uma variável
            if (instr.arg1 >= 0 && instr.arg1 < p->num_variaveis_declaradas) {
                p->memoria[instr.arg1] += instr.arg2;
            } else { p->estado_atual = EST_TERMINADO; }
            break;
        case 'S': // Subtrai valor de uma variável
            if (instr.arg1 >= 0 && instr.arg1 < p->num_variaveis_declaradas) {
                p->memoria[instr.arg1] -= instr.arg2;
            } else { p->estado_atual = EST_TERMINADO;}
            break;
        case 'B': // Bloqueia o processo
            p->estado_atual = EST_BLOQUEADO;
            p->tempo_restante_bloqueio = (instr.arg1 > 0) ? instr.arg1 : 1; // Mínimo 1 ut
            break;
        case 'T': // Termina o processo
            p->estado_atual = EST_TERMINADO;
            return; // PC não avança, processo encerrou nesta instrução
        case 'F': // Cria um processo filho (fork)
            /* {
                // Cria uma estrutura base para o filho
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

                // Define PCs conforme especificação do PDF
                filho->pc = p->pc + 1;                  // Filho começa na instrução seguinte a 'F'
                p->pc = (p->pc + 1) + instr.arg1;     // Pai avança (PC_atual + 1) + n instruções

                filho->tempo_total_cpu_usado = 0; // Filho inicia com tempo de CPU zerado

                *novo_processo_filho_ptr = filho; // Retorna o filho para o Gerenciador
                pc_foi_alterado_por_salto = 1;    // PC do pai foi modificado diretamente
            } */
            {
                pid_t pid_filho = fork();
                if (pid_filho == -1) {
                    perror("Erro ao criar processo filho");
                    p->estado_atual = EST_TERMINADO; // Falha na criação do processo
                    return;
                }
                if (pid_filho == 0) { // Processo filho
                    p->pid_pai = p->pid; // Define o PID do pai
                    p->pid = -1; // Define o PID do filho (o gerenciador irá atribuir)
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
                psCarregarProgramaDeArquivo(p, instr.nome_arquivo_R);
                // Se o carregamento falhar, psCarregarProgramaDeArquivo já marca p->estado_atual = EST_TERMINADO
                // Se suceder, PC é 0. Em ambos os casos, a execução desta instrução 'R' termina aqui.
                return; // PC foi resetado para 0 (ou processo terminou), não há incremento normal
            }
            break;
        default: // Instrução desconhecida
            // fprintf(stderr, "PID %d: Instrução desconhecida '%c'\n", p->pid, instr.tipoInstrucaoChar);
            p->estado_atual = EST_TERMINADO; // Trata como erro fatal
            return; // PC não avança
    }

    // Avança o PC se não foi uma instrução de salto (F) e o processo continua em execução.
    // Para R e T, ou falhas, o fluxo já retornou. Para B, o PC avança normalmente.
    if (!pc_foi_alterado_por_salto && p->estado_atual == EST_EXECUCAO) {
        p->pc++;
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