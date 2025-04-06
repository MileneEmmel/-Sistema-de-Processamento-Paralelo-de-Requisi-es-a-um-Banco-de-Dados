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