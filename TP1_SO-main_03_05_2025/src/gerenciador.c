#include "../include/config.h"
#include "../include/gerenciador.h"
#include "../include/processo_simulado.h"

void gerenciador(int fd_read, ProcessoSimulado *processo) {
    //printf("\n processos %d \n", processos);
    char comando[MAX_CMD_LEN];

    FILE *pipe_in = fdopen(fd_read, "r");
    if (!pipe_in) {
        perror("fdopen");
        exit(EXIT_FAILURE);
    }

    printf("=== Gerenciador de Processos Iniciado ===\n");
    printf("Aguardando comandos (U, I, M) do processo controle...\n\n");

    while (fgets(comando, sizeof(comando), pipe_in)) {
        //printf("\n\n\n entrou em while gerenciador \n\n\n");
        comando[strcspn(comando, "\r\n")] = '\0';
        if (strlen(comando) == 0)
            continue;

        printf("[Gerenciador] Comando recebido: '%s'\n", comando);
        int i;

        switch (comando[0]) {
            case 'U':
                printf("[Gerenciador] U → fim de unidade de tempo. Executando próxima instrução, incrementando contador e escalonando.\n");
                
                executar_instrucao(processo);
                
                break;

            case 'I':
                printf("[Gerenciador] I → solicitada impressão do estado atual. Disparando processo impressão...\n");

                
                
                break;

            case 'M':
                printf("[Gerenciador] M → impressão final e encerramento do simulador.\n");

                printf("=== Simulador encerrado com sucesso ===\n");

                fclose(pipe_in);

                return;

            default:
                printf("[Gerenciador] Comando inválido: '%s'\n", comando);
                break;
        }

        if (comando[0] == 'M'){
            break;
        }
            

    }

    fclose(pipe_in);
}

