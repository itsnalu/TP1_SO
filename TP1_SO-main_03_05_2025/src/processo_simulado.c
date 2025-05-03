#include "../include/config.h"
#include "../include/processos.h"

void FLVazia(TipoLista *Lista){ 
    Lista -> Primeiro = (TipoApontador) malloc(sizeof(TipoCelula));
    Lista -> Ultimo = Lista -> Primeiro;
    Lista -> Primeiro -> Prox = NULL;
}

int Vazia(TipoLista Lista){ 
    return (Lista.Primeiro == Lista.Ultimo);
}

void Insere(TipoInstrucao x, TipoLista *Lista){ 
    Lista -> Ultimo -> Prox = (TipoApontador) malloc(sizeof(TipoCelula));
    Lista -> Ultimo = Lista -> Ultimo -> Prox;
    Lista -> Ultimo -> Instrucao = x;
    Lista -> Ultimo -> Prox = NULL;
}

void Retira(TipoApontador p, TipoLista *Lista, TipoInstrucao *Instrucao){
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
        printf("%c %d %d \n", Aux->Instrucao.tp_inst,Aux->Instrucao.arg1,Aux->Instrucao.arg2);
        Aux = Aux -> Prox;
    }
}

void carregar_programa(ProcessoSimulado *processo, FILE *arquivo) {
    //printf("\n\n\n entrou em carregar_programa \n\n\n");
    char linha[100];
    TipoInstrucao instrucao;
    FLVazia(&processo->listaInstrucoes);

    while (fgets(linha, sizeof(linha), arquivo)) {
        // Ignora linhas vazias
        if (linha[0] == '\n' || linha[0] == '\0') continue;

        // Inicializa os valores para garantir limpeza
        instrucao.tp_inst = '\0';
        instrucao.arg1 = 0;
        instrucao.arg2 = 0;

        // Tenta ler a linha com até dois argumentos
        int num = sscanf(linha, " %c %d %d", &instrucao.tp_inst, &instrucao.arg1, &instrucao.arg2);

        if (num >= 1) {
            Insere(instrucao, &processo->listaInstrucoes);
        }
    }

    Imprime(processo->listaInstrucoes); // imprime a lista
}

void executar_instrucao(ProcessoSimulado *p) {
    /*printf("\n arg1 %d", p->listaInstrucoes.Primeiro->Instrucao.arg1);
    printf("\n arg2 %d", p->listaInstrucoes.Primeiro->Instrucao.arg2);
    printf("\n tp_inst %c", p->listaInstrucoes.Primeiro->Instrucao.tp_inst);
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

    TipoInstrucao instr = atual->Instrucao;

    switch (instr.tp_inst) {
        case 'N':
            strcpy(p->estado, "EXECUCAO");
            p->num_variaveis = instr.arg1;
            for (int i = 0; i < p->num_variaveis; i++)
                p->memoria[i] = 0;
            printf("Declaradas %d variáveis com valor inicial 0 \n", p->num_variaveis);
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
        
        case 'F':

        default:
            printf("Instrução desconhecida: %c\n", instr.tp_inst);
            break;
    }

    p->pc++; // Avança para próxima instrução
}
