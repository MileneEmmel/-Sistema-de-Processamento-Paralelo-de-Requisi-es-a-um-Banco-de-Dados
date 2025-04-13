#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct {
    int id;
    char nome[50];
} Registro;

void carregarRegistros(Registro registros[], int tam, int *contadorID) {
    FILE *arquivo = fopen("registros.txt", "r");
    if (!arquivo) {
        printf("Erro ao abrir o arquivo para leitura.\n");
        return;
    }

    int i = 0;
    while (fscanf(arquivo, "%d %[^\n]", &registros[i].id, registros[i].nome) == 2 && i < tam) {
        if (registros[i].id == 0) {
            break;
        } else {
            if (*contadorID < registros[i].id) {
                *contadorID = registros[i].id;
            } 
        }
    }
    (*contadorID)++;
    fclose(arquivo);
}

void salvarRegistros(Registro registros[], int tam) {
    FILE *arquivo = fopen("registros.txt", "w");
    if (!arquivo) {
        printf("Erro ao abrir o arquivo para escrita.\n");
        return;
    }

    for (int i = 0; i < tam; i++) {
        if (registros[i].id != -1) { 
            fprintf(arquivo, "%d %s\n", registros[i].id, registros[i].nome);
        }
    }

    fclose(arquivo);
}

bool inicializarRegistros(Registro registros[], int tam) {
    for (int i = 0; i < tam; i++) {
        registros[i].id = -1;
        registros[i].nome[0] = '\0';
    }
    return true;
}

void imprimirRegistros(Registro registros[], int tam) {
    printf("Estado atual do banco de registros:\n");
    for (int i = 0; i < tam; i++) {
        if (registros[i].id != -1) {
            printf("ID: %d, Nome: %s\n", registros[i].id, registros[i].nome);
        }
    }
}
