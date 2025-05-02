#include "config.h"
#include "gerenciador.h"

void gerenciador(int fd_read) {

    char comando[MAX_CMD_LEN];
    
    FILE *pipe_in = fdopen(fd_read, "r");
    if (!pipe_in) {
        perror("fdopen");
        exit(EXIT_FAILURE);
    }

    printf("=== Gerenciador de Processos Iniciado ===\n");
    printf("Aguardando comandos (U, I, M) do processo controle...\n\n");

    while (fgets(comando, sizeof(comando), pipe_in)) {
        comando[strcspn(comando, "\r\n")] = '\0';
        if (strlen(comando) == 0)
            continue;

        printf("[Gerenciador] Comando recebido: '%s'\n", comando);

        switch (comando[0]) {
            case 'U':
                printf("[Gerenciador] U → fim de unidade de tempo. Executando próxima instrução, incrementando contador e escalonando.\n");
                // comando_U(&sim);
                break;

            case 'I':
                printf("[Gerenciador] I → solicitada impressão do estado atual. Disparando processo impressão...\n");
                // comando_I(&sim);
                break;

            case 'M':
                printf("[Gerenciador] M → impressão final e encerramento do simulador.\n");
                printf("=== Simulação Encerrada ===\n");
                // comando_M(&sim);
                break;

            default:
                printf("[Gerenciador] Comando inválido: '%s'\n", comando);
                break;
        }

        if (comando[0] == 'M')
            break;

        printf("\n");
    }

    fclose(pipe_in);
}
