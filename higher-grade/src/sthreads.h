#ifndef STHREADS_H
#define STHREADS_H

/* On Mac OS (aka OS X) the ucontext.h functions are deprecated and requires the
   following define.
*/
#define _XOPEN_SOURCE 700

/* On Mac OS when compiling with gcc (clang) the -Wno-deprecated-declarations
   flag must also be used to suppress compiler warnings.
*/

#include <ucontext.h>

/* A thread can be in one of the following states. */
typedef enum {running, ready, waiting, terminated} state_t;

/* Thread ID. */
typedef int tid_t;

typedef struct thread thread_t;

/* Data to manage a single thread should be kept in this structure. Here are a few
   suggestions of data you may want in this structure but you may change this to
   your own liking.
*/
struct thread {
  tid_t tid;
  state_t state;
  ucontext_t ctx;

  // If this thread is ready, this is the id to the next thread in the ready queue.
  // If this thread is waiting, this is the id to the next thread waiting on the same resource.
  // -1 Indicates this is the last.
  tid_t next;

  // The first thread that is waiting for this thread to terminate.
  // If more than one thread is waiting on this thread, they are connected in a linked list by the `next` field.
  tid_t first_join_thread;
};


/*******************************************************************************
                               Simple Threads API

You may add or change arguments to the functions in the API. You may also add
new functions to the API.
********************************************************************************/


/* Initialization

   Initialization of global thread management data structures. A user program
   must call this function exactly once before calling any other functions in
   the Simple Threads API.

   Returns 1 on success and a negative value on failure.
*/
int init();

/* Creates a new thread executing the start function.

   start - a function with zero arguments returning void.

   On success the positive thread ID of the new thread is returned. On failure a
   negative value is returned.
*/
tid_t spawn(void (*start)());

/* Cooperative scheduling

   If there are other threads in the ready state, a thread calling yield() will
   trigger the thread scheduler to dispatch one of the threads in the ready
   state and change the state of the calling thread from running to ready.
*/
void  yield();

/* Thread termination

   A thread calling done() will change state from running to terminated. If
   there are other threads waiting to join, these threads will change state from
   waiting to ready. Next, the thread scheduler will dispatch one of the ready
   threads.
*/
void  done();

/* Join with a terminated thread

   The join() function waits for the specified thread to terminate.
   If the specified thread has already terminated, join() should return immediately.

   A thread calling join(thread) will be suspended and change state from running to
   waiting and trigger the thread scheduler to dispatch one of the ready
   threads. The suspended thread will change state from waiting to ready once the thread with
   thread id thread calls done and join() should then return the thread id of the
   terminated thread.
*/
tid_t join(tid_t thread);

#endif
