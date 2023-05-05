// GRR20190359 Calebe Pompeo Helpa
//-----------------------------------------------------------------------------|
//  CALEBE POMPEO HELPA                                                        |
//  GRR20190359                                                                |
//  SISTEMAS OPERACIONAIS - CARLOS MAZIERO                                     |
//-----------------------------------------------------------------------------|
//  OPERAÇÕES EM UMA FILA GENÉRICA                                             |
//-----------------------------------------------------------------------------|

#include <sys/time.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

#include "queue.h"
#include "ppos.h"

#define MANAGEMENT_TASKS 1
#define STACKSIZE 64*1024
#define QUANTUM 20

task_t task_Main, task_Dispatcher, *task_Current;
task_t *task_Queue = NULL;
task_t *taskSleep_Queue = NULL;

struct sigaction action;
struct itimerval timer;
unsigned int time = 0;

int userTasks = 0;
int nxtID = 0;

// =============================================================================
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Retorna o tempo do sistema em milissegundos
unsigned int systime(){
  return time;
}

// Retorna a proxima tarefa da fila de tarefas prontas
task_t *scheduler(){
  task_t *nextTask, *first, *current;
  
  if(task_Queue == NULL){
    //printf("AQUI\n");
    return NULL;
  }
  first = task_Queue;
  nextTask = first;
  current = first->next;

  // Determina a tarefa de maior prioridade
  while(current != first){
    if(current->dinamPrio < nextTask->dinamPrio)
      nextTask = current;
    current = current->next;
  }
  nextTask->dinamPrio = nextTask->statPrio;

  // Atualiza prioridades dinamicas das outras tarefas
  current = nextTask->next;
  while(current != nextTask){
    current->dinamPrio--;
    current = current->next;
  }

  nextTask->quantum = QUANTUM;
  task_Queue = nextTask;
  return nextTask;
}

void wake_tasks(){
  if(taskSleep_Queue == NULL)
    return;

  task_t *current, *next;
  current = taskSleep_Queue;
  next = current->next;
  for(int i=0; i < queue_size((queue_t *)taskSleep_Queue); i++){
    if(systime() == current->awakeTime)
      task_resume(current, &taskSleep_Queue);

    current = next;
    if(next != NULL)
      next = next->next;

  }
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Determina qual a proxima tarefa a executar a cada troca de contexto
void dispatcher(void *arg){
  unsigned int taskStart;
  unsigned int taskFinish;
  unsigned int taskTotal;

  unsigned int dispStart = systime();
  unsigned int dispFinish;
  unsigned int dispTotal;

  task_t *nextTask;
  while(userTasks > 0){
    
    if(systime()%100 == 0)
      wake_tasks();
    
    nextTask = scheduler();
    if(nextTask != NULL){
      
      dispFinish = systime();
      dispTotal = dispFinish - dispStart;
      task_Dispatcher.procTime += dispTotal;
      
      taskStart = systime();
      task_switch(nextTask);
      taskFinish = systime();
      
      dispStart = systime();
      
      taskTotal = taskFinish - taskStart;
      nextTask->procTime +=  taskTotal;

      switch(nextTask->status){
        case 0: //terminada
          #ifdef DEBUG
            printf ("### PPOS: Tarefa terminada, removendo tarefa %i da fila de prontos\n", nextTask->id);
          #endif
          queue_remove((queue_t **)(&task_Queue), (queue_t *)(nextTask));
          free((void *)(nextTask->context.uc_stack.ss_sp));
          nextTask->context.uc_stack.ss_size = 0;
          userTasks--;
        break;
      }
    }
  }
  task_exit(0);
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Tratador de sinal
void signal_handler(int signum){
  if(task_Current->userTask){
    if(!task_Current->quantum){
      task_switch(&task_Dispatcher);
    }
    task_Current->quantum--;
  }
  time++;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init(){

  setvbuf(stdout, 0, _IONBF, 0);

  #ifdef DEBUG
    printf ("### PPOS: Inicializando a tarefa main\n");
  #endif

  // Cria main
  char *stackMain;
  stackMain = malloc (STACKSIZE);
  if(stackMain){
    task_Main.context.uc_stack.ss_sp = stackMain;
    task_Main.context.uc_stack.ss_size = STACKSIZE;
    task_Main.context.uc_stack.ss_flags = 0;
    task_Main.context.uc_link = 0;
  }else{
    fprintf(stdout, "### ERRO: Falta de espaco para criacao da tarefa main.\n");
    exit (1);
  }
  task_Main.userTask = 1;
  task_Main.quantum = QUANTUM;
  task_Main.prev = NULL;
  task_Main.next = NULL;
  task_Main.status = 1;
  task_Main.statPrio = 0;
  task_Main.dinamPrio = 0;
  task_Main.id = nxtID;
  nxtID++;
  userTasks++;

  task_Current = &task_Main;
  queue_append((queue_t **)(&task_Queue), (queue_t *)(&task_Main));

  // Cria dispatcher
  #ifdef DEBUG
    printf ("### PPOS: Criando a tarefa dispatcher\n");
  #endif
  task_create(&task_Dispatcher, dispatcher, NULL);

  // registra a ação para o sinal de timer SIGALRM
  action.sa_handler = signal_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  if(sigaction(SIGALRM, &action, 0) < 0){
    perror("### ERRO: Erro em sigaction.\n");
    exit(1);
  }

  timer.it_value.tv_usec = 1000;   // primeiro disparo, em micro-segundos
  timer.it_value.tv_sec  = 0;      // primeiro disparo, em segundos
  timer.it_interval.tv_usec = 1000;// disparos subsequentes, em micro-segundos
  timer.it_interval.tv_sec  = 0;   // disparos subsequentes, em segundos

  // arma o temporizador ITIMER_REAL (vide man setitimer)
  if(setitimer(ITIMER_REAL, &timer, 0) < 0){
    perror("### ERRO: Erro em setitimer.\n");
    exit(1);
  }
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =============================================================================

// gerência de tarefas =========================================================
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task, void (*start_func)(void *), void *arg){

  #ifdef DEBUG
    printf ("### PPOS: Criando a tarefa %i\n", nxtID);
  #endif

  getcontext(&task->context);

  char *stack;
  stack = malloc (STACKSIZE);
  if(stack){
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;
  }else{
    fprintf(stdout, "### ERRO: Falta de espaco para criacao da tarefa %i.\n", nxtID);
    return (-1);
  }

  makecontext(&task->context, (void *) (*start_func), 1 , (char *)(arg));
  task->quantum = QUANTUM;
  task->prev = NULL;
  task->next = NULL;
  
  task->status = 1;
  task->userTask = 0;
  task->exitCode = 0;
  task->statPrio = 0;
  task->dinamPrio = 0;
  task->suspendQueue = NULL;
  
  task->execTime = systime();
  task->activations = 0;
  task->procTime = 0;
  
  task->id = nxtID;
  nxtID++;

  // Caso o id da tarefa criada seja maior que o numero de tarefas de gerenciamento
  if(task->id > MANAGEMENT_TASKS){
    task->userTask = 1;
    userTasks++;
    queue_append((queue_t **)(&task_Queue), (queue_t *)(task)); // Insere na fila
  }

  return 0;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exit_code){
  
  task_Current->execTime = systime() - task_Current->execTime;
  printf ("Task %i exit: execution time %i ms, processor time %i ms, %i activations\n", task_Current->id, task_Current->execTime, task_Current->procTime, task_Current->activations);
  
  if(task_Current == &task_Dispatcher){
    exit(0);
  }
  
  task_Current->status = 0;
  task_Current->exitCode = exit_code;
  
  if(task_Current->suspendQueue != NULL){
    task_t *current, *next;
    current = task_Current->suspendQueue;
    next = current->next;
    for(int i=0; i < queue_size((queue_t *)task_Current->suspendQueue); i++){
      task_resume(current, &task_Current->suspendQueue);
      current = next;
      if(next != NULL)
        next = next->next;
    }
  }
  task_yield();
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// alterna a execução para a tarefa indicada
int task_switch (task_t *task){
  
  #ifdef DEBUG
    printf ("### PPOS: Mudando da tarefa %i para a tarefa %i\n", task_Current->id, task->id);
  #endif
  
  task->activations++;
  
  if(!task){
    fprintf(stdout, "### ERRO: Tentativa de mudança para tarefa nula.\n");
    return(-1);
  }

  task_t *taskPrev;
  taskPrev = task_Current;
  task_Current = task;
  return swapcontext(&taskPrev->context, &task->context);
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id (){
    return task_Current->id;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield (){
  task_switch(&task_Dispatcher);
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio){
  if(task == NULL){
    task_Current->statPrio = prio;
    task_Current->dinamPrio = prio;
    return;
  }
  task->statPrio = prio;
  task->dinamPrio = prio;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task){
  if(task == NULL)
    return task_Current->statPrio;
  return task->statPrio;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// suspende a tarefa atual na fila "queue"
void task_suspend (task_t **queue){
  task_Current->status = -1;
  #ifdef DEBUG
    printf ("### PPOS: Tarefa suspensa, removendo tarefa %i da fila de prontos\n", task_Current->id);
  #endif
  if(!queue_remove((queue_t **)(&task_Queue), (queue_t *)(task_Current))){
    queue_append((queue_t **)(queue), (queue_t *)(task_Current));
  }
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// acorda a tarefa indicada, que está suspensa na fila indicada
void task_resume (task_t *task, task_t **queue){
  task->status = 1;
  #ifdef DEBUG
    printf ("### PPOS: Tarefa resumida, removendo tarefa %i da fila suspensão\n", task->id);
  #endif
  if(!queue_remove((queue_t **)(queue), (queue_t *)(task))){
    queue_append((queue_t **)(&task_Queue), (queue_t *)(task));
    #ifdef DEBUG
      printf ("### PPOS: Acordando tarefa %i\n", task->id);
    #endif
  }  
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// a tarefa corrente aguarda o encerramento de outra task
int task_join (task_t *task){
  if(task == NULL)
    return -1;
  
  if(task->status == 0)
    return task->exitCode;

  task_suspend(&task->suspendQueue);
  task_yield();
  return task->exitCode;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// suspende a tarefa corrente por t milissegundos
void task_sleep (int t){
  task_Current->awakeTime = systime() + t;
  task_suspend(&taskSleep_Queue);
  task_yield();
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =============================================================================