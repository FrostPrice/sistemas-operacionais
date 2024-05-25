#include "schedule_edf.h"
#include "list.h"
#include "task.h"
#include "CPU.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define TIME_SLICE 10 // Slice de tempo máximo

// Estrutura da fila de aptos como uma lista de listas de prioridades
struct node *readyQueue[MAX_PRIORITY] = {NULL};

// Variável global para simulação do tempo
int currentTime = 0;
pthread_mutex_t timeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t timeCond = PTHREAD_COND_INITIALIZER;
int timeFlag = 0; // Flag de estouro do tempo

// Função para calcular a folga de uma task
int calculateSlackTime(Task *task) {
    int tempo_limite = task->arrival_time + task->deadline;
    int Tedf = currentTime + task->burst;
    return tempo_limite - Tedf;
}

// Função para adicionar uma task à fila de aptos
void add(char *name, int priority, int burst, int deadline) {
    Task *newTask = malloc(sizeof(Task));
    newTask->name = strdup(name); // Usar strdup para duplicar a string
    newTask->priority = priority;
    newTask->burst = burst;
    newTask->deadline = deadline;
    newTask->arrival_time = currentTime; // Tempo de chegada da tarefa

    int slack = calculateSlackTime(newTask);
    int priority_level = (slack > 0) ? newTask->priority - 1 : 0;
    
    insert_end(&readyQueue[priority_level], newTask);
}

// Função de simulação do timer
void *timerThread(void *arg) {
    while (1) {
        sleep(1); // Simula 1 unidade de tempo
        pthread_mutex_lock(&timeMutex);
        currentTime += TIME_SLICE;
        timeFlag = 1; // Sinaliza estouro do tempo
        pthread_cond_signal(&timeCond);
        pthread_mutex_unlock(&timeMutex);
    }
}

// Função para reordenar a readyQueue de acordo com os deadlines
void reorderReadyQueue() {
    struct node *newReadyQueue[MAX_PRIORITY] = {NULL};

    for (int i = 0; i < MAX_PRIORITY; i++) {
        while (readyQueue[i] != NULL) {
            struct node *temp = readyQueue[i];
            readyQueue[i] = readyQueue[i]->next;

            int slack = calculateSlackTime(temp->task);
            int priority_level = (slack > 0) ? temp->task->priority - 1 : 0;
            temp->task->priority = priority_level + 1; // Define a nova prioridade no caso de reordenação
            insert(&newReadyQueue[priority_level], temp->task);
            free(temp);
        }
    }

    for (int i = 0; i < MAX_PRIORITY; i++) {
        readyQueue[i] = newReadyQueue[i];
    }
}

// Função para reordenar a fila de aptos periodicamente
void *reorderThread(void *arg) {
    printf("EDF: Reordering ready queues...\n");
    pthread_mutex_lock(&timeMutex);
    reorderReadyQueue();
    pthread_mutex_unlock(&timeMutex);
    return NULL;
}

// Função para escolher a próxima task a ser executada
Task *pickNextTask() {
    pthread_t reorder;
    pthread_create(&reorder, NULL, reorderThread, NULL);
    pthread_join(reorder, NULL);

    for (int i = 0; i < MAX_PRIORITY; i++) {
        if (readyQueue[i] != NULL) {
            struct node *temp = readyQueue[i];
            readyQueue[i] = temp->next;
            Task *task = temp->task;
            free(temp);
            return task;
        }
    }
    return NULL;
}

// Função principal de escalonamento
void schedule() {
    pthread_t timer;
    pthread_create(&timer, NULL, timerThread, NULL);

    while (1) {
        // Sincroniza com o temporizador
        pthread_mutex_lock(&timeMutex);
        while (timeFlag == 0) {
            pthread_cond_wait(&timeCond, &timeMutex);
        }
        timeFlag = 0;
        pthread_mutex_unlock(&timeMutex);

        Task *task = pickNextTask();
        if (task != NULL) {
            int timeToRun = (task->burst < TIME_SLICE) ? task->burst : TIME_SLICE;
            run(task, timeToRun);
            task->burst -= timeToRun;

            int tempo_limite = task->arrival_time + task->deadline;
            // Verifica se a tarefa perdeu o prazo antes de incrementar currentTime
            if (currentTime > tempo_limite) {
                printf("EDF WARNING: Deadline missed for task %s.\n", task->name);
            }

            if (task->burst > 0) {
                int slack = calculateSlackTime(task);
                int priority_level = (slack > 0) ? task->priority - 1 : 0;
                insert(&readyQueue[priority_level], task);
            } else {
                printf("Task %s completed.\n", task->name);
                free(task->name); // Liberar a string duplicada
                free(task); // Liberar a estrutura da tarefa
            }
        } else {
            printf("No tasks available. Waiting...\n");
            sleep(1); // Espera um tempo antes de tentar novamente
        }
    }
}
