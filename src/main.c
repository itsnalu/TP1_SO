#include "../include/config.h"          // Para ARQUIVO_INIT_PROGRAMA, MAX_CMD_LEN, e declarações extern de mutexes
#include "../include/gerenciador.h"      // Para gerenciadorProcessosSimulados
#include "../include/processoSimulado.h" // Para psCriarNovo, psCarregarProgramaDeArquivo, psLiberarMemoria
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>           
#include <sys/types.h>        
#include <sys/wait.h>         
#include <pthread.h>          

// Definição dos mutexes globais (declarados 'extern' em config.h)
pthread_mutex_t impressao_mutex;
pthread_mutex_t prontos_mutex;
pthread_mutex_t bloqueados_mutex;
pthread_mutex_t tabela_processos_mutex;

int main(void) {
    // Apresentação inicial
    printf("\n===== Simulador de Gerenciamento de Processos (Modelo Threads com MLFQ) =====\n\n");

    // 1. Inicialização de Mutexes Globais
    if (pthread_mutex_init(&impressao_mutex, NULL) != 0) {
        perror("MAIN ERRO: Falha ao inicializar impressao_mutex");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&prontos_mutex, NULL) != 0) {
        perror("MAIN ERRO: Falha ao inicializar prontos_mutex");
        pthread_mutex_destroy(&impressao_mutex); 
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&bloqueados_mutex, NULL) != 0) {
        perror("MAIN ERRO: Falha ao inicializar bloqueados_mutex");
        pthread_mutex_destroy(&impressao_mutex);
        pthread_mutex_destroy(&prontos_mutex);
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&tabela_processos_mutex, NULL) != 0) {
        perror("MAIN ERRO: Falha ao inicializar tabela_processos_mutex");
        pthread_mutex_destroy(&impressao_mutex);
        pthread_mutex_destroy(&prontos_mutex);
        pthread_mutex_destroy(&bloqueados_mutex);
        exit(EXIT_FAILURE);
    }
    printf("[Main] Mutexes globais inicializados.\n");

    // 2. Configuração do Pipe e Leitura de Configurações Iniciais
    int fd_pipe[2]; 
    pid_t pid_filho_gerenciador;

    if (pipe(fd_pipe) < 0) {
        perror("MAIN ERRO: Falha ao criar pipe");
        pthread_mutex_destroy(&impressao_mutex);
        pthread_mutex_destroy(&prontos_mutex);
        pthread_mutex_destroy(&bloqueados_mutex);
        pthread_mutex_destroy(&tabela_processos_mutex);
        exit(EXIT_FAILURE);
    }

    FILE *entrada_comandos_stream = NULL; 
    char origem_comandos_char;      

    char nome_arquivo_instrucoes_inicial[256]; 
    printf("Digite o nome do arquivo que contém as instruções para o processo inicial (Enter para usar '%s'): ", ARQUIVO_INIT_PROGRAMA);
    if (fgets(nome_arquivo_instrucoes_inicial, sizeof(nome_arquivo_instrucoes_inicial), stdin) == NULL) {
        fprintf(stderr, "MAIN ERRO: Falha ao ler nome do arquivo de instruções iniciais.\n");
        close(fd_pipe[0]); close(fd_pipe[1]);
        pthread_mutex_destroy(&impressao_mutex);
        pthread_mutex_destroy(&prontos_mutex);
        pthread_mutex_destroy(&bloqueados_mutex);
        pthread_mutex_destroy(&tabela_processos_mutex);
        exit(EXIT_FAILURE);
    }
    nome_arquivo_instrucoes_inicial[strcspn(nome_arquivo_instrucoes_inicial, "\n\r")] = '\0'; 

    if (strlen(nome_arquivo_instrucoes_inicial) == 0) {
        strncpy(nome_arquivo_instrucoes_inicial, ARQUIVO_INIT_PROGRAMA, sizeof(nome_arquivo_instrucoes_inicial) -1);
        nome_arquivo_instrucoes_inicial[sizeof(nome_arquivo_instrucoes_inicial) - 1] = '\0'; 
    }
    printf("Usando arquivo de instruções para processo inicial: %s\n", nome_arquivo_instrucoes_inicial);

    FILE *teste_arq_inst_fp = fopen(nome_arquivo_instrucoes_inicial, "r");
    if (!teste_arq_inst_fp) {
        perror("MAIN ERRO: Não foi possível abrir o arquivo de instruções iniciais especificado");
        fprintf(stderr, " -> Arquivo tentado: %s\n", nome_arquivo_instrucoes_inicial);
        close(fd_pipe[0]); close(fd_pipe[1]);
        pthread_mutex_destroy(&impressao_mutex);
        pthread_mutex_destroy(&prontos_mutex);
        pthread_mutex_destroy(&bloqueados_mutex);
        pthread_mutex_destroy(&tabela_processos_mutex);
        exit(EXIT_FAILURE);
    }
    fclose(teste_arq_inst_fp);

    // 3. Criação da Informação do Processo Inicial (Temporário)
    ProcessoSimulado_t *processo_inicial_info = psCriarNovo(-1, 0, 0L); 
    if (!processo_inicial_info) {
        fprintf(stderr, "MAIN ERRO CRÍTICO: Falha ao alocar memória para informações do processo inicial.\n");
        close(fd_pipe[0]); close(fd_pipe[1]);
        pthread_mutex_destroy(&impressao_mutex);
        pthread_mutex_destroy(&prontos_mutex);
        pthread_mutex_destroy(&bloqueados_mutex);
        pthread_mutex_destroy(&tabela_processos_mutex);
        exit(EXIT_FAILURE);
    }
    
    psCarregarProgramaDeArquivo(processo_inicial_info, nome_arquivo_instrucoes_inicial);
    if (processo_inicial_info->estado_atual == EST_TERMINADO || processo_inicial_info->listaInstrucoes.tamanho == 0) {
        fprintf(stderr, "MAIN ERRO: Falha ao carregar instruções de '%s' para o processo inicial, ou o arquivo está vazio/inválido.\n", nome_arquivo_instrucoes_inicial);
        psLiberarMemoria(processo_inicial_info); 
        close(fd_pipe[0]); close(fd_pipe[1]);
        pthread_mutex_destroy(&impressao_mutex);
        pthread_mutex_destroy(&prontos_mutex);
        pthread_mutex_destroy(&bloqueados_mutex);
        pthread_mutex_destroy(&tabela_processos_mutex);
        exit(EXIT_FAILURE);
    }
    printf("[Main] Programa '%s' carregado com %d instruções para a struct temporária do processo inicial.\n",
           nome_arquivo_instrucoes_inicial, processo_inicial_info->listaInstrucoes.tamanho);

    printf("\nDeseja ler comandos do teclado (T) ou de um arquivo (F)? ");
    char buffer_escolha_origem_cmd[10]; 
    if (fgets(buffer_escolha_origem_cmd, sizeof(buffer_escolha_origem_cmd), stdin) == NULL) {
         fprintf(stderr, "MAIN ERRO: Falha ao ler escolha da origem dos comandos.\n");
         psLiberarMemoria(processo_inicial_info);
         close(fd_pipe[0]); close(fd_pipe[1]);
        pthread_mutex_destroy(&impressao_mutex);
        pthread_mutex_destroy(&prontos_mutex);
        pthread_mutex_destroy(&bloqueados_mutex);
        pthread_mutex_destroy(&tabela_processos_mutex);
         exit(EXIT_FAILURE);
    }
    origem_comandos_char = buffer_escolha_origem_cmd[0]; 

    if (origem_comandos_char == 'F' || origem_comandos_char == 'f') {
        char nome_arquivo_comandos_str[256]; 
        printf("Digite o nome do arquivo de comandos (Enter para usar 'comandos.txt'): ");
        if (fgets(nome_arquivo_comandos_str, sizeof(nome_arquivo_comandos_str), stdin) == NULL) {
            fprintf(stderr, "MAIN ERRO: Falha ao ler nome do arquivo de comandos.\n");
            psLiberarMemoria(processo_inicial_info);
            close(fd_pipe[0]); close(fd_pipe[1]);
            pthread_mutex_destroy(&impressao_mutex);
            pthread_mutex_destroy(&prontos_mutex);
            pthread_mutex_destroy(&bloqueados_mutex);
            pthread_mutex_destroy(&tabela_processos_mutex);
            exit(EXIT_FAILURE);
        }
        nome_arquivo_comandos_str[strcspn(nome_arquivo_comandos_str, "\n\r")] = '\0';

        if (strlen(nome_arquivo_comandos_str) == 0) {
            strcpy(nome_arquivo_comandos_str, "comandos.txt"); 
        }
        printf("Usando arquivo de comandos: %s\n", nome_arquivo_comandos_str);

        entrada_comandos_stream = fopen(nome_arquivo_comandos_str, "r");
        if (!entrada_comandos_stream) {
            perror("MAIN ERRO: Falha ao abrir arquivo de comandos especificado");
            fprintf(stderr, " -> Arquivo tentado: %s\n", nome_arquivo_comandos_str);
            psLiberarMemoria(processo_inicial_info);
            close(fd_pipe[0]); close(fd_pipe[1]);
            pthread_mutex_destroy(&impressao_mutex);
            pthread_mutex_destroy(&prontos_mutex);
            pthread_mutex_destroy(&bloqueados_mutex);
            pthread_mutex_destroy(&tabela_processos_mutex);
            exit(EXIT_FAILURE);
        }
    } else {
        entrada_comandos_stream = stdin;
        printf("[Main] Digite os comandos (U, I, M), um por linha.\n(Ctrl+D para encerrar no Linux/macOS, Ctrl+Z Enter no Windows):\n");
    }

    printf("\n[Main] Iniciando simulação e criando Processo Gerenciador (via fork)...\n");
    fflush(stdout); 
    pid_filho_gerenciador = fork();

    if (pid_filho_gerenciador < 0) {
        perror("MAIN ERRO: Falha ao criar processo gerenciador (fork)");
        if (entrada_comandos_stream != stdin && entrada_comandos_stream != NULL) fclose(entrada_comandos_stream);
        psLiberarMemoria(processo_inicial_info);
        close(fd_pipe[0]); close(fd_pipe[1]);
        pthread_mutex_destroy(&impressao_mutex);
        pthread_mutex_destroy(&prontos_mutex);
        pthread_mutex_destroy(&bloqueados_mutex);
        pthread_mutex_destroy(&tabela_processos_mutex);
        exit(EXIT_FAILURE);
    }

    if (pid_filho_gerenciador == 0) {
        close(fd_pipe[1]); 
        gerenciadorProcessosSimulados(fd_pipe[0], processo_inicial_info);
        close(fd_pipe[0]); 
        exit(EXIT_SUCCESS); 
    } else { 
        close(fd_pipe[0]); 
        processo_inicial_info = NULL; 

        char linha_lida_do_input[MAX_CMD_LEN + 2];      
        char comando_para_pipe[MAX_CMD_LEN + 2];

        printf("[Processo Controle] Enviando comandos para o Gerenciador...\n");

        while (fgets(linha_lida_do_input, sizeof(linha_lida_do_input), entrada_comandos_stream)) {
            linha_lida_do_input[strcspn(linha_lida_do_input, "\r\n")] = 0; // Remove newline/CR da linha lida

            if (strlen(linha_lida_do_input) == 0) continue; // Ignora linhas vazias
            
            // Validação do primeiro caractere do comando original
            if (linha_lida_do_input[0] != 'U' && linha_lida_do_input[0] != 'I' && linha_lida_do_input[0] != 'M') {
                printf("[Processo Controle] Comando inválido ('%s') lido e ignorado.\n", linha_lida_do_input);
                continue;
            }
            
            if (strlen(linha_lida_do_input) > MAX_CMD_LEN) {
                linha_lida_do_input[MAX_CMD_LEN] = '\0'; 
            }
            
            int chars_escritos = snprintf(comando_para_pipe, sizeof(comando_para_pipe), "%s\n", linha_lida_do_input);

            // Verificação adicional (opcional, para robustez extrema)
            if (chars_escritos < 0 || (size_t)chars_escritos >= sizeof(comando_para_pipe)) {
                fprintf(stderr, "MAIN ERRO (Controle): Erro ou truncamento ao formatar comando para o pipe com snprintf.\n");
                // Lidar com o erro, talvez parar de enviar comandos
                break;
            }
            
            if (write(fd_pipe[1], comando_para_pipe, strlen(comando_para_pipe)) < 0) {
                perror("MAIN ERRO (Controle): Falha ao escrever no pipe para o gerenciador");
                break; 
            }
            
            // Verifica o comando original (sem o newline adicionado) para 'M'
            if (linha_lida_do_input[0] == 'M' && strlen(linha_lida_do_input) == 1) { 
                printf("[Processo Controle] Comando final 'M' enviado para o Gerenciador.\n");
                break; 
            }
        }
        // **** FIM DA CORREÇÃO REFINADA DO SNPRINTF ****

        close(fd_pipe[1]); 
        if (entrada_comandos_stream != stdin && entrada_comandos_stream != NULL) {
            fclose(entrada_comandos_stream);
        }

        printf("[Processo Controle] Aguardando o Processo Gerenciador (PID OS %d) finalizar...\n", pid_filho_gerenciador);
        wait(NULL); 
        
        printf("\n===== Simulação Concluída. Processo Controle (PID OS %d) Encerrado. =====\n", getpid());
    }

    printf("[Main] Destruindo mutexes globais...\n");
    pthread_mutex_destroy(&impressao_mutex);
    pthread_mutex_destroy(&prontos_mutex);
    pthread_mutex_destroy(&bloqueados_mutex);
    pthread_mutex_destroy(&tabela_processos_mutex);
    printf("[Main] Mutexes globais destruídos.\n");

    return 0;
}