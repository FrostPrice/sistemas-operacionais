#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
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
} Sensor;

// Variaveis Globais (Compartilhadas entre threads)
float WEIGHTS[NUM_THREADS] = {5.0, 2.0, 0.5}; // Pesos dos itens para cada esteira
float INTERVALS[NUM_THREADS] = {1, 0.5, 0.1};  // Intervalo em segundos para cada esteira
int totalItemsCount = 0;
double totalWeight = 0.0;
Sensor sensors[NUM_THREADS];
pthread_t sensors_threads[NUM_THREADS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_attr_t threads_attr;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int pipe_fd[2];
// Variáveis globais para controlar o tempo de exibição no display
time_t last_display_time = 0;
time_t current_time;
char sender_message[100]; // Para enviar mensagens entre processos, usa variavel global já que multiplas threads podem escrever ao mesmo tempo

void* sensorThread(void* param) {
    Sensor* sensor = (Sensor*)param;

    pthread_mutex_lock(&(mutex));
    sprintf(sender_message, "Esteira %d: Iniciando...\n", sensor->id);
    write(pipe_fd[1], sender_message, sizeof(sender_message));
    pthread_mutex_unlock(&(mutex));

    while (1)
    {
        pthread_mutex_lock(&(mutex));
        if (!sensor->is_running)
            pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&(mutex));

        sensor->items_count++;
        sensor->weight += WEIGHTS[sensor->id];

        pthread_mutex_lock(&(mutex));
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

            snprintf(sender_message, sizeof(sender_message),"Contagem total de itens: %d\n", totalItemsCount); // Exibir a contagem total de itens
            write(pipe_fd[1], sender_message, sizeof(sender_message));

            last_display_time = current_time;
        }
        pthread_mutex_unlock(&(mutex));


        usleep(INTERVALS[sensor->id] * 1000000); // usleep usa microssegundos
    }

    pthread_exit(0);
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
            }
            pthread_mutex_lock(&mutex);
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
        }
    }
    pthread_exit(0);
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        printf("WARNING: Contagem encerrada pelo operador\n");

        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_cancel(sensors_threads[i]);
        }

        close(pipe_fd[0]);
        close(pipe_fd[1]);
        exit(EXIT_SUCCESS);
    }
}

int main() {
    pthread_mutex_init(&(mutex), NULL);
    pthread_attr_init(&threads_attr);

    // Iniciar threads dos sensores e inicializar mutexes
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
        exit(EXIT_SUCCESS);
    } else if (pid > 0) {
        // Processo pai - Somente envia mensagens
        close(pipe_fd[0]);

        // Cria thread do teclado
        pthread_t keyboardInputThread;
        pthread_create(&keyboardInputThread, &threads_attr, keyboardInput, NULL);
        pthread_join(keyboardInputThread, NULL);

        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(sensors_threads[i], NULL);
        }
    } else {
        // Fork falhou
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Aguarda o processo filho encerrar
    wait(NULL);

    // Destruir mutex
    pthread_mutex_destroy(&(mutex));

    return 0;
}