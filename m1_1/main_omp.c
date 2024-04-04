#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h> // Usada para lidar com o tempo

// Constantes
#define NUM_THREADS 3
#define ITEMS_UPDATE_WEIGHT 1500
#define DISPLAY_SECONDS_INTERVAL 2

// Struct que define cada sensor (Thread)
typedef struct {
    int id;
    int items_count;
    double weight;
    int is_running;
    omp_lock_t lock;
} Sensor;

// Variaveis Globais (Compartilhadas entre threads)
float WEIGHTS[NUM_THREADS] = {5.0, 2.0, 0.5}; // Pesos dos itens para cada esteira
float INTERVALS[NUM_THREADS] = {1, 0.5, 0.1};  // Intervalo em segundos para cada esteira
int totalItemsCount = 0;
double totalWeight = 0.0;
Sensor sensors[NUM_THREADS];
int pipe_fd[2];
// Variáveis globais para controlar o tempo de exibição no display
time_t last_display_time = 0;
time_t current_time;

void* sensorThread(void* param) {
    Sensor* sensor = (Sensor*)param;
    char sender_message[100];

    sprintf(sender_message, "Esteira %d: Iniciando...\n", sensor->id);
    write(pipe_fd[1], sender_message, sizeof(sender_message));

    while (1)
    {
        
        if (!sensor->is_running)
            omp_set_lock(&sensor->lock);
    
        sensor->items_count++;
        sensor->weight += WEIGHTS[sensor->id];

        #pragma omp critical
        {
            totalItemsCount++;
            totalWeight += WEIGHTS[sensor->id];

            // Verificar se é necessário atualizar o peso total
            if (totalItemsCount % ITEMS_UPDATE_WEIGHT == 0) {
                sprintf(sender_message, "Quantidade total de Itens: %d, Peso total dos itens: %.2f Kg\n", totalItemsCount, totalWeight);
                write(pipe_fd[1], sender_message, sizeof(sender_message));
            }

            // Verificar se é necessário enviar a contagem para exibição
            current_time = time(NULL);
            if (current_time - last_display_time >= DISPLAY_SECONDS_INTERVAL) {
                // * Descomente o código abaixo para exibir a contagem de itens e o peso da esteira atual
                // sprintf(sender_message, "Esteira %d: %d itens, peso total: %.2f kg\n", sensor->id, sensor->items_count, sensor->weight); // Exibir a contagem de itens e o peso da esteira atual
                // write(pipe_fd[1], sender_message, sizeof(sender_message));

                sprintf(sender_message, "Contagem total de itens: %d\n", totalItemsCount); // Exibir a contagem total de itens
                write(pipe_fd[1], sender_message, sizeof(sender_message));

                last_display_time = current_time;
            }
        }

        usleep(INTERVALS[sensor->id] * 1000000); // usleep usa microssegundos
    }
}

void displayProcess() {
    char reciever_message[100];
    while (1)
    {
        read(pipe_fd[0], reciever_message, sizeof(reciever_message));
        printf("%s", reciever_message);
    }
}

void* keyboardInput(void* param) {
    int input_char = 0;

    while(1){
        input_char = getchar();
        if (input_char == 80 || input_char == 112)
        {
            char sender_message[100];
            sprintf(sender_message, "WARNING: Contagem interrompida pelo operador, Contagem total de itens: %d\n", totalItemsCount);
            write(pipe_fd[1], sender_message, sizeof(sender_message));
            for (int i = 0; i < NUM_THREADS; i++) {
                sensors[i].is_running = 0;
            }
        }
        if (input_char == 82 || input_char == 114)
        {
            char sender_message[100] = "WARNING: Contagem retomada pelo operador\n";
            write(pipe_fd[1], sender_message, sizeof(sender_message));
            for (int i = 0; i < NUM_THREADS; i++) {
                sensors[i].is_running = 1;
                omp_unset_lock(&sensors[i].lock);
            }
        }
    }
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        printf("WARNING: Contagem encerrada pelo operador\n");
        
        for (int i = 0; i < NUM_THREADS; i++) {
            sensors[i].is_running = 0;
        }
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        exit(EXIT_SUCCESS);
    }
}

int main() {
    // Inicia processo de exibição
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid == 0) {
        // Processo filho - Somente recebe mensagens 
        close(pipe_fd[1]);
        signal(SIGINT, signalHandler); // Saida de emergencia
        displayProcess();
        exit(0);
    } else if (pid > 0) {
        // Processo pai - Somente envia mensagens

        // Iniciar threads dos sensores
        #pragma omp parallel num_threads(NUM_THREADS + 1)
        {  
            int thread_id = omp_get_thread_num();
            if (thread_id < NUM_THREADS) {
                Sensor sensor;
                sensor.id = thread_id;
                sensor.items_count = 0;
                sensor.weight = 0;
                sensors[thread_id] = sensor;
                sensorThread((void*)&sensors[thread_id]);
            } else {
                keyboardInput(NULL);
            }
        }

        close(pipe_fd[0]);
        close(pipe_fd[1]);
    } else {
        // Fork falhou
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Aguarda o processo filho encerrar
    wait(NULL);

    return 0;
}