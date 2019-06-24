#include <stdlib.h>   // exit(), EXIT_FAILURE, EXIT_SUCCESS
#include <stdio.h>    // printf(), fprintf(), stdout, stderr, perror(), _IOLBF
#include <stdbool.h>  // true, false
#include <limits.h>   // INT_MAX

#include "sthreads.h" // init(), spawn(), yield(), done()

/*******************************************************************************
                   Functions to be used together with spawn()

    You may add your own functions or change these functions to your liking.
********************************************************************************/

/* Prints the sequence 0, 1, 2, .... INT_MAX over and over again.
 */
void numbers() {
  int n = 0;
  while (true) {
    printf(" n = %d\n", n);
    n = (n + 1) % (INT_MAX);
    if (n == 300) {
        //join();    //Commented out in order to test timer.
    }
 
   yield();
   
  }
}

/* Prints the sequence a, b, c, ..., z over and over again.
 */
void letters() {
  char c = 'a';

  while (true) {
      printf(" c = %c\n", c);
      if(c == 'f') done();
     
      c = (c == 'z') ? 'a' : c + 1;
       yield();
    }
}

/* Calculates the nth Fibonacci number using recursion.
 */
int fib(int n) {
  switch (n) {
  case 0:
    return 0;
  case 1:
    return 1;
  default:
    return fib(n-1) + fib(n-2);
  }
}

/* Print the Fibonacci number sequence over and over again.

   https://en.wikipedia.org/wiki/Fibonacci_number

   This is deliberately an unnecessary slow and CPU intensive
   implementation where each number in the sequence is calculated recursively
   from scratch.
*/

void fibonacci_slow() {
  int n = 0;
  int f;
  while (true) {
    f = fib(n);
    if (f < 0) {
      // Restart on overflow.
      n = 0;
    }
    printf("sfib(%02d) = %d\n", n, fib(n));
    n = (n + 1) % INT_MAX;
    join();
  }
}

/* Print the Fibonacci number sequence over and over again.

   https://en.wikipedia.org/wiki/Fibonacci_number

   This implementation is much faster than fibonacci().
*/
void fibonacci_fast() {
  int a = 0;
  int b = 1;
  int n = 0;
  int next = a + b;

  while(true) {
    printf(" ffib(%02d) = %d\n", n, a);
    next = a + b;
    a = b;
    b = next;
    n++;
    if (a < 0) {
      // Restart on overflow.
      a = 0;
      b = 1;
      n = 0;
    }
    yield();
  }
}

/* Prints the sequence of magic constants over and over again.

   https://en.wikipedia.org/wiki/Magic_square
*/
void magic_numbers() {
  int n = 3;
  int m;
  while (true) {

    m = (n*(n*n+1)/2);
    if (m > 0) {
      printf(" magic(%d) = %d\n", n, m);
      n = (n+1) % INT_MAX;
    } else {
      // Start over when m overflows.
      n = 3;
    }
    yield();
  }
  //yield();      //commented out this line in order to test a double nonyielding main.
}

//-----------------------------------------------------------------------------------------------

void test1(){
    int i=0;
    while(i<180000000){i++;
    if(i==90000000){
        printf("------Teste1-Y\n");
        yield();
    }
    }

    printf("--Teste1\n");

}
void test2(){
    int i=0;
    while(i<270000000){i++;
    if(i==90000000){
        printf("------Teste2-Y\n");
        yield();
    }
    }

    printf("----Teste2\n");
}
void test3(){
    int i=0;
    while(i<360000000){i++;
    if(i==90000000){
        printf("------Teste3-Y\n");
        yield();
    }
    }

    printf("------Teste3\n");
}

/*******************************************************************************
                                     main()

            Here you should add code to test the Simple Threads API.
********************************************************************************/


int main(int argc, char *argv[]){
  puts("\n==== Test program for the Simple Threads API ====\n");

  // modeAl     0 == FCFS 1 == prioridade 2 == prioridade reverse
  //TimeOut x
  init(atoi(argv[1]), atoi(argv[2])); // Initialization init(int _modeAl, int Time)

  spawn(&magic_numbers);
  spawn(&numbers);
  spawn(&fibonacci_fast);
  spawn(&letters);


  start();
}
