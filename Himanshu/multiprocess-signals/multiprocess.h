#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUMBER_OF_PROCESSES 8

#define NUMBER_OF_SIGNALS 100000

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)
#define BUF_SIZE 1024   /* Maximum size for exchanged string */
/* Define a structure that will be imposed on the shared
   memory object */
struct shmbuf {
    sem_t  sem1;            /* POSIX unnamed semaphore */
    pthread_mutex_t mutex;
    pthread_mutex_t sigMutex;
    pthread_mutex_t countMutex;
    int done;
    int handledCounter;
    int signalsCounter;
    int sigUser1SentCount;
    int sigUser2SentCount;
    int sigUser1ReceivedCount;
    int sigUser2ReceivedCount;
};