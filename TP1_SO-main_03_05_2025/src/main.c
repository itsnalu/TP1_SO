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
    ProcessoSimulado *processo_inicial = criarNovoProcesso();
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

    carregarPrograma(processo_inicial, arq_entrada);
    
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
        gerenciador(fd[0], processo_inicial); // Chama função do gerenciador, lendo de fd[0]
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