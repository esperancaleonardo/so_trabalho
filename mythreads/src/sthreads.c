/* On Mac OS (aka OS X) the ucontext.h functions are deprecated and requires the
   following define.
*/
#define _XOPEN_SOURCE 700

/* On Mac OS when compiling with gcc (clang) the -Wno-deprecated-declarations
   flag must also be used to suppress compiler warnings.
*/

#include <signal.h>   /* SIGSTKSZ (default stack size), MINDIGSTKSZ (minimal
                         stack size) */
#include <stdio.h>    /* puts(), printf(), fprintf(), perror(), setvbuf(), _IOLBF,
                         stdout, stderr */
#include <stdlib.h>   /* exit(), EXIT_SUCCESS, EXIT_FAILURE, malloc(), free() */
#include <ucontext.h> /* ucontext_t, getcontext(), makecontext(),
                         setcontext(), swapcontext() */
#include <stdbool.h>  /* true, false */

#include "sthreads.h"
#include <sys/time.h>  //ITIMER_REAL
#include <string.h>
/* Stack size for each context. */
#define STACK_SIZE SIGSTKSZ*100
#define TIMEOUT 10      //ms
#define TIMER_TYPE ITIMER_REAL

/*******************************************************************************
                             Global data structures

                Add data structures to manage the threads here.
********************************************************************************/

typedef struct fila{
  thread_t *first;
  thread_t *last;
}queue;

struct threadList {
  thread_t *running;

  queue ready;
  queue terminated;
};

struct threadList managerTh;

int qtdThreads = 0;
int maxID = 0;

int modeAl = 0; //0 == FCFS 1 == prioridade


/*******************************************************************************
                             Auxiliary functions

                      Add internal helper functions here.
********************************************************************************/
void addToReady(thread_t *aux){
  aux->state = ready;
  //Modo Cooperativo
  if(modeSc == 0){
    if(managerTh.ready.first == NULL){
      managerTh.ready.first = aux;
      managerTh.ready.first->next = NULL;
    }else if(managerTh.ready.last == NULL){
      managerTh.ready.first->next = aux;
      managerTh.ready.last = aux;
      managerTh.ready.last->next = NULL;
    }else{
      managerTh.ready.last->next = aux;
      managerTh.ready.last = aux;
      managerTh.ready.last->next = NULL;
    }

  }
  //Modo preemptivo
  else{

  }

}
thread_t *prioriSelect(){

  struct threadList tempManager;
  tempManager = managerTh;
  thread_t *temp;
  int maxPriori = 0;

  while(tempManager.ready.first != NULL){
    if(tempManager.ready.first->priori > maxPriori){
      maxPriori = tempManager.ready.first->priori;
      temp = tempManager.ready.first;

    }
    tempManager.ready.first = tempManager.ready.first->next;
  }

  return temp;

}
//Funcao responsavel por retirar tarefa da fila de prontos de acordo com o algoritmo referenciado
thread_t *getFromReady(){

    if(modeAl == 0){ //FCFS
      if(managerTh.ready.first == NULL){
        return NULL; // Fila de prontos vazia
      }else{
        thread_t *jobNew = managerTh.ready.first;
        managerTh.ready.first = managerTh.ready.first->next;
        return jobNew;
      }

    }else{ //Prioridade
        thread_t *jobNew = prioriSelect();
        return jobNew;
    }

    return NULL;
}

//Funcao faz a troca de contexto
void addToRunning(thread_t *aux){

  if(aux != NULL){
    aux->state = running;
    thread_t *temp = managerTh.running;
    managerTh.running = aux;
    if(temp != NULL){
      int res = swapcontext(&temp->ctx, &aux->ctx);
      if(res < 0){
        perror("swapcontext");
        exit(EXIT_FAILURE);
      }
    }else{
      setcontext(&aux->ctx); //Passa o controle para aux
    }
  }
}

void addToTerminated(thread_t *aux){
  aux->state = terminated;
  qtdThreads--;

  if(managerTh.terminated.first == NULL){
      managerTh.terminated.first = aux;
      managerTh.terminated.first->next = NULL;
    }else if(managerTh.terminated.last == NULL){
      managerTh.terminated.first->next = aux;
      managerTh.terminated.last = aux;
      managerTh.terminated.last->next = NULL;
    }else{
      managerTh.terminated.last->next = aux;
      managerTh.terminated.last = aux;
      managerTh.terminated.last->next = NULL;
    }

}

int timer_signal(int timer_type) {
  int sig;

  switch (timer_type) {
    case ITIMER_REAL:
      sig = SIGALRM;
      break;
    case ITIMER_VIRTUAL:
      sig = SIGVTALRM;
      break;
    case ITIMER_PROF:
      sig = SIGPROF;
      break;
    default:
      fprintf(stderr, "ERROR: unknown timer type %d!\n", timer_type);
      exit(EXIT_FAILURE);
  }

  return sig;
}

/* Set a timer and a handler for the timer.

   Arguments

   type: type of timer, one of ITIMER_REAL, ITIMER_VIRTUAL, or ITIMER_PROF.

   handler: timer signal handler.

   ms: time in ms for the timer.

 */
void set_timer(int type, void (*handler) (int), int ms) {
  struct itimerval timer;
  struct sigaction sa;

  /* Install signal handler for the timer. */
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler =  handler;
  sigaction (timer_signal(type), &sa, NULL);

  /* Configure the timer to expire after ms msec... */
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = ms*1000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;

  if (setitimer (type, &timer, NULL) < 0) {
    perror("Setting timer");
    exit(EXIT_FAILURE);
  };
}
/* Timer signal handlar */
void timer_handler (int signum) {
  fprintf (stderr, "======> timer (%03d)\n", signum);
  puts("test");
  set_timer(TIMER_TYPE, timer_handler, TIMEOUT);
  yield();
}


/*******************************************************************************
                    Implementation of the Simple Threads API
********************************************************************************/
void start(){
  thread_t *temp = getFromReady();
  addToRunning(temp);
  printf("%lu : \n",(unsigned long int)&temp->ctx);
  puts("i should not print");

}

int  init(int _modeAl, int _modeSc){
  struct threadList block =  {NULL, {NULL, NULL}, {NULL, NULL}};
  //Timer Slicer set
  set_timer(TIMER_TYPE, timer_handler, TIMEOUT);
  managerTh = block;
  modeAl = _modeAl;
  return 1;
}

//Criar tarefa e coloca na fila de prontos
tid_t spawn(void (*start)()){

  thread_t *novaTarefa = malloc(sizeof(thread_t));
  qtdThreads++; //Incrementa o contador de threads
  maxID++;
  novaTarefa->tid = maxID;
  void *stack = malloc(STACK_SIZE);
  if (stack == NULL) {
    perror("Allocating stack");
    exit(EXIT_FAILURE);
  }
  if (getcontext(&novaTarefa->ctx) < 0) {
    perror("getcontext");
    exit(EXIT_FAILURE);
  }
  novaTarefa->ctx.uc_link           = &novaTarefa->next->ctx;
  novaTarefa->ctx.uc_stack.ss_sp    = stack;
  novaTarefa->ctx.uc_stack.ss_size  = STACK_SIZE;
  novaTarefa->ctx.uc_stack.ss_flags = 0;

  makecontext(&novaTarefa->ctx, start, 0);

//adicionar na fila de prontos
  addToReady(novaTarefa);

  printf("Contexto criado: %lu\n",(unsigned long int)&novaTarefa->ctx);
  return -1;
}

void yield(){
  if(qtdThreads > 1){
    //Verifica de tem alguma tarefa na fila de pronto aguardando sua vez
    if(managerTh.ready.first != NULL){
      thread_t *jobExec = managerTh.running; // copia a tarefa que esta em execucao
      addToReady(jobExec); // Coloca ela de volta para a fila de prontos
      thread_t *jobNew = getFromReady(); //pega a nova tarefa
      addToRunning(jobNew); //Faz o dispath para modo run
    }
  }
  printf("Qtd thread: %d\n", qtdThreads);
}

void  done(){
  thread_t *aux = managerTh.running;
  addToTerminated(aux);

  thread_t *jobNew = getFromReady();
  addToRunning(jobNew);

}

tid_t join() {
  return -1;
}
