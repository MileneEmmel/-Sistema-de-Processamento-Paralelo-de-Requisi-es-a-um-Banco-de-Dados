#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_PATH "/tmp/pipe"

int main() {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return 1; }

    struct sockaddr_un remote = { .sun_family = AF_UNIX };
    strncpy(remote.sun_path, SOCK_PATH, sizeof(remote.sun_path)-1);
    if (connect(sockfd, (struct sockaddr *)&remote, sizeof(remote)) < 0) {
        perror("connect"); return 1;
    }

    printf("Conectado ao servidor.\n");

    char buffer[256];
    char msg[300];

    while (1) {
        printf("1-Inserir  2-Remover  3-Consultar  4-Atualizar  5-Sair > ");
        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        buffer[strcspn(buffer, "\n")] = '\0';
        int op = atoi(buffer);

        switch (op) {
            case 1: {
                printf("Nome: ");
                if (!fgets(buffer, sizeof(buffer), stdin)) break;
                buffer[strcspn(buffer, "\n")] = '\0';
                snprintf(msg, sizeof(msg), "INSERT %s", buffer);
                break;
            }
            case 2: {
                printf("ID a remover: ");
                if (!fgets(buffer, sizeof(buffer), stdin)) break;
                buffer[strcspn(buffer, "\n")] = '\0';
                snprintf(msg, sizeof(msg), "DELETE %s", buffer);
                break;
            }
            case 3: {
                printf("ID a consultar: ");
                if (!fgets(buffer, sizeof(buffer), stdin)) break;
                buffer[strcspn(buffer, "\n")] = '\0';
                snprintf(msg, sizeof(msg), "SELECT %s", buffer);
                break;
            }
            case 4: {
                char id[32], novoNome[200];
                printf("ID a atualizar: ");
                if (!fgets(id, sizeof(id), stdin)) break;
                id[strcspn(id, "\n")] = '\0';

                printf("Novo nome: ");
                if (!fgets(novoNome, sizeof(novoNome), stdin)) break;
                novoNome[strcspn(novoNome, "\n")] = '\0';

                snprintf(msg, sizeof(msg), "UPDATE %s %s", id, novoNome);
                break;
            }
            case 5: {
                write(sockfd, "SAIR", 5);
                if (read(sockfd, buffer, sizeof(buffer)) > 0)
                    printf("%s\n", buffer);
                close(sockfd);
                return 0;
            }
            default: {
                printf("Opção inválida.\n");
                continue;
            }
        }

        write(sockfd, msg, strlen(msg)+1);
        if (read(sockfd, buffer, sizeof(buffer))>0)
            printf("Resposta: %s\n", buffer);
    }
    close(sockfd);
    return 0;
}