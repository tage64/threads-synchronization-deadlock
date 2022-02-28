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

/* Stack size for each context. */
#define STACK_SIZE SIGSTKSZ*100

// Maximum number of threads
#define MAX_NO_THREADS 64

/*******************************************************************************
                             Global data structures

                Add data structures to manage the threads here.
********************************************************************************/

// Array of threads
thread_t threads[MAX_NO_THREADS];

// Index of the next availlable thread id.
// -1 if all thread slots are occupied.
tid_t next_availlable_tid = 0;

// Head of the ready queue. -1 indicates there're no ready threads.
tid_t ready_queue_first = -1;

// Last thread in ready queue. -1 indicates there're no ready threads.
tid_t ready_queue_last = -1;

tid_t running_thread;

// Whether the sthreads machinary is initialized
bool sthreads_initialized = false;




/*******************************************************************************
                             Auxiliary functions

                      Add internal helper functions here.
********************************************************************************/

/* Initialize a context.

   ctxt - context to initialize.

   next - successor context to activate when ctx returns. If NULL, the thread
          exits when ctx returns.
 */
void init_context(ucontext_t *ctx, ucontext_t *next) {
  /* Allocate memory to be used as the stack for the context. */
  void *stack = malloc(STACK_SIZE);

  if (stack == NULL) {
    perror("Allocating stack");
    exit(EXIT_FAILURE);
  }

  if (getcontext(ctx) < 0) {
    perror("getcontext");
    exit(EXIT_FAILURE);
  }

  /* Before invoking makecontext(ctx), the caller must allocate a new stack for
     this context and assign its address to ctx->uc_stack, and define a successor
     context and assigns address to ctx->uc_link.
  */

  ctx->uc_link           = next;
  ctx->uc_stack.ss_sp    = stack;
  ctx->uc_stack.ss_size  = STACK_SIZE;
  ctx->uc_stack.ss_flags = 0;
}

void ready_queue_append(tid_t tid) {
  if (ready_queue_last < 0)
    ready_queue_first = tid;
  else
    threads[ready_queue_last].next = tid;
  ready_queue_last = tid;
  threads[tid].state = ready;
  threads[tid].next = -1;
}

void make_running(tid_t tid) {
  if (running_thread == tid)
    return;
  
  thread_t *current_running = &threads[running_thread];
  thread_t *new_running = &threads[tid];
  new_running->state = running;

  // Now, we should swap the contexts but we also need to swap the pointers to the contexts.
  ucontext_t running_ctx = current_running->ctx;
  current_running->ctx = new_running->ctx;
  new_running->ctx = running_ctx;

  if (swapcontext(&new_running->ctx, &current_running->ctx) < 0) {
    perror("swapcontext");
    exit(EXIT_FAILURE);
  }
}

void select_next_ready() {
  tid_t tid = ready_queue_first;
  if (tid < 0)
    return;
  ready_queue_first = threads[tid].next;
  make_running(tid);
}

void terminate_thread(tid_t tid) {
  thread_t *thread = &threads[tid];
  thread->state = terminated;
  if (thread->first_join_thread >= 0) {
    // Loop threw all waiting thread and change their state from waiting to ready.
    tid_t join_thread = thread->first_join_thread;
    while (join_thread >= 0) {
      ready_queue_append(join_thread);
    }
  }
  select_next_ready();
}

void run_thread(tid_t tid, void (*start)()) {
  start();
  terminate_thread(tid);
}

/*******************************************************************************
                    Implementation of the Simple Threads API
********************************************************************************/


int  init(){
  // We will create a main thread from the current context.
  ucontext_t main_context;
  if (getcontext(&main_context) < 0) {
    perror("getcontext");
    exit(EXIT_FAILURE);
  }
  if (sthreads_initialized) {
    // We've returned from getcontext the second time after calling setcontext()
    return 1;
  }

  // Create the main thread
  threads[0] = (thread_t){
    .tid = 0,
    .state = running,
    .ctx = main_context,
    .first_join_thread = -1,
  };
  running_thread = 0;
  next_availlable_tid++;
  sthreads_initialized = true;
  setcontext(&main_context);
  
  // Execution of the main thread failed.
  return -1;
}


tid_t spawn(void (*start)()){
  if (next_availlable_tid < 0)
    return -1;
  tid_t tid = next_availlable_tid;
  ucontext_t ctx;
  init_context(&ctx, NULL);
  makecontext(&ctx, (void (*)())run_thread, 2, tid, start);
  threads[tid] = (thread_t){
    .tid = tid,
    .ctx = ctx,
    .first_join_thread = -1,
    .state = running
  };

  ready_queue_append(running_thread);
  make_running(tid);
  return tid;
}

void yield(){
  ready_queue_append(running_thread);
  select_next_ready();
}

void  done(){
}

tid_t join(tid_t thread) {
  return -1;
}
