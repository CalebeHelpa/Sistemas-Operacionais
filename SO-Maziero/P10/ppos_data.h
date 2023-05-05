// GRR20190359 Calebe Pompeo Helpa
// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next;   // ponteiros para usar em filas
  int id;                       // identificador da tarefa
  ucontext_t context;           // contexto armazenado da tarefa
  
  short userTask;               // se tarefa de usuario = 1 e pode ser preemptada, se tarefa de sistema = 0
  short quantum;                // quantum da tarefa
  short status;                 // terminada = 0, pronta = 1, suspensa = -1
  short exitCode;               // valor de termino da tarefa, default = 0
  struct task_t *suspendQueue;  // fila de tarefas que estao suspensas aguardando tarefa
  
  unsigned int activations;
  unsigned int awakeTime;
  unsigned int execTime;
  unsigned int procTime;
                                //prioridades variam de -20 a +20 sendo -20 a maior prioridade
  int statPrio;                 // prioridade estatica (0 por default)
  int dinamPrio;                // prioridade dinamica

} task_t ;

// Estrutura que define um semáforo
typedef struct semaphore_t
{
  int value;
  int exitCode;
  int locked;
  int init;
  struct task_t* semQueue;

} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct mqueue_t
{
  int exitCode;
  int maxMsg, msgSize;
  int start, end, size;
  
  void *msgQueue;

  semaphore_t semBuff, semItem, semSlot;

} mqueue_t ;

#endif

