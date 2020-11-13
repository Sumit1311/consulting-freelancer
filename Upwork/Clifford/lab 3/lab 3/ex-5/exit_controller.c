/**
 * CS2106 AY 20/21 Semester 1 - Lab 3
 *
 * This file contains declarations. You should only modify the fifo_sem_t struct,
 * as the method signatures will be needed to compile with the runner.
 */
#include "exit_controller.h"
#include <stdlib.h>

void exit_controller_init(exit_controller_t *exit_controller, int no_of_priorities)
{
    sem_init(&exit_controller->exitQueueSem, 0, 1);     //Mutex lock for manipulating the priority coutners
    sem_init(&exit_controller->exitNotification, 0, 1); //Semaphore for synchronising the exit queue thread execution
    exit_controller->priorityCount = malloc(sizeof(int) * no_of_priorities);
}

void exit_controller_wait(exit_controller_t *exit_controller, int priority)
{
    int haveHighPriorityProcess = 0;
    while (1)
    {
        haveHighPriorityProcess = 0;
        sem_wait(&exit_controller->exitNotification); //Wait for the notification to enter exit line
        sem_wait(&exit_controller->exitQueueSem);
        exit_controller->priorityCount[priority]++; //Increment the process count with given priority
        for (int i = 0; i < priority; i++)
        {
            if (exit_controller->priorityCount[i] != 0) //Check if process with higher priority exists
            {
                haveHighPriorityProcess = 1;
                break;
            }
        }
        sem_post(&exit_controller->exitQueueSem);
        if (haveHighPriorityProcess == 0) //If higher priority process does not exist, break
        {
            break;
        }
        else
        {
            sem_post(&exit_controller->exitNotification); //If other higher priority process exists wakeup other threads
        }
    }
}

void exit_controller_post(exit_controller_t *exit_controller, int priority)
{
    sem_wait(&exit_controller->exitQueueSem);
    exit_controller->priorityCount[priority]--; //decrement the process count for given priority
    sem_post(&exit_controller->exitQueueSem);
    sem_post(&exit_controller->exitNotification);
}

void exit_controller_destroy(exit_controller_t *exit_controller)
{
    sem_destroy(&exit_controller->exitNotification);
    sem_destroy(&exit_controller->exitQueueSem);
    free(exit_controller->priorityCount);
}
