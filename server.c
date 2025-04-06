#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <ctype.h>
#include "banco.h"

#define SOCK_PATH "/tmp/unix_socket"

int id = 0;
Registro registros[100];

int main() {
    // Inicializa o banco
    if (inicializar(registros, 100)) {
        printf("Banco inicializado com sucesso!\n");
    } else {
        printf("Falha ao inicializar o banco.\n");
    }

    int sockfd, newsockfd, len;
    struct sockaddr_un local, remote;
    char buffer[1024];

    // Criação do socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Falha em criar o socket");
        return 1;
    }

    // Bind do socket
    memset(&local, 0, sizeof(local));
    local.sun_family = AF_UNIX;
    strncpy(local.sun_path, SOCK_PATH, sizeof(local.sun_path) - 1);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(sockfd, (struct sockaddr *)&local, len) < 0) {
        perror("Falha em capturar o socket");
        close(sockfd);
        return 1;
    }

    // Escuta por conexões
    if (listen(sockfd, 5) < 0) {
        perror("Falha em escutar o socket");
        close(sockfd);
        return 1;
    }

    printf("Servidor IPC ouvindo em %s...\n", SOCK_PATH);

    while (1) {
        // Aceita uma nova conexão de cliente
        memset(&remote, 0, sizeof(remote));
        len = sizeof(remote);
        newsockfd = accept(sockfd, (struct sockaddr *)&remote, &len);
        if (newsockfd < 0) {
            perror("Falha em aceitar conexão");
            continue;
        }

        printf("Cliente conectado!\n");

        // Loop interno para processar várias operações do mesmo cliente
        while (1) {
            memset(buffer, 0, sizeof(buffer));
            int bytes_lidos = read(newsockfd, buffer, sizeof(buffer));
            if (bytes_lidos <= 0) {
                printf("Cliente desconectado.\n");
                break;
            }

            printf("Operação recebida: %s\n", buffer);

            // Realizar operação
            if (strncmp(buffer, "INSERT ", 7) == 0) {
                char *nome = buffer + 7;
                for (int i = 0; i < 100; i++) {
                    if (registros[i].id == -1) {
                        registros[i].id = id++;
                        strcpy(registros[i].nome, nome);
                        printf("Inserção realizada: Nome: %s, ID: %d\n", nome, registros[i].id);
                        break;
                    }
                    if (i == 99) {
                        printf("Banco de dados cheio.\n");
                    }
                }
            }
            else if (strncmp(buffer, "DELETE ", 7) == 0) {
                char *nome = buffer + 7;
                for (int i = 0; i < 100; i++) {
                    if (strcmp(registros[i].nome, nome) == 0) {
                        registros[i].id = -1;
                        registros[i].nome[0] = '\0';
                        printf("Remoção realizada: %s\n", nome);
                        break;
                    }
                    if (i == 99) {
                        printf("Nome não encontrado.\n");
                    }
                }
            }
            else {
                printf("Operação não reconhecida.\n");
            }

            // Envia confirmação ao cliente
            if (write(newsockfd, buffer, strlen(buffer) + 1) < 0) {
                perror("Falha em escrever no socket");
                break;
            }

            // Print do banco atual
            printf("Estado atual do banco:\n");
            for (int i = 0; i < 100; i++) {
                if (registros[i].id != -1) {
                    printf("Nome: %s, ID: %d\n", registros[i].nome, registros[i].id);
                }
            }
        }

        close(newsockfd); // Fecha o socket do cliente
    }

    close(sockfd); // Nunca será alcançado, mas é boa prática
    return 0;
}
