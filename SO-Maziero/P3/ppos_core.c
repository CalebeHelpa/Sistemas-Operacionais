//-----------------------------------------------------------------------------|
//  CALEBE POMPEO HELPA                                                        |
//  GRR20190359                                                                |
//  SISTEMAS OPERACIONAIS - CARLOS MAZIERO                                     |
//-----------------------------------------------------------------------------|
//  OPERAÇÕES EM UMA FILA GENÉRICA                                             |
//-----------------------------------------------------------------------------|

#include <stdio.h>
#include <stdlib.h>

#include "ppos.h"
#include "queue.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */
#define MANAGEMENT_TASKS 1

task_t task_Main, task_Dispatcher, *task_Current;
task_t *task_Queue = NULL;

int nxtID = 0;
int userTasks = 0;

// funções gerais ==============================================================
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Retorna a proxima tarefa da fila de tarefas prontas
task_t *scheduler(){
  task_t *nextTask;
  nextTask = task_Queue;
  task_Queue = task_Queue->next;
  return nextTask;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Determina qual a proxima tarefa a executar a cada troca de contexto
void dispatcher(void *arg){
  
  task_t *nextTask;
  while(userTasks > 0){
    
    nextTask = scheduler();
    if(nextTask != NULL){
      task_switch(nextTask);

      switch(nextTask->status){
        case 0: //terminada
          #ifdef DEBUG
            printf ("### PPOS: Removendo tarefa %i da fila\n", nextTask->id);
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

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init(){

  setvbuf(stdout, 0, _IONBF, 0);

  #ifdef DEBUG
    printf ("### PPOS: Inicializando a tarefa main\n");
  #endif

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
  task_Main.id = 0;
  task_Current = &task_Main;
  nxtID++;

  task_create(&task_Dispatcher, dispatcher, NULL);
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
  task->prev = NULL;
  task->next = NULL;
  task->status = 1;
  task->id = nxtID;
  nxtID++;

  if(task->id > MANAGEMENT_TASKS){
    userTasks++;
    queue_append((queue_t **)(&task_Queue), (queue_t *)(task));
  }

  return 0;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exit_code){
  
  #ifdef DEBUG
    printf ("### PPOS: Saindo da tarefa %i\n", task_Current->id);
  #endif
  
  if(task_Current == &task_Dispatcher)
    task_switch(&task_Main);
  
  task_Current->status = 0;
  task_yield();
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// alterna a execução para a tarefa indicada
int task_switch (task_t *task){
  
  #ifdef DEBUG
    printf ("### PPOS: Mudando da tarefa %i para a tarefa %i\n", task_Current->id, task->id);
  #endif

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

// suspende a tarefa atual na fila "queue"
void task_suspend (task_t **queue) ;

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// acorda a tarefa indicada, que está suspensa na fila indicada
int task_resume (task_t *task, task_t **queue) ;

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =============================================================================