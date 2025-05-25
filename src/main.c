#include "config.h"
#include "gerenciador.h"      
#include "processoSimulado.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>           
#include <sys/types.h>        
#include <sys/wait.h>         
#include <pthread.h>          

pthread_mutex_t impressao_mutex;

int main(int argc, char *argv[]) {
    if (pthread_mutex_init(&impressao_mutex, NULL) != 0) {
        perror("MAIN ERRO: Falha ao inicializar impressao_mutex");
        exit(EXIT_FAILURE);
    }

    printf("===== Simulador de Gerenciamento de Processos (Modelo Threads) =====\n\n");

    int fd_pipe[2]; 
    pid_t pid_gerenciador_os; 

    if (pipe(fd_pipe) < 0) {
        perror("MAIN ERRO: Falha ao criar pipe");
        pthread_mutex_destroy(&impressao_mutex);
        exit(EXIT_FAILURE);
    }

    char nomeArquivoInstrucoes[256]; 
    printf("Digite o nome do arquivo que contém as instruções para o processo inicial (Enter para usar '%s'): ", ARQUIVO_INIT_PROGRAMA);
    if (fgets(nomeArquivoInstrucoes, sizeof(nomeArquivoInstrucoes), stdin) == NULL) {
        fprintf(stderr, "MAIN ERRO: Falha ao ler nome do arquivo de instruções.\n");
        close(fd_pipe[0]); close(fd_pipe[1]);
        pthread_mutex_destroy(&impressao_mutex);
        exit(EXIT_FAILURE);
    }
    nomeArquivoInstrucoes[strcspn(nomeArquivoInstrucoes, "\n\r")] = '\0'; 

    if (strlen(nomeArquivoInstrucoes) == 0) {
        strncpy(nomeArquivoInstrucoes, ARQUIVO_INIT_PROGRAMA, sizeof(nomeArquivoInstrucoes) -1);
        nomeArquivoInstrucoes[sizeof(nomeArquivoInstrucoes) - 1] = '\0'; 
    }
    printf("Usando arquivo de instruções padrão: %s\n", nomeArquivoInstrucoes);

    FILE *teste_arq_inst = fopen(nomeArquivoInstrucoes, "r");
    if (!teste_arq_inst) {
        perror("MAIN ERRO: Não foi possível abrir o arquivo de instruções iniciais especificado");
        fprintf(stderr, " -> Arquivo tentado: %s\n", nomeArquivoInstrucoes);
        close(fd_pipe[0]); close(fd_pipe[1]);
        pthread_mutex_destroy(&impressao_mutex);
        exit(EXIT_FAILURE);
    }
    fclose(teste_arq_inst);

    ProcessoSimulado_t *info_processo_inicial = psCriarNovo(-1, 0, 0L); 
    if (!info_processo_inicial) {
        fprintf(stderr, "MAIN ERRO CRÍTICO: Falha ao alocar memória para as informações do processo inicial.\n");
        close(fd_pipe[0]); close(fd_pipe[1]);
        pthread_mutex_destroy(&impressao_mutex);
        exit(EXIT_FAILURE);
    }
    
    psCarregarProgramaDeArquivo(info_processo_inicial, nomeArquivoInstrucoes);
    if (info_processo_inicial->estado_atual == EST_TERMINADO || info_processo_inicial->listaInstrucoes.tamanho == 0) {
        fprintf(stderr, "MAIN ERRO: Falha ao carregar instruções de '%s' para o processo inicial ou o arquivo está vazio/inválido.\n", nomeArquivoInstrucoes);
        psLiberarMemoria(info_processo_inicial); 
        close(fd_pipe[0]); close(fd_pipe[1]);
        pthread_mutex_destroy(&impressao_mutex);
        exit(EXIT_FAILURE);
    }
    printf("MAIN: Programa '%s' carregado com %d instruções para processo inicial.\n",
           nomeArquivoInstrucoes, info_processo_inicial->listaInstrucoes.tamanho);

    FILE *stream_entrada_comandos = stdin; 
    char buffer_escolha_origem[10]; 

    printf("\nDeseja ler comandos do teclado (T) ou de um arquivo (F)? ");
    if (fgets(buffer_escolha_origem, sizeof(buffer_escolha_origem), stdin) == NULL) {
         fprintf(stderr, "MAIN ERRO: Falha ao ler escolha da origem dos comandos.\n");
         psLiberarMemoria(info_processo_inicial);
         close(fd_pipe[0]); close(fd_pipe[1]);
         pthread_mutex_destroy(&impressao_mutex);
         exit(EXIT_FAILURE);
    }

    char escolha_origem = buffer_escolha_origem[0];

    if (escolha_origem == 'F' || escolha_origem == 'f') {
        char nomeArquivoComandos[256]; 
        printf("Digite o nome do arquivo de comandos (Enter para usar 'comandos.txt'): ");
        if (fgets(nomeArquivoComandos, sizeof(nomeArquivoComandos), stdin) == NULL) {
            fprintf(stderr, "MAIN ERRO: Falha ao ler nome do arquivo de comandos.\n");
            psLiberarMemoria(info_processo_inicial);
            close(fd_pipe[0]); close(fd_pipe[1]);
            pthread_mutex_destroy(&impressao_mutex);
            exit(EXIT_FAILURE);
        }
        nomeArquivoComandos[strcspn(nomeArquivoComandos, "\n\r")] = '\0';

        if (strlen(nomeArquivoComandos) == 0) {
            strcpy(nomeArquivoComandos, "comandos.txt"); 
        }
        printf("Usando arquivo de comandos: %s\n", nomeArquivoComandos);

        stream_entrada_comandos = fopen(nomeArquivoComandos, "r");
        if (!stream_entrada_comandos) {
            perror("MAIN ERRO: Falha ao abrir arquivo de comandos especificado");
            fprintf(stderr, " -> Arquivo tentado: %s\n", nomeArquivoComandos);
            psLiberarMemoria(info_processo_inicial);
            close(fd_pipe[0]); close(fd_pipe[1]);
            pthread_mutex_destroy(&impressao_mutex);
            exit(EXIT_FAILURE);
        }
    } else {
        printf("MAIN: Digite os comandos (U, I, M), um por linha. (Ctrl+D para encerrar no Linux/macOS, Ctrl+Z Enter no Windows):\n");
    }

    printf("\nMAIN: Iniciando simulação e criando Processo Gerenciador (via fork)...\n");
    fflush(stdout); 
    pid_gerenciador_os = fork();

    if (pid_gerenciador_os < 0) {
        perror("MAIN ERRO: Falha ao criar processo gerenciador (fork)");
        if (stream_entrada_comandos != stdin && stream_entrada_comandos != NULL) fclose(stream_entrada_comandos);
        psLiberarMemoria(info_processo_inicial);
        close(fd_pipe[0]); close(fd_pipe[1]);
        pthread_mutex_destroy(&impressao_mutex);
        exit(EXIT_FAILURE);
    }

    if (pid_gerenciador_os == 0) {
        close(fd_pipe[1]); 
        gerenciadorProcessosSimulados(fd_pipe[0], info_processo_inicial);
        close(fd_pipe[0]); 
        exit(EXIT_SUCCESS); 
    } else { 
        close(fd_pipe[0]); 

        char line_buffer[MAX_CMD_LEN + 2];      
        char comando_a_enviar[MAX_CMD_LEN + 2]; 

        while (fgets(line_buffer, sizeof(line_buffer), stream_entrada_comandos)) {
            line_buffer[strcspn(line_buffer, "\r\n")] = 0;

            if (strlen(line_buffer) == 0) continue;
            
            // Determina o comprimento do conteúdo do comando, limitado por MAX_CMD_LEN
            int content_len = strlen(line_buffer);
            if (content_len > MAX_CMD_LEN) {
                content_len = MAX_CMD_LEN; // Trunca o conteúdo se for maior que MAX_CMD_LEN
            }

            // Formata "conteudo_do_comando[até content_len caracteres]\n"
            // A string formatada terá 'content_len' caracteres + 1 para '\n'.
            // Máximo de caracteres a serem escritos por snprintf (excluindo o nulo): content_len + 1
            // Máximo de 'content_len + 1' é MAX_CMD_LEN + 1.
            // snprintf precisa de espaço para (MAX_CMD_LEN + 1) caracteres + '\0'.
            // Total de bytes necessários: MAX_CMD_LEN + 2.
            // sizeof(comando_a_enviar) é MAX_CMD_LEN + 2. Isso deve ser seguro.
            // O aviso de "unused variable ret" em uma linha 159 não deve ocorrer com este código.
            // A linha ~190 (ou similar) do seu erro de truncamento é esta snprintf:
            snprintf(comando_a_enviar, sizeof(comando_a_enviar), "%.*s\n", content_len, line_buffer);

            if (write(fd_pipe[1], comando_a_enviar, strlen(comando_a_enviar)) < 0) {
                perror("MAIN ERRO: Falha ao escrever no pipe para o gerenciador");
                break; 
            }
            
            // Verifica o comando *original lido* (line_buffer) para 'M', não o formatado.
            // E garante que seja apenas 'M'.
            if (line_buffer[0] == 'M' && strlen(line_buffer) == 1) { 
                break; 
            }
        }

        close(fd_pipe[1]); 
        if (stream_entrada_comandos != stdin && stream_entrada_comandos != NULL) {
            fclose(stream_entrada_comandos);
        }

        wait(NULL); 
        printf("\n===== Simulação Concluída. Processo Controle Encerrado. =====\n");
    }

    pthread_mutex_destroy(&impressao_mutex);
    return 0;
}