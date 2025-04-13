#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_PATH "/tmp/pipe"

int main() {
    int sockfd, len;
    struct sockaddr_un remote;
    char buffer[1024];

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        return 1;
    }

    memset(&remote, 0, sizeof(remote));
    remote.sun_family = AF_UNIX;
    strncpy(remote.sun_path, SOCK_PATH, sizeof(remote.sun_path) - 1);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);

    if (connect(sockfd, (struct sockaddr *)&remote, len) < 0) {
        perror("Erro ao conectar");
        close(sockfd);
        return 1;
    }

    // Envia comando automaticamente
    const char *cmd = "INSERT StressUser";
    write(sockfd, cmd, strlen(cmd) + 1);
    printf("Enviado: %s\n", cmd);

    // Lê resposta
    if (read(sockfd, buffer, sizeof(buffer)) > 0) {
        printf("Resposta: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}
/*
gcc client_auto.c -o client_auto
./stress_test.sh
tail -f server.log
grep "ALERTA" server.log | wc -l

*/