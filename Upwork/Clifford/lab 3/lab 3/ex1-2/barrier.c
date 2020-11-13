/**
 * CS2106 AY 20/21 Semester 1 - Lab 3
 *
 * This file contains function definitions. Your implementation should go in
 * this file.
 */

#include "barrier.h"

// Initialise barrier here
void barrier_init(barrier_t *barrier, int count)
{
    barrier->count = count;
    barrier->threadCount = 0;
    sem_init(&barrier->counterSem, 0, 1);
    sem_init(&barrier->barrierSem, 0, 0);
}

void barrier_wait(barrier_t *barrier)
{
    int i = 0;

    sem_wait(&barrier->counterSem);
    (barrier->threadCount)++;
    if (barrier->count == barrier->threadCount)
    {
        i = (barrier->count - 1);
        barrier->threadCount = 0;
        sem_post(&barrier->counterSem);
        while (i > 0)
        {
            sem_post(&barrier->barrierSem);
            i--;
        }
    }
    else
    {
        sem_post(&barrier->counterSem);
        sem_wait(&barrier->barrierSem);
    }
}

// Perform cleanup here if you need to
void barrier_destroy(barrier_t *barrier)
{
    sem_destroy(&barrier->counterSem);
    sem_destroy(&barrier->barrierSem);
}