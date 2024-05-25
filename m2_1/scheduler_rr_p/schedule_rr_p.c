#include "schedule_rr_p.h"
#include "list.h"
#include "task.h"
#include "CPU.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define TIME_SLICE 10 // Slice de tempo máximo

// Estrutura da fila de aptos para diferentes prioridades
struct node *readyQueue[MAX_PRIORITY]; // Array de listas encadeadas, uma para cada prioridade

// Variável global para simulação do tempo
int currentTime = 0;
pthread_mutex_t timeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t timeCond = PTHREAD_COND_INITIALIZER;
int timeFlag = 0; // Flag de estouro do tempo

// Função para adicionar uma task à fila de aptos correspondente
void add(char *name, int priority, int burst) {
    Task *newTask = malloc(sizeof(Task));
    newTask->name = strdup(name); // Usar strdup para duplicar a string
    newTask->priority = priority;
    newTask->burst = burst;

    // Corrigindo o índice da fila
    int queueIndex = priority - 1;
    insert_end(&readyQueue[queueIndex], newTask);
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

// Função para escolher a próxima task a ser executada
Task *pickNextTask() {
    for (int i = 0; i < MAX_PRIORITY; i++) {
        if (readyQueue[i] != NULL) {
            struct node *temp = readyQueue[i];
            readyQueue[i] = readyQueue[i]->next;
            Task *task = temp->task;
            free(temp); // Liberar o nó da lista
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
        Task *task = pickNextTask();
        if (task != NULL) {
            int timeToRun = (task->burst < TIME_SLICE) ? task->burst : TIME_SLICE;
            
            pthread_mutex_lock(&timeMutex);
            while (timeFlag == 0) {
                pthread_cond_wait(&timeCond, &timeMutex);
            }
            timeFlag = 0;
            pthread_mutex_unlock(&timeMutex);

            run(task, timeToRun); // Executa a task por 1 unidade de tempo
            task->burst -= timeToRun;

            if (task->burst == 0) {
                printf("Task %s completed.\n", task->name);
                free(task->name); // Liberar a string duplicada
                free(task); // Liberar a estrutura da tarefa
                continue;
            }
            
            if (task->burst > 0) {
                // Re-adiciona a tarefa de volta à fila de aptos no fim da list (evitando starvation)
                int queueIndex = task->priority - 1;
                insert_end(&readyQueue[queueIndex], task);
            }
        } else {
            printf("No tasks available. Waiting...\n");
            sleep(1); // Espera um tempo antes de tentar novamente
            break;
        }
    }
}

