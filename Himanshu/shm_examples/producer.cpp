#include <sys/ipc.h>
#include <sys/shm.h>
#include <iostream>

#include "common.h"

int main() {
    /**
     * 1. Get the Shared Memory with shmget
     * 2. Attach Shared Memory using shmat, wait for semaphore to become 0
     * 3. Initialize the common data structure
     * 4. Lock the mutex and push one element to queue
     * 5. Wait for 30 sec if queue is full
     * 6. After pushing 50 elements producer should stop producing
     * 7. Detach the shared memory shmdt
     */
    cout<<"Hello"<<endl;
}