#include "../include/config.h"
#include "../include/processos.h"
#include "../include/tabelaProcessos.h"

void FLVazia(TipoLista *Lista){ 
    Lista -> Primeiro = (TipoApontador) malloc(sizeof(TipoCelula));
    Lista -> Ultimo = Lista -> Primeiro;
    Lista -> Primeiro -> Prox = NULL;
}

int Vazia(TipoLista Lista){ 
    return (Lista.Primeiro == Lista.Ultimo);
}

void Insere(instrucao x, TipoLista *Lista){ 
    Lista -> Ultimo -> Prox = (TipoApontador) malloc(sizeof(TipoCelula));
    Lista -> Ultimo = Lista -> Ultimo -> Prox;
    Lista -> Ultimo -> Instrucao = x;
    Lista -> Ultimo -> Prox = NULL;
}

void Retira(TipoApontador p, TipoLista *Lista, instrucao *Instrucao){
  TipoApontador q;
  if (Vazia(*Lista) || p == NULL || p -> Prox == NULL){
    printf(" Erro   Lista vazia ou posicao nao existe\n");
    return;
  }
  q = p -> Prox;
  *Instrucao = q -> Instrucao;
  p -> Prox = q -> Prox;
  if (p -> Prox == NULL) Lista -> Ultimo = p;
  free(q);
}

void Imprime(TipoLista Lista){ 
    TipoApontador Aux;
    Aux = Lista.Primeiro -> Prox;
    while (Aux != NULL){ 
        printf("%c %d %d \n", Aux->Instrucao.tipoInstrucao,Aux->Instrucao.arg1,Aux->Instrucao.arg2);
        Aux = Aux -> Prox;
    }
}

void carregarPrograma(ProcessoSimulado *processo, FILE *arquivo) {
    //printf("\n\n\n entrou em carregarPrograma \n\n\n");
    char linha[100];
    instrucao instrucao;
    FLVazia(&processo->listaInstrucoes);

    while (fgets(linha, sizeof(linha), arquivo)) {
        // Ignora linhas vazias
        if (linha[0] == '\n' || linha[0] == '\0') continue;

        // Inicializa os valores para garantir limpeza
        instrucao.tipoInstrucao = '\0';
        instrucao.arg1 = 0;
        instrucao.arg2 = 0;

        // Tenta ler a linha com até dois argumentos
        int num = sscanf(linha, " %c %d %d", &instrucao.tipoInstrucao, &instrucao.arg1, &instrucao.arg2);

        if (num >= 1) {
            Insere(instrucao, &processo->listaInstrucoes);
        }
    }

    Imprime(processo->listaInstrucoes); // imprime a lista
}

void executarInstrucao(ProcessoSimulado *p) {
    /*printf("\n arg1 %d", p->listaInstrucoes.Primeiro->Instrucao.arg1);
    printf("\n arg2 %d", p->listaInstrucoes.Primeiro->Instrucao.arg2);
    printf("\n tipoInstrucao %c", p->listaInstrucoes.Primeiro->Instrucao.tipoInstrucao);
    printf("\n pc %d \n", p->pc);*/

    if (p->qtd_bloq > 0){
        printf("Processo está bloqueado por %d unidade(s)\n", p->qtd_bloq);
        p->qtd_bloq--;
        return;
    }

    TipoApontador atual = p->listaInstrucoes.Primeiro->Prox;
    int i = 0;

    // Percorre até o pc atual
    while (atual != NULL && i < p->pc) {
        atual = atual->Prox;
        i++;
    }

    if (atual == NULL) {
        printf("Fim do programa\n");
        return;
    }

    instrucao instr = atual->Instrucao;

    switch (instr.tipoInstrucao) {
        case 'N':
            strcpy(p->estado, "EXECUCAO");
            p->numVariaveis = instr.arg1;
            for (int i = 0; i < p->numVariaveis; i++)
                p->memoria[i] = 0;
            printf("Declaradas %d variáveis com valor inicial 0 \n", p->numVariaveis);
            break;

        case 'D':
            strcpy(p->estado, "EXECUCAO");
            if (instr.arg1 >= 0 && instr.arg1 < 100) {
                p->memoria[instr.arg1] = 0;
                printf("Variável %d declarada com valor 0\n", instr.arg1);
            }
            break;

        case 'V':
            strcpy(p->estado, "EXECUCAO");
            if (instr.arg1 >= 0 && instr.arg1 < 100) {
                p->memoria[instr.arg1] = instr.arg2;
                printf("Variável %d atribuída com valor %d \n", instr.arg1, instr.arg2);
            }
            break;

        case 'A':
            strcpy(p->estado, "EXECUCAO");
            if (instr.arg1 >= 0 && instr.arg1 < 100) {
                p->memoria[instr.arg1] += instr.arg2;
                printf("Adicionado %d à variável %d. Novo valor: %d \n", instr.arg2, instr.arg1, p->memoria[instr.arg1]);
            }
            break;

        case 'S':
            strcpy(p->estado, "EXECUCAO");
            if (instr.arg1 >= 0 && instr.arg1 < 100) {
                p->memoria[instr.arg1] -= instr.arg2;
                printf("Subtraído %d da variável %d. Novo valor: %d \n", instr.arg2, instr.arg1, p->memoria[instr.arg1]);
            }
            break;

        case 'B':
            strcpy(p->estado, "BLOQUEADO");
            p->qtd_bloq = instr.arg1;
            printf("Processo bloqueado por %d unidades de tempo\n", instr.arg1);
            break;
        
        case 'T':
            strcpy(p->estado, "EXECUCAO");
            if (instr.arg1 >= 0 && instr.arg1 < 100) {
                printf("Valor da variável %d: %d \n", instr.arg1, p->memoria[instr.arg1]);
            }
            break;
        case 'F':
            strcpy(p->estado, "EXECUCAO");
            ProcessoSimulado *novoProcesso = (ProcessoSimulado *)malloc(sizeof(ProcessoSimulado));
            if (novoProcesso == NULL) {
                printf("Erro ao alocar memória para novo processo\n");
                return;
            }
            memcpy(novoProcesso, p, sizeof(ProcessoSimulado));
            novoProcesso->pc = p->pc + 1;
            p->pc = instr.arg1; 
            // definir um index para o novo processo
            static int proximo_pid = 1;
            novoProcesso->pid = proximo_pid++;
            printf("Criado novo processo com PID %d. Processo pai continua com PID", novoProcesso->pid, p->pid);
            // Adicionar o novo processo à lista de processos
            // Vou colocar a aplicação da  função adicionar processo aqui, em qual header? Eu não sei.
            adicionarProcessoAoGerenciador(novoProcesso);
            break;
        case 'R':
            strcyp(p->estado, "EXECUCAO");
            // Abre o arquivo como nome_do_arquivo
            char nome_do_arquivo[100];
            sprintf(nome_do_arquivo, "programas/%d.txt", instr.arg1);

            FILE *arquivo = fopen(nome_do_arquivo, "r");
            if (arquivo == NULL) {
                printf("Erro ao abrir o arquivo %s\n", nome_do_arquivo);
                return;
            }
            // Limpar a lista de instruções atual
            FLVazia(&p->listaInstrucoes);
            // Carregar o novo programa
            carregarPrograma(p, arquivo);   
            // Fechar o arquivo
            fclose(arquivo);
            // Redefinir o contador de programa para 0, indo para a primeira instrução
            p->pc = 0;

            printf("Programa do processo substituído pelo arquivo %s. PC redefinido para 0.\n ", nome_do_arquivo);
            
            break;
            
        default:
            printf("Instrução desconhecida: %c\n", instr.tipoInstrucao);
            break;
    }

    p->pc++; // Avança para próxima instrução
}
