#include "../include/processoControle.h"
#include "../include/processos.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void processo_controle(int fd_read) {
    char comando[MAX_CMD_LEN];
    FILE *pipe_in = fdopen(fd_read, "r");
    
    if (!pipe_in) {
        perror("fdopen");
        exit(EXIT_FAILURE);
    }
    
    printf("=== Processo Controle Iniciado ===\n");
    printf("Aguardando comandos (U, I, M) do usuário...\n\n");
    
    while (fgets(comando, sizeof(comando), pipe_in)) {
        comando[strcspn(comando, "\r\n")] = '\0';
        if (strlen(comando) == 0) continue;
        
        printf("[Controle] Comando recebido: '%s'\n", comando);
        
        switch (comando[0]) {
            case 'U':
                printf("[Gerenciador] U → fim de unidade de tempo. Executando próxima instrução, incrementando contador e escalonando.\n");

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
    }
    fclose(pipe_in);
}

