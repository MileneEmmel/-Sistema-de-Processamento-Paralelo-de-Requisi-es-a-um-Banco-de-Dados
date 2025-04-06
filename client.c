#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "banco.h"

#define SOCK_PATH "/tmp/unix_socket"

int main() {
    int sockfd, len;
    struct sockaddr_un remote;
    char buffer[1024];

    // Create socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Falha em criar o socket");
        return 1;
    }

    // Connect to server
    memset(&remote, 0, sizeof(remote));
    remote.sun_family = AF_UNIX;
    strncpy(remote.sun_path, SOCK_PATH, sizeof(remote.sun_path) - 1);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(sockfd, (struct sockaddr *)&remote, len) < 0) {
        perror("Falha em conectar no servidor");
        close(sockfd);
        return 1;
    }

    printf("Conectado ao servidor!\n");

    while (1) {
        // Send data to server
        printf("Operações disponíveis:\n");
        printf("  INSERT nome\n");
        printf("  DELETE nome\n");
        printf("  SAIR \n");
        printf("Digite a operação desejada: ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcasecmp(buffer, "SAIR") == 0) {
            printf("Encerrando conexão com o servidor...\n");
            break;
        }

        if (write(sockfd, buffer, strlen(buffer) + 1) < 0) {
            perror("Falha em escrever no socket");
            close(sockfd);
            return 1;
        }

        printf("Operação enviada ao servidor.\n");

        // Read data from server
        if (read(sockfd, buffer, sizeof(buffer)) < 0) {
            perror("Falha em ler do socket");
            close(sockfd);
            return 1;
        }

        printf("Resposta do servidor: %s\n\n\n", buffer);
    }
    
    // Close socket and exit
    close(sockfd);
    return 0;
}