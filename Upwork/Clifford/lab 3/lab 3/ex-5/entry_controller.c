/**
 * CS2106 AY 20/21 Semester 1 - Lab 3
 *
 * Your implementation should go in this file.
 */
#include "entry_controller.h"

void entry_controller_init(entry_controller_t *entry_controller, int loading_bays)
{
    sem_init(&entry_controller->entryQueueSem, 0, 1);//Mutex for entry queue
    sem_init(&entry_controller->notifyEntry, 0, loading_bays); //Semaphore to synchronize actions of entry
    entry_controller->lastIndex = 0;//Tail of entry queue
    entry_controller->headIndex = 0;//Head of entry queue
}

void entry_controller_wait(entry_controller_t *entry_controller)
{
    int assignedIndex, currentIndex;
    sem_wait(&entry_controller->entryQueueSem);
    assignedIndex = (entry_controller->lastIndex);//Get an index from entry queue for current thread
    (entry_controller->lastIndex)++;//Increment the tail
    sem_post(&entry_controller->entryQueueSem);
    while (1)
    {
        sem_wait(&entry_controller->notifyEntry);//Wait for notification of exiting the train if no space available on loading bay
        sem_wait(&entry_controller->entryQueueSem);
        currentIndex = entry_controller->headIndex;//Get current index for the train which is allowed in loading bay
        sem_post(&entry_controller->entryQueueSem);
        if (assignedIndex == currentIndex) // If the thread index is the one to be entered to loading bay
        {
            break;
        }
        else
        {
            sem_post(&entry_controller->notifyEntry); // If not the turn of this thread wake other thread and go to sleep
        }
    }
}

void entry_controller_post(entry_controller_t *entry_controller)
{
    sem_wait(&entry_controller->entryQueueSem);
    entry_controller->headIndex++;//Increment the index for train being served
    sem_post(&entry_controller->entryQueueSem);
    sem_post(&entry_controller->notifyEntry);//Notify the next train to enter loading bay
}

void entry_controller_destroy(entry_controller_t *entry_controller)
{
    sem_destroy(&entry_controller->entryQueueSem);
    sem_destroy(&entry_controller->notifyEntry);
}
