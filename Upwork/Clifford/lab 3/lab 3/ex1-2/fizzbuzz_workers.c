/**
 * CS2106 AY 20/21 Semester 1 - Lab 3
 *
 * Your implementation should go in this file.
 */
#include "fizzbuzz_workers.h"
#include "barrier.h" // you may use barriers if you think it can help your
                     // implementation
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// declare variables to be used here
barrier_t *fizzBarrier, *buzzBarrier, *fizzBuzzBarrier;
sem_t mutexLock, resumeMutex;
int counter = 1;

void fizzbuzz_init(int n)
{
    fizzBarrier = malloc(sizeof(barrier_t));
    buzzBarrier = malloc(sizeof(barrier_t));
    fizzBuzzBarrier = malloc(sizeof(barrier_t));
    barrier_init(fizzBarrier, 2);
    barrier_init(buzzBarrier, 2);
    barrier_init(fizzBuzzBarrier, 2);
    sem_init(&mutexLock, 0, 1);
    sem_init(&resumeMutex, 0, 0);
}

void num_thread(int n, void (*print_num)(int))
{
    int i = 0;
    while (1)
    {
        sem_wait(&mutexLock);
        if ((counter % 15) == 0)
        {
            sem_post(&mutexLock);
            barrier_wait(fizzBuzzBarrier);
            sem_wait(&resumeMutex);
            sem_wait(&mutexLock);
        }
        else if ((counter % 3) == 0)
        {
            sem_post(&mutexLock);
            barrier_wait(fizzBarrier);
            sem_wait(&resumeMutex);
            sem_wait(&mutexLock);
        }
        else if ((counter % 5) == 0)
        {
            sem_post(&mutexLock);
            barrier_wait(buzzBarrier);
            sem_wait(&resumeMutex);
            sem_wait(&mutexLock);
        }
        else if (counter <= n)
        {
            (*print_num)(counter);
        }
        counter++;
        i = counter;
        sem_post(&mutexLock);

        if (i == (n + 1))
        {
            barrier_wait(fizzBuzzBarrier);
            barrier_wait(fizzBarrier);
            barrier_wait(buzzBarrier);
            break;
        }
    }
}

void fizz_thread(int n, void (*print_fizz)(void))
{
    while (1)
    {
        barrier_wait(fizzBarrier);
        if (counter == (n+1))
        {
            break;
        }
        (*print_fizz)();
        sem_post(&resumeMutex);
    }
}

void buzz_thread(int n, void (*print_buzz)(void))
{
    while (1)
    {
        barrier_wait(buzzBarrier);
        if (counter == (n+1))
        {
            break;
        }
        (*print_buzz)();
        sem_post(&resumeMutex);
    }
}

void fizzbuzz_thread(int n, void (*print_fizzbuzz)(void))
{
    while (1)
    {
        barrier_wait(fizzBuzzBarrier);
        if (counter == (n+1))
        {
            break;
        }
        (*print_fizzbuzz)();
        sem_post(&resumeMutex);
    }
}

void fizzbuzz_destroy()
{
    sem_destroy(&mutexLock);
    sem_destroy(&resumeMutex);
    barrier_destroy(fizzBarrier);
    barrier_destroy(buzzBarrier);
    barrier_destroy(fizzBuzzBarrier);
    free(fizzBarrier);
    free(buzzBarrier);
    free(fizzBuzzBarrier);
}
