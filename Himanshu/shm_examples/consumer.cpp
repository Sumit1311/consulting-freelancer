#include <sys/ipc.h>
#include <sys/shm.h>
#include <iostream>

#include "common.h"

int main() {
    /**
     * 1. Get the Shared Memory with shmget
     * 2. Attach Shared Memory using shmat
     * 3. Wait for semaphore to become 1 and pop one element to queue
     * 4. Wait for 30 sec if queue is empty
     * 5. After popping 50 elements consumer should stop
     * 6. Detach the shared memory shmdt
     * 7. Destroy the shared memory shmctl
     */
    cout<<"Hello"<<endl;
}