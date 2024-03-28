/*
Desenvolver uma aplicação de leia uma entrada do teclado, some com uma
constante e imprima na tela o dado resultante da soma. Divida as tarefas em 
threads usando a biblioteca pthread (preferencialmente).
gcc -o ex_1 ex_1.c -lpthread
g++
*/
#include <pthread.h>
#include <stdio.h>

int soma = 0;
int valor_constante = 10;

void *thread_teclado(void *param){
	int valor_temp = 0;
	printf("\nIndique o valor a ser somado: ");
	scanf("%d", &valor_temp);
	soma = valor_temp;
   pthread_exit(0);
}

void *thread_soma(void *param){
	int valor_temp = 0;
	valor_temp = soma + valor_constante;
	soma = valor_temp; 
   pthread_exit(0);
}

void *thread_print(void *param){
	printf("\nValor somado: %d\n", soma);
   pthread_exit(0);
}

int main(int argc, char *argv[]){
	pthread_t tid_soma, tid_teclado, tid_print; /* the thread identifier */
	pthread_attr_t attr; /* set of attributes for the thread */
	pthread_attr_init(&attr);
	
  pthread_create(&tid_teclado, &attr, thread_teclado, NULL);
  pthread_create(&tid_soma, &attr, thread_soma, NULL);
  pthread_create(&tid_print, &attr, thread_print, NULL);
	
  pthread_join(tid_teclado, NULL);
  pthread_join(tid_soma, NULL);
  pthread_join(tid_print, NULL);
}
