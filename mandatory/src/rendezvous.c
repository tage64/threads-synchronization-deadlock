/**
 * Rendezvous
 *
 * Two threads executing chunks of work in lock step.
 *
 * Author: Nikos Nikoleris <nikos.nikoleris@it.uu.se>
 *
 */

#include <stdio.h>     /* printf() */
#include <stdlib.h>    /* abort(), [s]rand() */
#include <unistd.h>    /* sleep() */
#include <pthread.h>   /* pthread_...() */

#include "psem.h"

#define LOOPS 10
#define NTHREADS 3
#define MAX_SLEEP_TIME 3

// Declare global semaphore variables. Note, they must be initialized before use.
psem_t *semA;
psem_t *semB;

void *
threadA(void *param __attribute__((unused)))
{
    int i;

    for (i = 0; i < LOOPS; i++) {
        printf("A%d\n", i);
        sleep(rand() % MAX_SLEEP_TIME);
        psem_signal(semA);
        psem_wait(semB);
    }

    pthread_exit(0);
}


void *
threadB(void *param  __attribute__((unused)))
{
    int i;

    for (i = 0; i < LOOPS; i++) {
        printf("B%d\n", i);
        sleep(rand() % MAX_SLEEP_TIME);
        psem_signal(semB);
        psem_wait(semA);
    }

    pthread_exit(0);
}

int
main()
{
    pthread_t tidA, tidB;

    semA = psem_init(0);
    semB = psem_init(0);
    
    srand(time(NULL));
    pthread_setconcurrency(3);

    if (pthread_create(&tidA, NULL, threadA, NULL) ||
        pthread_create(&tidB, NULL, threadB, NULL)) {
        perror("pthread_create");
        abort();
    }
    if (pthread_join(tidA, NULL) != 0 ||
        pthread_join(tidB, NULL) != 0) {
        perror("pthread_join");
        abort();
    }

    psem_destroy(semA);
    psem_destroy(semB);

    return 0;
}
