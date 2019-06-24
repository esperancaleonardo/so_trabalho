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
#include <sys/time.h> //ITIMER_REAL
#include <string.h>
/* Stack size for each context. */
#define STACK_SIZE SIGSTKSZ * 100
#define TIMER_TYPE ITIMER_REAL

/*******************************************************************************
                             Global data structures

                Add data structures to manage the threads here.
********************************************************************************/
// contadores e varaveis globais
int qtdThreads = 0;
int maxID = 0;
int modeAl; //0 == FCFS 1 == prioridade 2 == Prioridade Reversa
int TIMEOUT;

// Fila para implementação de filas de pronto e terminado
typedef struct fila
{
  thread_t *first;
  thread_t *last;
  int tamanho;
} queue;

// Struct para gerenciar e encapsular thread em execucao e filas de threads
struct threadList
{
  thread_t *running;
  queue ready;
  queue terminated;
  queue waiting;
};

struct threadList managerTh;



/*******************************************************************************
                             Auxiliary functions

                      Add internal helper functions here.
********************************************************************************/


// OK
// insercao do ponteiro para thread *aux em uma lista
// encadeada funcionando como uma fila
void addToReady(thread_t *aux)
{
  aux->state = ready;

  if (managerTh.ready.first == NULL)
  { // caso fila de prontos vazia
    managerTh.ready.first = aux;
    managerTh.ready.first->next = NULL;
    managerTh.ready.first->prev = NULL;
    managerTh.ready.last = aux;
    managerTh.ready.last->next = NULL;
    managerTh.ready.first->prev = NULL;
  }
  else if (managerTh.ready.first == managerTh.ready.last)
  { // caso um elemento na fila
    //printf("caso1elemento\n");
    managerTh.ready.first->next = aux;
    managerTh.ready.last = aux;
    managerTh.ready.last->next = NULL;
    managerTh.ready.last->prev = managerTh.ready.first;
  }
  else
  {
    thread_t *aux2;
   // printf("casogeral\n");
    managerTh.ready.last->next = aux;
    aux2 = managerTh.ready.last;
    managerTh.ready.last = aux;
    managerTh.ready.last->next = NULL;
    managerTh.ready.last->prev = aux2;
  }

  managerTh.ready.tamanho++;
}

//Funcao adciona em um lista duplamente encadeado tarefas terminadas
void addTerminated(thread_t *aux)
{
  aux->state = terminated;

  qtdThreads--;

  if (managerTh.terminated.first == NULL)
  { // caso fila de prontos vazia
    managerTh.terminated.first = aux;
    managerTh.terminated.first->next = NULL;
    managerTh.terminated.first->prev = NULL;
    managerTh.terminated.last = aux;
    managerTh.terminated.last->next = NULL;
    managerTh.terminated.first->prev = NULL;
  }
  else if (managerTh.terminated.first == managerTh.terminated.last)
  { // caso um elemento na fila
    managerTh.terminated.first->next = aux;
    managerTh.terminated.last = aux;
    managerTh.terminated.last->next = NULL;
    managerTh.terminated.last->prev = managerTh.terminated.first;
  }
  else
  {
    thread_t *aux2;
    managerTh.terminated.last->next = aux;
    aux2 = managerTh.terminated.last;
    managerTh.terminated.last = aux;
    managerTh.terminated.last->next = NULL;
    managerTh.terminated.last->prev = aux2;
  }

  managerTh.terminated.tamanho++;
}
void addWaiting(thread_t *aux){
  aux->state = waiting;

  if (managerTh.waiting.first == NULL)
  { // caso fila de prontos vazia
    managerTh.waiting.first = aux;
    managerTh.waiting.first->next = NULL;
    managerTh.waiting.first->prev = NULL;
    managerTh.waiting.last = aux;
    managerTh.waiting.last->next = NULL;
    managerTh.waiting.first->prev = NULL;
  }
  else if (managerTh.waiting.first == managerTh.waiting.last)
  { // caso um elemento na fila
    managerTh.waiting.first->next = aux;
    managerTh.waiting.last = aux;
    managerTh.waiting.last->next = NULL;
    managerTh.waiting.last->prev = managerTh.waiting.first;
  }
  else
  {
    thread_t *aux2;
    managerTh.waiting.last->next = aux;
    aux2 = managerTh.waiting.last;
    managerTh.waiting.last = aux;
    managerTh.waiting.last->next = NULL;
    managerTh.waiting.last->prev = aux2;
  }

  managerTh.waiting.tamanho++;

}

// OK
// seleciona um contexto de uma thread na fila de prontos de acordo
// com a politica de escalonamento definida anteriormente
thread_t *getFromReady()
{

  if (modeAl == 0)
  { //FCFS
    if (managerTh.ready.first == NULL)
    {
      return NULL; // Fila de prontos vazia
    }
    else
    {
      // pega o primeiro elementoda fila de prontos e atualiza o começo
      // da fila para o proximo elemento
      thread_t *selecionado = managerTh.ready.first;
      managerTh.ready.first = managerTh.ready.first->next;
      managerTh.ready.first->prev = NULL;
      managerTh.ready.tamanho--; //decrementa o tamanho da fila
      if (managerTh.ready.tamanho == 0)
      { //atualiza os ponteiros caso a fila fique vazia
        managerTh.ready.first = NULL;
        managerTh.ready.first->next = NULL;
        managerTh.ready.first->prev = NULL;
        managerTh.ready.last = NULL;
        managerTh.ready.last->next = NULL;
        managerTh.ready.last->prev = NULL;
      }
      return selecionado;
    }
  }
  else if (modeAl == 2)
  {
    if (qtdThreads > 1)
    {
      thread_t *selecionado = prioSelectRev(); //seleciona a maior prioridade
      managerTh.ready.tamanho--;               //decrementa o tamanho da fila
      return selecionado;
    }
    else
    {
      thread_t *selecionado = managerTh.ready.first;
      managerTh.ready.tamanho--; //decrementa o tamanho da fila
      if (managerTh.ready.tamanho == 0)
      { //atualiza os ponteiros caso a fila fique vazia
        managerTh.ready.first = NULL;
        managerTh.ready.last = NULL;
      }
      return selecionado;
    }
  }

  else
  { //Prioridade
    if (qtdThreads > 1)
    {
      thread_t *selecionado = prioSelect(); //seleciona a maior prioridade
      managerTh.ready.tamanho--;            //decrementa o tamanho da fila
      return selecionado;
    }
    else
    {
      thread_t *selecionado = managerTh.ready.first;
      managerTh.ready.tamanho--; //decrementa o tamanho da fila
      if (managerTh.ready.tamanho == 0)
      { //atualiza os ponteiros caso a fila fique vazia
        managerTh.ready.first = NULL;
        managerTh.ready.last = NULL;
      }
      return selecionado;
    }
  }
  return NULL;
}
//ok
//Pega tarefa da fila de waiting
thread_t *getWaiting()
{
  if (managerTh.waiting.first == NULL){
    return NULL;
  }
  else {
    thread_t *selecionado = managerTh.waiting.first;
    managerTh.waiting.first = managerTh.waiting.first->next;
    managerTh.waiting.first->prev = NULL;
    managerTh.waiting.tamanho--;
    if (managerTh.waiting.tamanho == 0){
      managerTh.waiting.first = NULL;
      managerTh.waiting.first->next = NULL;
      managerTh.waiting.first->prev = NULL;
      managerTh.waiting.last = NULL;
      managerTh.waiting.last->next = NULL;
      managerTh.waiting.last->prev = NULL;
    }
    return selecionado;
  }
}

// OK
// funcao que realiza a troca de contexto entre as threads,
// trocando o contexto da thread em execucao pelo contexto
// aux recebida como parametro
void addToRunning(thread_t *aux){

  if(aux != NULL){
    aux->state = running;
    //troca das threads no gerenciador
    thread_t *running = managerTh.running;
    managerTh.running = aux;
    if(running != NULL){
      //troca de contexto caso já tenha uma thread rodando
      int res = swapcontext(&running->ctx, &aux->ctx);
      if(res < 0){
        perror("swapcontext");
        exit(EXIT_FAILURE);
      }
    }else{
      //"troca" de contexto quando se executar a primeira thread da fila de prontos
      setcontext(&aux->ctx); //Passa o controle para aux
    }
  }
}

// Seleciona a thread com maior prioridade na fila de threads com estado ready
thread_t* prioSelect(){

    thread_t* selected;
    thread_t* iter  = managerTh.ready.first;

    int maxPrio = iter->prio;
    do{ //itera entre os contextos e seleciona o maior em Selected

        if(iter->prio >= maxPrio){
            maxPrio = iter->prio;
            selected = iter;
        }
        iter = iter->next;
    }while(iter != NULL);

    //printf("maxPrio Selected (first appear): %d\n", maxPrio);
    //remove o contexto da lista duplamente encadeada
    //atualizando os ponteiros prev e next
    if(selected == managerTh.ready.first){
        //printf("F\n");
        managerTh.ready.first =  managerTh.ready.first->next;
        managerTh.ready.first->prev = NULL;
    }
    else if(selected == managerTh.ready.last){
       // printf("L\n");
        managerTh.ready.last =  managerTh.ready.last->prev;
        managerTh.ready.last->next = NULL;
    }
    else{
        //printf("M\n");
        thread_t* aux = selected;
        selected->prev->next = selected->next;
        aux->next->prev = aux->prev;
    }
    return selected;
}

// Seleciona a thread com menor prioridade na fila de threads com estado ready
thread_t* prioSelectRev(){

    thread_t* selected;
    thread_t* iter  = managerTh.ready.first;

    int maxPrio = iter->prio;
    do{ //itera entre os contextos e seleciona o maior em Selected

        if(iter->prio <= maxPrio){
            maxPrio = iter->prio;
            selected = iter;
        }
        iter = iter->next;
    }while(iter != NULL);

    //printf("maxPrio Selected (first appear): %d\n", maxPrio);
    //remove o contexto da lista duplamente encadeada
    //atualizando os ponteiros prev e next
    if(selected == managerTh.ready.first){
        //printf("F\n");
        managerTh.ready.first =  managerTh.ready.first->next;
        managerTh.ready.first->prev = NULL;
    }
    else if(selected == managerTh.ready.last){
       // printf("L\n");
        managerTh.ready.last =  managerTh.ready.last->prev;
        managerTh.ready.last->next = NULL;
    }
    else{
        //printf("M\n");
        thread_t* aux = selected;
        selected->prev->next = selected->next;
        aux->next->prev = aux->prev;
    }
    return selected;
}

// OK
// Lista as prioridades na fila de prontos
void printReadyPrio(){
  thread_t *aux = managerTh.ready.first;
  //printf("tamanho %d || ", managerTh.ready.tamanho);
  int count = 0;
  do{
    //printf("%d ", aux->prio);
    aux = aux->next;
    count++;
  } while (aux != NULL);
  //printf("\n");
}

// OK
// Lista as prioridades na fila de prontos em ordem reversa
void printReadyPrioRev()
{
  thread_t *aux = managerTh.ready.last;
  printf("Tamanho %d || ", managerTh.ready.tamanho);

  int count = 0;
  do
  {
    printf("%d ", aux->prio);
    aux = aux->prev;
    count++;
  } while (aux != NULL);
  printf("\n");
}

int timer_signal(int timer_type)
{
  int sig;

  switch (timer_type)
  {
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

/* 
  Secao responsavel por controlar o tempo do nosso scheduler
 */
void set_timer(int type, void (*handler)(int), int ms)
{
  struct itimerval timer;
  struct sigaction sa;

  /* Install signal handler for the timer. */
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handler;
  sigaction(timer_signal(type), &sa, NULL);

  /* Configure the timer to expire after ms msec... */
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = ms * 1000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;

  if (setitimer(type, &timer, NULL) < 0)
  {
    perror("Setting timer");
    exit(EXIT_FAILURE);
  };
}
/* Timer signal handlar */
void timer_handler(int signum)
{
  fprintf(stderr, "======> timer \n");

  set_timer(TIMER_TYPE, timer_handler, TIMEOUT);
  yield();
}

/*******************************************************************************
                    Implementation of the Simple Threads API
********************************************************************************/
// OK
// inicia a execucao das threads que sofreram spawn de acordo
// com a política de escalonamento definida
void start(){
  thread_t *temp = getFromReady();
  addToRunning(temp);
  //printReadyPrio();
  printf("%lu : \n",(unsigned long int)&temp->ctx);
  puts("i should not print");
}

// OK
// inicializacao da estrutura gerenciadora de Threads
//
// _modeAl int booleano, diferencia entre as politicas
//          de escalonamento FCFS(0) e Prioridade(1)
int  init(int _modeAl, int _timeSlice){
  TIMEOUT = _timeSlice;
  struct threadList block =  {NULL, {NULL, NULL, 0}, {NULL, NULL, 0}};
  //set_timer(TIMER_TYPE, timer_handler, TIMEOUT);  //Timer
  managerTh = block;
  modeAl = _modeAl;
  printf("Escalonamento Selecionado: ");
  (modeAl == 0) ? printf("%s\n", "FCFS"):printf("%s\n", "PRIO");

  return 1;
}


// OK
// Cria tarefa representada pela funcao start e a adciona na fila de prontos
// (*start) ponteiro para algma outra funcao implementada
tid_t spawn(void (*start)()){

  thread_t *novaTarefa = malloc(sizeof(thread_t));
  qtdThreads++; //Incrementa o contador de threads
  maxID++;      //Incrementa o gerador de Id das threads

  void *stack = malloc(STACK_SIZE);
  if (stack == NULL) {
    perror("Allocating stack");
    exit(EXIT_FAILURE);
  }
  if (getcontext(&novaTarefa->ctx) < 0) {
    perror("getcontext");
    exit(EXIT_FAILURE);
  }
  //ajustes no contexto da criacao da thread
  novaTarefa->ctx.uc_link           = &novaTarefa->next->ctx;
  novaTarefa->ctx.uc_stack.ss_sp    = stack;
  novaTarefa->ctx.uc_stack.ss_size  = STACK_SIZE;
  novaTarefa->ctx.uc_stack.ss_flags = 0;

  //setando thread_id e prioridade
  novaTarefa->tid = maxID;

  novaTarefa->prio = (unsigned long int)(&novaTarefa->ctx)%100;
  novaTarefa->next = NULL;
  novaTarefa->prev = NULL;
  novaTarefa->state = ready;

  // unindo o contexto criado a função passada por parâmetro
  makecontext(&novaTarefa->ctx, start, 0);

  //adicionar na fila de prontos
  addToReady(novaTarefa);

  printf("Contexto criado: %lu || PRIO: %d\n",(unsigned long int)&novaTarefa->ctx, novaTarefa->prio);
  return -1;
}

// OK
// realiza o escalonamento coooperativo quando eh chamada
void yield()
{
  if (qtdThreads > 1)
  {
    if (managerTh.ready.first != NULL)
    {
      thread_t *jobExec = managerTh.running; // Copia a tarefa que esta em execucao
      addToReady(jobExec);                   // Insere a tarefa no fim da fila de prontos
      //printReadyPrio();
      thread_t *jobNew = getFromReady(); // Pega a nova tarefa no início da fila de prontos
      addToRunning(jobNew);              // Faz o dispatch para a execucao da tarefa
                                         //printReadyPrio();
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
// Daqui para baixo escalonamento preemptivo

//Done pega a tarefa em execucao finaliza ela colocando na fila de terminados apos isso

void done()
{
  printf("enrtou DONE");
  thread_t *tarefa = managerTh.running;
  addTerminated(tarefa);
  thread_t *tarefaEsperada = getWaiting();

  //Pega as tarefas da fila de wainting e coloca em Ready
  while (tarefaEsperada != NULL)
  {
    addToReady(tarefaEsperada);
    tarefaEsperada = getWaiting();
  }

  //free(tarefa);
  thread_t *next = getFromReady();
  addToRunning(next);
}

//join pega a tarefa em execucao coloca na fila Waiting pega uma nova tarefa da fila de Prontos e coloca pra rodar e retorna ID do pai
tid_t join()
{
 
  thread_t *tarefa = managerTh.running;

   printf("runinasda");

  addWaiting(tarefa); //adiciona tarefa a fila de waiting 
  thread_t *novaTarefa = getFromReady();
  addToRunning(novaTarefa);

  return tarefa->tid;
}