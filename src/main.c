/*int main(int argc, char *argv[]) {    
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
    ProcessoSimulado_t *processo_inicial = criarNovoProcesso();//Eu acho que deveria ser psCriarNovoProcesso()
    adicionarProcesso(&gerenciadorProcessos, processo_inicial);


    // Acho que deveria usar psCarregarProgramaDeArquivo()
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
    
    psCarregarProgramaDeArquivo(processo_inicial, nomeArquivoInst);
    
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
    
    if (pid == 0) {
        // Filho: processo gerenciador
        close(fd[1]); // Fecha extremidade de escrita
        gerenciadorProcessosSimulados(fd[0], processo_inicial); // Chama função do gerenciador, lendo de fd[0]
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
}*/
/*#include "../include/config.h"
#include "../include/gerenciador.h"
#include "../include/processos.h"
#include "../include/cpu.h"
#include "../include/estados.h"
#include "../include/fila.h"
#include "../include/processoImpressao.h"
#include "../include/processoSimulado.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    // Apresentação inicial
    printf("\n===== Simulador de Gerenciamento de Processos =====\n\n");
    
    // 1. Inicialização
    int fd[2];
    pid_t pid;
    
    // Cria o pipe
    if (pipe(fd) < 0) {
        perror("Erro ao criar pipe");
        exit(EXIT_FAILURE);
    }
    
    // 2. Interação com o Usuário
    FILE *entrada = NULL;
    char origem;
    
    // Pergunta pelo arquivo de instruções
    char nomeArquivoInst[128];
    printf("Digite o nome do arquivo que contém as instruções (ou Enter para usar 'init.txt'): ");
    fgets(nomeArquivoInst, sizeof(nomeArquivoInst), stdin);
    nomeArquivoInst[strcspn(nomeArquivoInst, "\n")] = '\0';
    
    if (strlen(nomeArquivoInst) == 0) {
        strcpy(nomeArquivoInst, "init.txt");
        printf("Usando arquivo padrão: %s\n", nomeArquivoInst);
    }
    
    // Verifica se o arquivo existe
    FILE *arq_entrada = fopen(nomeArquivoInst, "r");
    if (!arq_entrada) {
        perror("Erro ao abrir arquivo de instruções");
        close(fd[1]);
        exit(EXIT_FAILURE);
    }
    fclose(arq_entrada);
    
    // Cria o primeiro processo
    ProcessoSimulado_t *processo_inicial = psCriarNovo(0, -1, 0, 0);
    if (!processo_inicial) {
        perror("Erro ao criar processo inicial");
        exit(EXIT_FAILURE);
    }
    
    // Carrega o programa para o processo inicial
    psCarregarProgramaDeArquivo(processo_inicial, nomeArquivoInst);
    printf("Programa carregado com sucesso: %d instruções\n", processo_inicial->listaInstrucoes.tamanho);
    
    // Pergunta pela fonte de comandos
    printf("\nDeseja ler comandos do teclado (T) ou de um arquivo (F)? ");
    scanf(" %c", &origem);
    getchar(); // Consome '\n'
    
    if (origem == 'F' || origem == 'f') {
        char nomeArquivo[128];
        printf("Digite o nome do arquivo de comandos (ou Enter para usar 'comandos.txt'): ");
        fgets(nomeArquivo, sizeof(nomeArquivo), stdin);
        nomeArquivo[strcspn(nomeArquivo, "\n")] = '\0';
        
        if (strlen(nomeArquivo) == 0) {
            strcpy(nomeArquivo, "comandos.txt");
            printf("Usando arquivo de comandos padrão: %s\n", nomeArquivo);
        }
        
        entrada = fopen(nomeArquivo, "r");
        if (!entrada) {
            perror("Erro ao abrir arquivo de comandos");
            close(fd[1]);
            exit(EXIT_FAILURE);
        }
        printf("Lendo comandos do arquivo: %s\n", nomeArquivo);
    } else {
        entrada = stdin;
        printf("\nDigite os comandos (U, I, M), um por linha. Ctrl+D para encerrar:\n");
    }
    
    // 3. Criação do Processo Gerenciador
    printf("\nIniciando simulação...\n");
    pid = fork();
    
    if (pid < 0) {
        perror("Erro ao criar processo gerenciador");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        // Processo filho (gerenciador)
        printf("[Processo Gerenciador] Iniciado\n");
        close(fd[1]); // Fecha extremidade de escrita
        gerenciadorProcessosSimulados(fd[0], processo_inicial);
        close(fd[0]);
        printf("[Processo Gerenciador] Finalizado\n");
        exit(EXIT_SUCCESS);
    } else {
        // 4. Envio de Comandos (Processo Controle)
        printf("[Processo Controle] Iniciado. Enviando comandos para o Gerenciador...\n");
        close(fd[0]); // Fecha extremidade de leitura
        
        char buffer[MAX_CMD_LEN];
        // Leitura e envio de comandos para o gerenciador
        while (fgets(buffer, sizeof(buffer), entrada)) {
            // Remove lixo e valida comando
            buffer[strcspn(buffer, "\r\n")] = 0;
            
            if (strlen(buffer) == 0) continue;
            
            if (buffer[0] != 'U' && buffer[0] != 'I' && buffer[0] != 'M') {
                printf("[Processo Controle] Comando inválido: %s\n", buffer);
                continue;
            }
            
            printf("[Processo Controle] Enviando comando: %c\n", buffer[0]);
            strcat(buffer, "\n"); // Garante quebra de linha
            
            if (write(fd[1], buffer, strlen(buffer)) < 0) {
                perror("Erro ao escrever no pipe");
                break;
            }
            
            if (buffer[0] == 'M') {
                printf("\n[Processo Controle] Comando M recebido. Finalizando simulação...\n");
                break;
            }
        }
        
        // 5. Finalização
        printf("\n[Processo Controle] Finalizando e aguardando término do Gerenciador...\n");
        close(fd[1]);
        if (entrada != stdin) fclose(entrada);
        wait(NULL); // Espera gerenciador encerrar
        printf("\n===== Simulação concluída com sucesso =====\n");
    }
    
    return 0;
}*/


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

int main(int argc, char *argv[]) {
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

    // Verifica brevemente se o arquivo de instruções é acessível (psCarregarProgramaDeArquivo fará a abertura real)
    FILE *teste_arq_inst = fopen(nomeArquivoInst, "r");
    if (!teste_arq_inst) {
        perror("Erro ao tentar acessar arquivo de instruções iniciais");
        // Fechar descritores do pipe em caso de falha antes do fork
        close(fd[0]);
        close(fd[1]);
        exit(EXIT_FAILURE);
    }
    fclose(teste_arq_inst);

    // Cria a estrutura para o primeiro processo simulado (PID 0)
    // psCriarNovo aloca memória para ProcessoSimulado_t
    ProcessoSimulado_t *processo_inicial = psCriarNovo(0, -1, 0, 0L); // PID 0, Pai -1, Prio 0, Chegada 0
    if (!processo_inicial) {
        // psCriarNovo já trata erro de malloc, mas uma checagem aqui é boa prática.
        fprintf(stderr, "Erro crítico: Falha ao alocar memória para o processo inicial.\n");
        exit(EXIT_FAILURE);
    }

    // Carrega o programa (lista de instruções) para o processo inicial
    // psCarregarProgramaDeArquivo aloca memória para as instruções.
    psCarregarProgramaDeArquivo(processo_inicial, nomeArquivoInst);
    if (processo_inicial->listaInstrucoes.tamanho == 0 && processo_inicial->estado_atual == EST_TERMINADO) {
        fprintf(stderr, "Falha ao carregar instruções do arquivo '%s' para o processo inicial. O arquivo pode estar vazio, ser inválido ou não encontrado.\n", nomeArquivoInst);
        psLiberarMemoria(processo_inicial); // Libera a memória alocada por psCriarNovo
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
            strcpy(nomeArquivoCmd, "comandos.txt"); // Arquivo de comandos padrão
            printf("Usando arquivo de comandos padrão: %s\n", nomeArquivoCmd);
        }

        entrada_comandos = fopen(nomeArquivoCmd, "r");
        if (!entrada_comandos) {
            perror("Erro ao abrir arquivo de comandos");
            psLiberarMemoria(processo_inicial); // Libera processo inicial antes de sair
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
        // printf("[DEBUG Gerenciador PID %d] Iniciado.\n", getpid());
        close(fd[1]); // Gerenciador não escreve neste pipe, apenas lê.
        
        // O gerenciador assume a responsabilidade pela memória de 'processo_inicial' a partir daqui.
        gerenciadorProcessosSimulados(fd[0], processo_inicial);
        
        close(fd[0]); // Fecha a extremidade de leitura após o uso.
        // A liberação da memória de 'processo_inicial' e de outros processos
        // deve ser feita pelo gerenciador ao final de sua execução ou quando os processos terminam.
        // Se 'processo_inicial' foi copiado para a tabela interna do gerenciador e a lista de instruções
        // também foi copiada profundamente, o gerenciador pode liberar sua cópia.
        // No modelo atual, 'gerenciador.tabela_de_processos[0] = *processo_inicial;' faz uma cópia da estrutura,
        // mas a lista de instruções (ponteiros) é compartilhada. O gerenciador deve chamar
        // psLiberarMemoria para os processos em sua tabela.
        printf("[Processo Gerenciador PID %d] Finalizado.\n", getpid());
        exit(EXIT_SUCCESS);
    } else {
        // PROCESSO PAI: Controle
        // printf("[DEBUG Controle PID %d] Iniciado. Gerenciador criado com PID %d.\n", getpid(), pid);
        close(fd[0]); // Controle não lê deste pipe, apenas escreve.

        char buffer_comando[MAX_CMD_LEN];
        printf("[Processo Controle] Enviando comandos para o Gerenciador...\n");

        while (fgets(buffer_comando, sizeof(buffer_comando), entrada_comandos)) {
            buffer_comando[strcspn(buffer_comando, "\r\n")] = 0; // Remove newline/CR

            if (strlen(buffer_comando) == 0) continue; // Ignora linhas vazias

            // Valida o comando
            if (buffer_comando[0] != 'U' && buffer_comando[0] != 'I' && buffer_comando[0] != 'M') {
                printf("[Processo Controle] Comando inválido ignorado: '%s'\n", buffer_comando);
                continue;
            }

            // Adiciona newline para garantir que fgets no gerenciador leia uma linha completa
            char comando_com_newline[MAX_CMD_LEN + 2]; // +1 para newline, +1 para null terminator
            snprintf(comando_com_newline, sizeof(comando_com_newline), "%s\n", buffer_comando);
            
            // printf("[Processo Controle] Enviando: %s", comando_com_newline); // Debug
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
        wait(NULL); // Espera o processo gerenciador (filho) terminar.
        
        // O processo pai (Controle) NÃO deve liberar 'processo_inicial' aqui,
        // pois o ponteiro foi passado para o filho (Gerenciador), que se torna
        // responsável por gerenciar e liberar essa memória.
        // psLiberarMemoria(processo_inicial); // NÃO FAZER AQUI!

        printf("\n===== Simulação Concluída. Processo Controle Encerrado. =====\n");
    }

    return 0;
}

/*O QUE TEMOS QUE FAZER (AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA NAO AGUENTO MAIS)
// FAVOR ESCREVER O QUE FEZ, SE NÃO A GENTE VAI FICAR 2 HORAS LENDO A BOSTA DO CODIGO
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
********************************************************************************************
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
*********************************************************************************************
Gerenciador de Processos,
3.1 Corrigir em include/gerenciador.h e src/gerenciador.c

    Corrigir a função criarProcessoSimulado() (remover comentário "TESTAR E CONFERIR")
    Corrigir a função substituirImagemProcesso() (remover comentário "TESTAR E CONFERIR")
    Completar a função gerenciarTransicoesEstados()
    Corrigir a função escalonarProcessos()
    Corrigir e testar a função escalonarProcessosFIFO()
    Corrigir e testar as funcoes estadosProntosFIFO()
    Corrigir a função trocarContexto() (remover comentário "TESTAR E CONFERIR")
**********************************************************************************************
3.2 Implementar a política de escalonamento (Falta testar) Linha 135 no gerenciador.c

    Implementar a política de múltiplas filas com classes de prioridade
    Implementar a lógica de quantum por nível de prioridade
    Implementar a lógica de aumento/diminuição de prioridade
    Implementar uma política de escalonamento adicional (requisito do grupo)
************************************************************************************************
Processo Controle e Gerenciador,
4.1 Completar em src/main.c

    Implementar a função carregarPrograma() (descomentar chamada)
    Implementar a função gerenciador() (descomentar chamada)
    Garantir comunicação correta via pipe entre processo controle e gerenciador
    Implementar tratamento dos comandos U, I e M no gerenciador

Processo de Impressão, (Falta testar)
5.1 Implementar em novo arquivo src/impressao.c e include/impressao.h

    Criar estrutura para o processo de impressão(Ok)
    Implementar função para imprimir o estado atual do sistema(Ok)
    Implementar função para imprimir estatísticas finais(Ok)
    Implementar diferentes configurações de impressão (simplificada/detalhada) (Não é 100% necessário, mas talvez seja legal fazer depois?)

Arquivos de Teste,
6.1 Criar arquivos de teste

    Criar arquivo init para o primeiro processo simulado
    Criar arquivos para teste da instrução R (ex: file_a, file_b, etc.)
    Criar arquivo de comandos para teste automatizado

*/