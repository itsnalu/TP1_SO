#include "../include/config.h"
#include "../include/gerenciador.h" 
#include "../include/processos.h" 

#define MAX_CMD_LEN 100

int main(int argc, char *argv[]) {    
    int fd[2];
    pid_t pid;
    // Cria o pipe
    if (pipe(fd) < 0) {
        perror("Erro ao criar pipe");
        exit(EXIT_FAILURE);
    }
    // Interação com o usuário
    FILE *entrada = NULL;
    FILE *arq_entrada = NULL;
    char origem;

    Processos gerenciadorProcessos;
    inicializarGerenciador(&gerenciadorProcessos);
    // Cria o primeiro processo
    ProcessoSimulado_t *processo_inicial = criarNovoProcesso();
    adicionarProcesso(&gerenciadorProcessos, processo_inicial);

    char nomeArquivoInst[128];
    printf("Digite o nome do arquivo que contem as instrucoes: ");
    fgets(nomeArquivoInst, sizeof(nomeArquivoInst), stdin);
    nomeArquivoInst[strcspn(nomeArquivoInst, "\n")] = '\0';

    arq_entrada = fopen(nomeArquivoInst, "r");
    
    if (!arq_entrada) {
        perror("Erro ao abrir arquivo");
        close(fd[1]);
        exit(EXIT_FAILURE);
    }
    //carregarPrograma(processo_inicial, arq_entrada);
    printf("Deseja ler comandos do teclado (T) ou de um arquivo (F)? ");
    scanf(" %c", &origem);
    getchar(); // Consome '\n'

    if (origem == 'F' || origem == 'f') {
        char nomeArquivo[128];
        printf("Digite o nome do arquivo: ");
        fgets(nomeArquivo, sizeof(nomeArquivo), stdin);
        nomeArquivo[strcspn(nomeArquivo, "\n")] = '\0';

        entrada = fopen(nomeArquivo, "r");
        
        if (!entrada) {
            perror("Erro ao abrir arquivo");
            close(fd[1]);
            exit(EXIT_FAILURE);
        }
    } else {
        entrada = stdin;
        printf("Digite os comandos (U, I, M), um por linha. Ctrl+D para encerrar:\n");
    }
    // Cria processo filho para o gerenciador
    pid = fork();
    if (pid < 0) {
        perror("Erro ao criar processo gerenciador");
        exit(EXIT_FAILURE);
    }
    // Processo pai: lê comandos da entrada, escreve no pipe
    // Processo filho: lê do pipe, trata os comandos (U, I, M)    
    if (pid == 0) {
        // Filho: processo gerenciador
        close(fd[1]); // Fecha extremidade de escrita
        //gerenciador(fd[0], processo_inicial); // Chama função do gerenciador, lendo de fd[0]
        close(fd[0]);
        exit(EXIT_SUCCESS);
    } else {
        // Pai: processo controle
        close(fd[0]); // Fecha extremidade de leitura

        char buffer[MAX_CMD_LEN];
        // Leitura e envio de comandos para o gerenciador
        while (fgets(buffer, sizeof(buffer), entrada)) {
            // Remove lixo e valida comando
            buffer[strcspn(buffer, "\r\n")] = 0;
            if (strlen(buffer) == 0) continue;
            if (buffer[0] != 'U' && buffer[0] != 'I' && buffer[0] != 'M') {
                printf("Comando inválido: %s\n", buffer);
                continue;
            }
            strcat(buffer, "\n"); // Garante quebra de linha
            if (write(fd[1], buffer, strlen(buffer)) < 0) {
                perror("Erro ao escrever no pipe");
                break;
            }
            if (buffer[0] == 'M') break;  // Para depois do comando final
        }
        // Finaliza
        close(fd[1]);
        if (entrada != stdin) fclose(entrada);
        wait(NULL);  // Espera gerenciador encerrar
    }
    return 0;
}

/*O QUE TEMOS QUE FAZER (AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA NAO AGUENTO MAIS)
1.1 Implementar/Completar em include/fila.h e src/fila.c (OK, mas falta arrumar o main.c)

    Definir a estrutura FilaProcessos_t para armazenar PIDs
    Implementar funções para inicializar filas
    Implementar funções para adicionar processos às filas
    Implementar funções para remover processos das filas
    Implementar funções para verificar se uma fila está vazia

1.2 Completar em include/cpu.h e src/cpu.c (OK, mas falta arrumar o main.c)

    Definir a estrutura CPU_t com todos os campos necessários
    Implementar funções para inicializar a CPU
    Implementar funções para atualizar registradores da CPU
    Implementar funções para salvar e restaurar o contexto da CPU

1.3 Completar em include/estados.h e src/estados.c (OK, mas falta arrumar o main.c)

    Implementar funções para gerenciar a estrutura EstadoPronto_t
    Implementar funções para gerenciar a estrutura EstadoBloqueado_t
    Implementar funções para transição entre estados

Processo Simulado,
2.1 Completar em include/processoSimulado.h e src/processoSimulado.c

    Implementar a função psInicializarListaInstrucoes()
    Implementar a função psLiberarListaInstrucoes()
    Implementar a função psInserirInstrucao()
    Implementar a função psCopiarListaInstrucoes()
    Implementar a função psCarregarProgramaDeArquivo()
    Implementar a função psLiberarMemoria()
    Implementar a função psExecutarProximaInstrucao() com lógica para cada tipo de instrução:
        Instrução N (número de variáveis)
        Instrução D (declarar variável)
        Instrução V (definir valor)
        Instrução A (adicionar valor)
        Instrução S (subtrair valor)
        Instrução B (bloquear processo)
        Instrução T (terminar processo)
        Instrução F (criar processo filho)
        Instrução R (substituir imagem)

Gerenciador de Processos,
3.1 Corrigir em include/gerenciador.h e src/gerenciador.c

    Corrigir a função criarProcessoSimulado() (remover comentário "TESTAR E CONFERIR")
    Corrigir a função substituirImagemProcesso() (remover comentário "TESTAR E CONFERIR")
    Completar a função gerenciarTransicoesEstados()
    Corrigir a função escalonarProcessos()
    Corrigir a função trocarContexto() (remover comentário "TESTAR E CONFERIR")

3.2 Implementar a política de escalonamento

    Implementar a política de múltiplas filas com classes de prioridade
    Implementar a lógica de quantum por nível de prioridade
    Implementar a lógica de aumento/diminuição de prioridade
    Implementar uma política de escalonamento adicional (requisito do grupo)

Processo Controle e Gerenciador,
4.1 Completar em src/main.c

    Implementar a função carregarPrograma() (descomentar chamada)
    Implementar a função gerenciador() (descomentar chamada)
    Garantir comunicação correta via pipe entre processo controle e gerenciador
    Implementar tratamento dos comandos U, I e M no gerenciador

Processo de Impressão,
5.1 Implementar em novo arquivo src/impressao.c e include/impressao.h

    Criar estrutura para o processo de impressão
    Implementar função para imprimir o estado atual do sistema
    Implementar função para imprimir estatísticas finais
    Implementar diferentes configurações de impressão (simplificada/detalhada)

Arquivos de Teste,
6.1 Criar arquivos de teste

    Criar arquivo init para o primeiro processo simulado
    Criar arquivos para teste da instrução R (ex: file_a, file_b, etc.)
    Criar arquivo de comandos para teste automatizado


*/