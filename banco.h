#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct {
    int id;
    char nome[50];
} Registro;

bool inicializar(Registro registros[], int tamanho) {
    for (int i = 0; i < tamanho; i++) {
        registros[i].id = -1;
        registros[i].nome[0] = '\0';
    }
    return true;
}

void imprimir(Registro registros[], int tamanho) {
    printf("Estado atual do banco de registros:\n");
    for (int i = 0; i < 100; i++) {
        if (registros[i].id != -1) {
            printf("ID: %d, Nome: %s\n", registros[i].id, registros[i].nome);
        }
    }
}