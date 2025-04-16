#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <sys/time.h>
#include "banco.h"

#define SOCK_PATH "/tmp/pipe"
#define MAX_TAREFAS 256
#define NUM_REGISTROS 1000
#define NUM_THREADS 4
#define ARQUILO_LOG "server.log"
#define TAM_MENSAGEM 256
#define TAM_LOG 256

pthread_mutex_t mutexFila  = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  condFila   = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexLog   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBanco = PTHREAD_MUTEX_INITIALIZER;

typedef enum { 
    OP_INSERT,
    OP_DELETE,
    OP_SELECT,
    OP_UPDATE
} TipoOp;

typedef struct {
    int clientfd;
    TipoOp op;
    char mensagem[TAM_MENSAGEM];
} BlocoTarefa;

int status = 1; 
int socket_servidor_global = -1; 

int contadorID = 0;
Registro registros[NUM_REGISTROS];
BlocoTarefa filaTarefas[MAX_TAREFAS];
int contadorTarefas = 0;
FILE *arquivoLog;

void eventoLog(const char *evento) {
    pthread_mutex_lock(&mutexLog);

    struct timeval tv;
    gettimeofday(&tv, NULL);                
    time_t sec = tv.tv_sec;                  
    struct tm *tm_info = localtime(&sec);    
    char buffer[TAM_LOG];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(arquivoLog, "[%s.%03ld] %s\n", buffer, (long)(tv.tv_usec / 1000), evento);
    fflush(arquivoLog);
    pthread_mutex_unlock(&mutexLog);
}

void enfileirarTarefa(BlocoTarefa tarefa) {
    pthread_mutex_lock(&mutexFila);

    if (contadorTarefas < MAX_TAREFAS) {
        filaTarefas[contadorTarefas++] = tarefa;

        char mensagemLog[128];
        snprintf(mensagemLog, sizeof(mensagemLog), "Tarefa entrou na fila. Posição atual: %d", contadorTarefas);
        eventoLog(mensagemLog);

        pthread_cond_signal(&condFila);
    } else {
        write(tarefa.clientfd, "Fila de tarefas cheia. Tente novamente.", 40);
        eventoLog("Fila de tarefas cheia. Solicitação ignorada.");
    }

    pthread_mutex_unlock(&mutexFila);
}

void executarTarefa(const BlocoTarefa *tarefa, int thread_id) {
    char resposta[TAM_MENSAGEM] = {0};
    char log_msg[TAM_LOG];

    snprintf(log_msg, sizeof(log_msg), "[Thread %d] Iniciando tarefa: Operação %d | Dados: %.200s", thread_id, tarefa->op, tarefa->mensagem);
    eventoLog(log_msg);

    if (tarefa->op == OP_INSERT) {
        bool inserido = false;
        //usleep(300000);  // Delay para evidenciar o paralelismo (0.3 seg)
        pthread_mutex_lock(&mutexBanco);
        for (int i = 0; i < NUM_REGISTROS; i++) {
            if (registros[i].id == -1) { 
                registros[i].id = contadorID++;
                strncpy(registros[i].nome, tarefa->mensagem, sizeof(registros[i].nome)-1);
                snprintf(resposta, sizeof(resposta), "Inserção: %s => ID %d", registros[i].nome, registros[i].id);
                inserido = true;
                break;
            }
        }
        if (!inserido) {
            snprintf(resposta, sizeof(resposta), "Erro ao inserir. Banco de dados cheio.");
        }
        pthread_mutex_unlock(&mutexBanco);
    }
    else if (tarefa->op == OP_DELETE) {
        int id = atoi(tarefa->mensagem);
        bool removido = false;
        pthread_mutex_lock(&mutexBanco);
        for (int i = 0; i < NUM_REGISTROS; i++) {
            if (registros[i].id == id) {
                registros[i].id = -1;
                registros[i].nome[0] = '\0';
                snprintf(resposta, sizeof(resposta), "Remoção: ID %d", id);
                removido = true;
                break;
            }
        }
        if (!removido) {
            snprintf(resposta, sizeof(resposta), "Erro ao remover. ID %d não encontrado.", id);
        }
        pthread_mutex_unlock(&mutexBanco);
    }
    else if (tarefa->op == OP_SELECT) {
        int id = atoi(tarefa->mensagem);
        bool encontrado = false;
        pthread_mutex_lock(&mutexBanco);
        for (int i = 0; i < NUM_REGISTROS; i++) {
            if (registros[i].id == id) {
                snprintf(resposta, sizeof(resposta), "Consulta: ID %d => Nome: %s", id, registros[i].nome);
                encontrado = true;
                break;
            }
        }
        if (!encontrado) {
            snprintf(resposta, sizeof(resposta), "Erro ao consultar. ID %d não encontrado.", id);
        }
        pthread_mutex_unlock(&mutexBanco);
    }
    else if (tarefa->op == OP_UPDATE) {
        int id;
        char novoNome[100];
    
        if (sscanf(tarefa->mensagem, "%d %[^\n]", &id, novoNome) != 2) {
            snprintf(resposta, sizeof(resposta), "Formato inválido. Use: UPDATE <id> <novo_nome>");
        } else {
            bool atualizado = false;
            pthread_mutex_lock(&mutexBanco);
            for (int i = 0; i < NUM_REGISTROS; i++) {
                if (registros[i].id == id) {
                    strncpy(registros[i].nome, novoNome, sizeof(registros[i].nome)-1);
                    snprintf(resposta, sizeof(resposta), "Atualização: ID %d => Novo nome: %s", id, registros[i].nome);
                    atualizado = true;
                    break;
                }
            }
            if (!atualizado) {
                snprintf(resposta, sizeof(resposta), "Erro ao atualizar. ID %d não encontrado.", id);
            }
            pthread_mutex_unlock(&mutexBanco);
        }
    }
    
    if (write(tarefa->clientfd, resposta, strlen(resposta)+1) < 0) {
        // Se ocorrer erro na escrita, registra no log.
        eventoLog("Erro no write da resposta.");
    }
    
    snprintf(log_msg, sizeof(log_msg), "[Thread %d] Fim da tarefa executada. Resposta enviada ao cliente.", thread_id);
    eventoLog(log_msg);
    
    //imprimirRegistros(registros, NUM_REGISTROS);
}

void *th_executarTarefa(void *arg) {
    int thread_id = *((int*) arg);
    free(arg); 

    while (1) {
        pthread_mutex_lock(&mutexFila);
        while (contadorTarefas == 0) {
            pthread_cond_wait(&condFila, &mutexFila);
        }
        
        BlocoTarefa tarefa = filaTarefas[0];
        for (int i = 1; i < contadorTarefas; i++) {
            filaTarefas[i - 1] = filaTarefas[i];
        }
        contadorTarefas--;

        pthread_mutex_unlock(&mutexFila);

        executarTarefa(&tarefa, thread_id);
    }
    return NULL;
}

void* th_atenderCliente(void *arg) {
    int clientfd = *(int*)arg;
    free(arg);

    char buffer[TAM_MENSAGEM];
    while (1) {
        int bytes_lidos = read(clientfd, buffer, TAM_MENSAGEM - 1);
        if (bytes_lidos <= 0) {
            break;
        }
        buffer[bytes_lidos] = '\0';

        if (strcasecmp(buffer, "SAIR") == 0) {
            write(clientfd, "Tchau!", 7);
            eventoLog("Cliente desconectado.");
            break;
        }

        char comando[16], argumento[TAM_MENSAGEM];
        if (sscanf(buffer, "%15s %255[^\n]", comando, argumento) != 2) {
            write(clientfd, "Formato inválido.", 17);
            continue;
        }

        BlocoTarefa tarefa;
        tarefa.clientfd = clientfd;
        strncpy(tarefa.mensagem, argumento, sizeof(tarefa.mensagem)-1);

        if (strcasecmp(comando, "INSERT") == 0) {
            tarefa.op = OP_INSERT;
        } else if (strcasecmp(comando, "DELETE") == 0) {
            tarefa.op = OP_DELETE;
        } else if (strcasecmp(comando, "SELECT") == 0) { 
            tarefa.op = OP_SELECT;
        } else if (strcasecmp(comando, "UPDATE") == 0) {
            tarefa.op = OP_UPDATE;
        } else {
            write(clientfd, "Operação desconhecida.", 23);
            continue;
        }
        enfileirarTarefa(tarefa);
    }

    close(clientfd);
    return NULL;
}

void* th_comandos(void* arg) {
    char comando[100];
    
    while (fgets(comando, sizeof(comando), stdin)) {
        comando[strcspn(comando, "\n")] = '\0';
        if (strncmp(comando, "SHUTDOWN", 8) == 0) {
            eventoLog("Comando SHUTDOWN recebido. Encerrando o servidor...");
            status = 0;
            if (socket_servidor_global != -1) {
                shutdown(socket_servidor_global, SHUT_RDWR);
                close(socket_servidor_global);
                salvarRegistros(registros, NUM_REGISTROS);
                eventoLog("Servidor encerrado.");
                fclose(arquivoLog);
                unlink(SOCK_PATH);
               
                exit(0);
            }
            break;
        } else if (strncmp(comando, "PRINT", 5) == 0) {
            eventoLog("Comando PRINT recebido.");
            imprimirRegistros(registros, NUM_REGISTROS);
        }
    }
    return NULL;
}

int main(void) {
    if (!(arquivoLog = fopen(ARQUILO_LOG, "a"))) {
        exit(1);
    }

    printf("Servidor inicializando...\n");
    eventoLog("Servidor inicializando...");
    inicializarRegistros(registros, NUM_REGISTROS);
    carregarRegistros(registros, NUM_REGISTROS, &contadorID);

    // Criação do pool de threads com IDs enumerados
    pthread_t pool[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        int *thread_id = malloc(sizeof(int));
        if (thread_id == NULL) {
            exit(1);
        }
        *thread_id = i;
        pthread_create(&pool[i], NULL, th_executarTarefa, thread_id);
        pthread_detach(pool[i]);
    }

    unlink(SOCK_PATH);
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    socket_servidor_global = sockfd;


    struct sockaddr_un addr = { .sun_family = AF_UNIX };
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path)-1);
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        exit(1);
    }
    listen(sockfd, 5);
    eventoLog("Servidor pronto, aguardando clientes...");

    pthread_t thread_comandos;
    pthread_create(&thread_comandos, NULL, th_comandos, NULL);

    while (status) {
        int *pclient = malloc(sizeof(int));
        *pclient = accept(sockfd, NULL, NULL);

        if (!status || *pclient < 0) {
            free(pclient);
            break;
        }

        eventoLog("Cliente conectado.");
        pthread_t tid;
        pthread_create(&tid, NULL, th_atenderCliente, pclient);
        pthread_detach(tid);
    }
    
    pthread_join(thread_comandos, NULL);
    salvarRegistros(registros, NUM_REGISTROS);
    eventoLog("Servidor encerrado.");
    fclose(arquivoLog);
    unlink(SOCK_PATH);
    pthread_mutex_destroy(&mutexFila);
    pthread_cond_destroy(&condFila);
    pthread_mutex_destroy(&mutexLog);
    pthread_mutex_destroy(&mutexBanco);
    return 0;
}
