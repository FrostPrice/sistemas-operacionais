#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h> // Adicionando biblioteca time.h para lidar com o tempo

// Constantes
#define NUM_THREADS 3
#define ITEMS_UPDATE_WEIGHT 1500
#define DISPLAY_SECONDS_INTERVAL 2

// Struct que define cada sensor (Thread)
typedef struct {
    int id;
    int items_count;
    double weight;
} Sensor;

// Variaveis Globais (Compartilhadas entre threads)
float WEIGHTS[NUM_THREADS] = {5.0, 2.0, 0.5}; // Pesos dos itens para cada esteira
float INTERVALS[NUM_THREADS] = {1, 0.5, 0.1};  // Intervalo em segundos para cada esteira
int totalItemsCount = 0;
double totalWeight = 0;
Sensor sensors[NUM_THREADS];
pthread_t sensors_threads[NUM_THREADS];
pthread_attr_t threads_attr;
time_t last_display_time = 0;
time_t current_time;

void* sensorThread(void* param) {
    Sensor* sensor = (Sensor*)param;

    printf("Esteira %d: Iniciando...\n", sensor->id);

    char sender_message[100];
    while (1)
    {
        sensor->items_count++;
        totalItemsCount++;
        sensor->weight += WEIGHTS[sensor->id];
        totalWeight += WEIGHTS[sensor->id];

        // Verificar se é necessário atualizar o peso total
        if (totalItemsCount % ITEMS_UPDATE_WEIGHT == 0) {
            sprintf(sender_message, "Quantidade total de Itens: %d, Peso total dos itens: %.2f Kg\n",totalItemsCount, totalWeight);
            write(1, sender_message, sizeof(sender_message));
        }

        // Fazer isso esperar 2 segundos para mandar a mensagem
        current_time = time(NULL);
        if (current_time - last_display_time >= DISPLAY_SECONDS_INTERVAL) {
            // sprintf(sender_message, "Esteira %d: %d itens, peso total: %.2f kg\n", sensor->id, sensor->items_count, sensor->weight);
            sprintf(sender_message, "Contagem total de itens: %d\n", totalItemsCount); // Exibir a contagem total de itens
            write(1, sender_message, sizeof(sender_message));
            last_display_time = current_time;
        }

        usleep(INTERVALS[sensor->id] * 1000000);
    }

    pthread_exit(0);
}

void displayProcess(int pipe_fd) {
    char reciever_message[100];
    while (1)
    {
        read(pipe_fd, reciever_message, sizeof(reciever_message));
        printf("%s", reciever_message);
    }
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        printf("WARNING: Contagem interrompida pelo operador\n");
        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_cancel(sensors_threads[i]);
        }
        exit(EXIT_SUCCESS);
    }
}

int main() {
    pthread_attr_init(&threads_attr);

    // Iniciar threads dos sensores
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        Sensor sensor;
        sensor.id = i;
        sensor.items_count = 0;
        sensor.weight = 0;
        sensors[i] = sensor;

        pthread_create(&sensors_threads[i], &threads_attr, sensorThread, (void*)&sensors[i]); 
    }

    // Inicia processo de exibição
    pid_t pid = fork();

    int pipe_fd[2];

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Processo filho
        close(pipe_fd[1]);
        displayProcess(pipe_fd[0]);
        exit(0);
    } else if (pid > 0) {
        // Processo pai
        signal(SIGINT, signalHandler);
        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(sensors_threads[i], NULL);
        }
        close(pipe_fd[0]);
    } else {
        // Fork falhou
        perror("fork");
        exit(EXIT_FAILURE);
    }

    return 0;
}
