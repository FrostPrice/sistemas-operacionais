# EDF - Earliest Deadline First Scheduler

## Compilando

Para compilar o projeto, basta executar o comando:

```bash
make edf - for edf scheduling
```

## Executando

Para executar o projeto, basta executar o comando:

```bash
./edf edf-schedule_pri.txt
```

## Informações Adicionais

O EDF permite trocar a fila de prioridade pela deadline, ou seja, a tarefa com deadline mais próximo é a que será executada. O processo não pode passar o deadline
O deadline é igual ao tempo de execução da tarefa + o tempo atual
Uma thread pode ser usada para verificar os deadlines e reordenar a fila de prioridade
Se estiver sem Folga, aumenta a prioridade da tarefa. Criterio de decisão para aumentar a prioridade
Exemplo dos dados passado:
nome_da_task= = P1, prioridade = 5, burst = 10, deadline = 50
Tempo de inicio = 10
Tempo do sistema = 55 ut
Deadline = 60 ut
Quanto_falta = 5 ut

### Calculos

Os calculos abaixo são feitos para o EDF, e usado como condição para definir a prioridade da tarefa:

- (Burst) tempo_limite = tempo_de_inicio + deadline
- quanto_falta = deadline - tempo_do_sistema
- tedf = tempo_do_sistema + quanto_falta
- folga = tempo_limite - tedf
