/* Faça uma aplicação que tenha um vetor de 10 valores, gerados randomicamente
ou com entrada do usuário. Com o vetor preenchido, eles devem gerar uma soma e 
um produto (resultado de uma multiplicação). Você deve usar pelo menos duas 
threads para cada operação (soma e multiplicação) e utilizar os dados no 
vetor original.
gcc -o ex_2 ex_2.c -lpthread
g++
*/

#include <pthread.h>
#include <stdio.h>

int vetor[10] = {1,2,3,4,5,6,7,8,9,10};
int soma_1 = 0;
int soma_2 = 0;
int produto_1 = 1;
int produto_2 = 1;

void *thread_soma_1(void *param) {
    int i;
    for (i = 0; i < 5; i++) {
        soma_1 += vetor[i];
    }
    pthread_exit(0);
}

void *thread_soma_2(void *param) {
    int i;
    for (i = 5; i < 10; i++) {
        soma_2 += vetor[i];
    }
    pthread_exit(0);
}

void *thread_multiplicacao_1(void *param)  {
    int i;
    for (i = 0; i < 5; i++) {
        produto_1 *= vetor[i];
    }
    pthread_exit(0);
}

void *thread_multiplicacao_2(void *param)  {
    int i;
    for (i = 50; i < 10; i++) {
        produto_2 *= vetor[i];
    }
    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    pthread_t tid_soma_1, tid_soma_2, tid_multiplicacao_1, tid_multiplicacao_2;
    pthread_attr_t attr;

    pthread_attr_init(&attr);

    pthread_create(&tid_soma_1, &attr, thread_soma_1, NULL);
    pthread_create(&tid_soma_2, &attr, thread_soma_2, NULL);

    pthread_create(&tid_multiplicacao_1, &attr, thread_multiplicacao_1, NULL);
    pthread_create(&tid_multiplicacao_2, &attr, thread_multiplicacao_2, NULL);

    pthread_join(tid_soma_1, NULL);
    pthread_join(tid_soma_2, NULL);

    pthread_join(tid_multiplicacao_1, NULL);
    pthread_join(tid_multiplicacao_2, NULL);

    printf("Soma Final: %d\n", (soma_1 + soma_2));
    printf("Produto Final: %d\n", (produto_1 + produto_2));
}
