#include "../include/config.h"
#include "../include/gerenciador.h"
// #include "../include/processos.h" // Aparentemente não é mais necessário se Processos e criarNovoProcesso não são usados aqui
#include "../include/cpu.h"
#include "../include/estados.h"
#include "../include/fila.h"
#include "../include/processoImpressao.h"
#include "../include/processoSimulado.h" // Essencial para psCriarNovo e psCarregarProgramaDeArquivo
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    // Apresentação inicial
    printf("\n===== Simulador de Gerenciamento de Processos =====\n\n");

    // 1. Inicialização
    int fd[2]; // Descritores do pipe
    pid_t pid;   // PID para o fork

    // Cria o pipe para comunicação entre Controle e Gerenciador
    if (pipe(fd) < 0) {
        perror("Erro ao criar pipe");
        exit(EXIT_FAILURE);
    }

    // 2. Configuração do Processo Inicial e Comandos
    FILE *entrada_comandos = NULL; // Stream para ler os comandos (teclado ou arquivo)
    char origem_comandos;      // 'T' para teclado, 'F' para arquivo

    // Define o nome do arquivo de instruções para o processo inicial
    char nomeArquivoInst[128];
    printf("Digite o nome do arquivo que contém as instruções para o processo inicial (ou Enter para usar '%s'): ", ARQUIVO_INIT_PROGRAMA);
    fgets(nomeArquivoInst, sizeof(nomeArquivoInst), stdin);
    nomeArquivoInst[strcspn(nomeArquivoInst, "\n")] = '\0'; // Remove o newline

    if (strlen(nomeArquivoInst) == 0) {
        strncpy(nomeArquivoInst, ARQUIVO_INIT_PROGRAMA, sizeof(nomeArquivoInst) -1);
        nomeArquivoInst[sizeof(nomeArquivoInst) - 1] = '\0'; // Garante terminação nula
        printf("Usando arquivo de instruções padrão: %s\n", nomeArquivoInst);
    }

    // Verifica brevemente se o arquivo de instruções é acessível
    FILE *teste_arq_inst = fopen(nomeArquivoInst, "r");
    if (!teste_arq_inst) {
        perror("Erro ao tentar acessar arquivo de instruções iniciais");
        close(fd[0]);
        close(fd[1]);
        exit(EXIT_FAILURE);
    }
    fclose(teste_arq_inst);

    // Cria a estrutura para o primeiro processo simulado (PID 0)
    ProcessoSimulado_t *processo_inicial = psCriarNovo(0, -1, 0, 0L);
    if (!processo_inicial) {
        fprintf(stderr, "Erro crítico: Falha ao alocar memória para o processo inicial.\n");
        exit(EXIT_FAILURE);
    }

    // Carrega o programa para o processo inicial
    psCarregarProgramaDeArquivo(processo_inicial, nomeArquivoInst);
    if (processo_inicial->listaInstrucoes.tamanho == 0 && processo_inicial->estado_atual == EST_TERMINADO) {
        fprintf(stderr, "Falha ao carregar instruções do arquivo '%s' para o processo inicial.\n", nomeArquivoInst);
        psLiberarMemoria(processo_inicial);
        exit(EXIT_FAILURE);
    }
    printf("Programa inicial '%s' carregado: %d instruções.\n", nomeArquivoInst, processo_inicial->listaInstrucoes.tamanho);

    // Determina a origem dos comandos (teclado ou arquivo)
    printf("\nDeseja ler comandos do teclado (T) ou de um arquivo (F)? ");
    if (scanf(" %c", &origem_comandos) != 1) {
         fprintf(stderr, "Entrada inválida para origem dos comandos.\n");
         psLiberarMemoria(processo_inicial);
         exit(EXIT_FAILURE);
    }
    while (getchar() != '\n'); // Limpa o buffer do stdin (consome o newline do scanf)

    if (origem_comandos == 'F' || origem_comandos == 'f') {
        char nomeArquivoCmd[128];
        printf("Digite o nome do arquivo de comandos (ou Enter para usar 'comandos.txt'): ");
        fgets(nomeArquivoCmd, sizeof(nomeArquivoCmd), stdin);
        nomeArquivoCmd[strcspn(nomeArquivoCmd, "\n")] = '\0';

        if (strlen(nomeArquivoCmd) == 0) {
            strcpy(nomeArquivoCmd, "comandos.txt");
            printf("Usando arquivo de comandos padrão: %s\n", nomeArquivoCmd);
        }

        entrada_comandos = fopen(nomeArquivoCmd, "r");
        if (!entrada_comandos) {
            perror("Erro ao abrir arquivo de comandos");
            psLiberarMemoria(processo_inicial);
            close(fd[0]);
            close(fd[1]);
            exit(EXIT_FAILURE);
        }
        printf("Lendo comandos do arquivo: %s\n", nomeArquivoCmd);
    } else {
        entrada_comandos = stdin;
        printf("\nDigite os comandos (U, I, M), um por linha. (Ctrl+D para encerrar no Linux, Ctrl+Z Enter no Windows):\n");
    }

    // 3. Criação do Processo Gerenciador
    printf("\nIniciando simulação e criando Processo Gerenciador...\n");
    fflush(stdout); // Garante que a saída seja impressa antes do fork
    pid = fork();

    if (pid < 0) {
        perror("Erro ao criar processo gerenciador (fork)");
        if (entrada_comandos != stdin) fclose(entrada_comandos);
        psLiberarMemoria(processo_inicial);
        close(fd[0]);
        close(fd[1]);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
    // PROCESSO FILHO: Gerenciador de Processos
    close(fd[1]); // Fecha a extremidade de escrita do pipe
    gerenciadorProcessosSimulados(fd[0], processo_inicial); // Passa o ponteiro
    close(fd[0]);
    printf("[Processo Gerenciador PID %d] Finalizado.\n", getpid());
    exit(EXIT_SUCCESS);
    } else {
        // PROCESSO PAI: Controle
        close(fd[0]);

        char buffer_comando[MAX_CMD_LEN];
        printf("[Processo Controle] Enviando comandos para o Gerenciador...\n");

        while (fgets(buffer_comando, sizeof(buffer_comando), entrada_comandos)) {
            buffer_comando[strcspn(buffer_comando, "\r\n")] = 0;

            if (strlen(buffer_comando) == 0) continue;

            if (buffer_comando[0] != 'U' && buffer_comando[0] != 'I' && buffer_comando[0] != 'M') {
                printf("[Processo Controle] Comando inválido ignorado: '%s'\n", buffer_comando);
                continue;
            }

            // Adiciona newline para garantir que fgets no gerenciador leia uma linha completa
            char comando_com_newline[MAX_CMD_LEN + 2]; // +1 para newline, +1 para null terminator
            snprintf(comando_com_newline, sizeof(comando_com_newline), "%s\n", buffer_comando);
            
            if (write(fd[1], comando_com_newline, strlen(comando_com_newline)) < 0) {
                perror("[Processo Controle] Erro ao escrever no pipe");
                break; 
            }

            if (buffer_comando[0] == 'M') {
                printf("[Processo Controle] Comando final 'M' enviado.\n");
                break; // Encerra o loop de envio após o comando 'M'
            }
        }

        // 5. Finalização do Processo Controle
        printf("[Processo Controle] Todos os comandos foram enviados. Fechando pipe de escrita.\n");
        close(fd[1]); // Fecha a extremidade de escrita. Isso enviará EOF ao leitor (gerenciador) se ele ainda estiver lendo.

        if (entrada_comandos != stdin) {
            fclose(entrada_comandos);
        }

        printf("[Processo Controle] Aguardando o Processo Gerenciador (PID %d) finalizar...\n", pid);
        wait(NULL);
        
        printf("\n===== Simulação Concluída. Processo Controle Encerrado. =====\n");
    }

    return 0;
}
