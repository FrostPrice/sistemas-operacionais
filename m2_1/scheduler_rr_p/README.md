# Round Robin with Priority Scheduler

## Arquivos alterados

- list.h e list.c: Adicionamos a função insert_end, que insere um elemento no final da lista. (Isso foi feito para facilitar a implementação do escalonador Round Robin com prioridade, evitando que sempre a mesma task seja adicionada no inicio da lista).
- schedule_rr_p.c: Implementamos o escalonador Round Robin com prioridade.

## Compilando

Para compilar o projeto, basta executar o comando:

```bash
make rr_p - for round-robin scheduling
```

## Executando

Para executar o projeto, basta executar o comando:

```bash
./rr_p rr-schedule_pri.txt
```
