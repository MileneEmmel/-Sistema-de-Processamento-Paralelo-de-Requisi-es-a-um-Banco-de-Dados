#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void* func(void* arg) {
    sleep(60);
    return NULL;
}

int main() {
    int i = 0;
    pthread_t t;
    while (1) {
        if (pthread_create(&t, NULL, func, NULL)) {
            printf("Falhou ao criar a thread %d\n", i);
            break;
        }
        i++;
    }
    printf("Total criado: %d threads\n", i);
    sleep(60); // pra manter vivo e testar no htop
    return 0;
}
