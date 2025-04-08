#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include "banco.h"

#define SOCK_PATH "/tmp/unix_socket"

int id = 0;
Registro registros[100];

void *atender_cliente(void *arg) {
    int clientfd = *((int *)arg);
    free(arg);
    char buffer[1024];

    while (1) {
        ssize_t bytes = read(clientfd, buffer, sizeof(buffer));
        buffer[bytes] = '\0';
        if (bytes <= 0) {
            printf("Cliente desconectado ou erro na leitura...\n");
            close(clientfd);
            pthread_exit(NULL);
        }

        printf("Operação recebida: %s\n", buffer);

        if (strncmp(buffer, "INSERT ", 7) == 0) {
            char *nome = buffer + 7;
            for (int i = 0; i < 100; i++) {
                if (registros[i].id == -1) {
                    registros[i].id = id++;
                    strcpy(registros[i].nome, nome);
                    printf("Inserção realizada: Nome: %s, ID: %d\n", nome, registros[i].id);
                    strcpy(buffer, "Registro inserido com sucesso...");
                    break;
                }
                if (i == 99) {
                    printf("Banco de registros cheio...\n");
                    strcpy(buffer, "Banco de registros cheio...");
                }
            }
        }
        else if (strncmp(buffer, "DELETE ", 7) == 0) {
            char *nome = buffer + 7;
            for (int i = 0; i < 100; i++) {
                if (strcmp(registros[i].nome, nome) == 0) {
                    registros[i].id = -1;
                    registros[i].nome[0] = '\0';
                    printf("Remoção realizada com sucesso... %s\n", nome);
                    strcpy(buffer, "Remoção realizada com sucesso...");
                    break;
                }
                if (i == 99) {
                    printf("Nome não encontrado...\n");
                    strcpy(buffer, "Nome não encontrado...");
                }
            }
        }
        else {
            printf("Operação não reconhecida...\n");
            strcpy(buffer, "Operação não reconhecida...");
        }

        if (write(clientfd, buffer, strlen(buffer) + 1) < 0) {
            perror("Erro ao enviar resposta...");
            close(clientfd);
            pthread_exit(NULL);
        }

        printf("Resposta enviada ao cliente...\n");

        imprimir(registros, 100);
    }
}

int main() {
    // Inicializa banco
    if (inicializar(registros, 100)) {
        printf("Banco inicializado com sucesso...\n");
    }

    int sockfd, len;
    struct sockaddr_un local, remote;

    // Create socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket...");
        return 1;
    }

    // Bind socket to local address
    memset(&local, 0, sizeof(local));
    local.sun_family = AF_UNIX;
    strncpy(local.sun_path, SOCK_PATH, sizeof(local.sun_path) - 1);
    unlink(SOCK_PATH);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(sockfd, (struct sockaddr *)&local, len) < 0) {
        perror("Erro ao associar socket...");
        close(sockfd);
        return 1;
    }

    // Listen for connections
    if (listen(sockfd, 5) < 0) {
        perror("Erro ao escutar...");
        close(sockfd);
        return 1;
    }

    printf("Servidor ouvindo em %s...\n", SOCK_PATH);

    while (1) {
        int client_len = sizeof(remote);
        int *newsockfd = malloc(sizeof(int));
        *newsockfd = accept(sockfd, (struct sockaddr *)&remote, (socklen_t *)&client_len);
        if (*newsockfd < 0) {
            perror("Erro ao aceitar conexão...");
            free(newsockfd);
            continue;
        }

        printf("Novo cliente conectado...\n");

        pthread_t tid;
        pthread_create(&tid, NULL, atender_cliente, newsockfd);
        pthread_detach(tid);
    }

    close(sockfd);
    return 0;
}
