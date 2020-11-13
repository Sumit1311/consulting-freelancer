#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include<sys/wait.h> 
#include "multiprocess.h"
#include <time.h>
#include <sys/time.h>
#include <sys/timeb.h>

const char *shmpath = "/multiprocess";

time_t countUsr1 = 0, countUsr2 = 0;
time_t  lastSigUsr1, lastSigUsr2;
double sumUsr1 = 0, sumUsr2 = 0;
struct shmbuf *sharedObject;

int shmFd;

struct shmbuf * getSharedObject() {
        //Signal Generating Processes
    int fd = shm_open(shmpath, O_CREAT | O_RDWR,
                        S_IRUSR | S_IWUSR);
    
    if (fd == -1)
        errExit("shm_open");

    /* Map the object into the caller's address space */

    struct shmbuf *shmp = mmap(NULL, sizeof(struct shmbuf),
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);

    if (shmp == MAP_FAILED)
        errExit("mmap");

    shmFd = fd;
    return shmp;
}

struct shmbuf* initSharedObject(){
        /* Create shared memory object and set its size to the size
       of our structure */

    int fd = shm_open(shmpath, O_CREAT | O_RDWR,
                                 S_IRUSR | S_IWUSR);
    if (fd == -1)
        errExit("shm_open");

    if (ftruncate(fd, sizeof(struct shmbuf)) == -1)
        errExit("ftruncate");

    /* Map the object into the caller's address space */

    struct shmbuf *shmp = mmap(NULL, sizeof(struct shmbuf),
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED, fd, 0);

    if (shmp == MAP_FAILED)
        errExit("mmap");

    /* Initialize semaphores as process-shared, with value 0 */

    if (sem_init(&shmp->sem1, 1, 1) == -1)
        errExit("sem_init-sem1");

    shmFd = fd;

    shmp->sigUser1SentCount = 0;
    shmp->sigUser2SentCount = 0;
    shmp->sigUser1ReceivedCount = 0;
    shmp->sigUser2ReceivedCount = 0;
    shmp->signalsCounter = 0;
    shmp->handledCounter = 0;
    shmp->done = 0;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shmp->mutex, &attr);
    pthread_mutex_init(&shmp->sigMutex, &attr);
    pthread_mutex_init(&shmp->countMutex, &attr);

    return shmp;
}

void destroySharedObject(){
        /* Unlink the shared memory object. Even if the peer process
       is still using the object, this is okay. The object will
       be removed only after all open references are closed. */
    close(shmFd);
    shm_unlink(shmpath);
    sem_destroy(&sharedObject->sem1);
    pthread_mutex_destroy(&sharedObject->mutex);
    pthread_mutex_destroy(&sharedObject->sigMutex);
    pthread_mutex_destroy(&sharedObject->countMutex);
    munmap(sharedObject, sizeof(struct shmbuf));
}


void incrementReceivedSigCount(int sigType){
    /*if (sem_wait(&sharedObject->sem1) == -1)
        errExit("sem_wait");*/
    
    if (pthread_mutex_lock(&sharedObject->mutex) == -1)
        errExit("sem_wait");
    
    if(sigType == SIGUSR1){
        sharedObject->sigUser1ReceivedCount++;
    } else {
        sharedObject->sigUser2ReceivedCount++;
    }

    /*if (sem_post(&sharedObject->sem1) == -1)*/
    if (pthread_mutex_unlock(&sharedObject->mutex) == -1)
        errExit("sem_post");
}

void incrementSentSigCount(int sigType){
    /*if (sem_wait(&sharedObject->sem1) == -1)
        errExit("sem_wait");*/
    if (pthread_mutex_lock(&sharedObject->mutex) == -1)
        errExit("sem_wait");
    
    if(sigType == SIGUSR1){
        sharedObject->sigUser1SentCount++;
    } else {
        sharedObject->sigUser2SentCount++;
    }

    /*if (sem_post(&sharedObject->sem1) == -1)*/
    if (pthread_mutex_unlock(&sharedObject->mutex) == -1)
        errExit("sem_post");
    
}

int incrementSignalsHandled(int number){
    pthread_mutex_lock(&sharedObject->sigMutex);
    sharedObject->handledCounter+=number;
    pthread_mutex_unlock(&sharedObject->sigMutex);
}

int decrementSignalsHandled(int number){    
    pthread_mutex_lock(&sharedObject->sigMutex);
    sharedObject->handledCounter -= number;
    pthread_mutex_unlock(&sharedObject->sigMutex);
}

int getSignalsHandled(){
    int h = 0;
    pthread_mutex_lock(&sharedObject->sigMutex);
    h = sharedObject->handledCounter;
    pthread_mutex_unlock(&sharedObject->sigMutex);
    return h;
}

void sigUsr1Handler(int sig)
{
    incrementReceivedSigCount(sig);
}

void sigUsr2Handler(int sig)
{
    incrementReceivedSigCount(sig);
}

void getCurrentTime(char *curr) {
    time_t timer;
    struct tm* tm_info;

    timer = time(NULL);
    tm_info = localtime(&timer);

    strftime(curr, 26, "%Y-%m-%d %H:%M:%S", tm_info);
}


void sigHandled(int sig){
    int sentCountSigUsr1 = 0, sentCountSigUsr2 = 0, receivedCountSigUsr1 = 0, receivedCountSigUsr2 = 0;
    char currentTime[26];
    struct timeb current;
    int shouldPrint = 0;
    float avgUsr1 = 0, avgUsr2 = 0;
    pthread_mutex_lock(&sharedObject->sigMutex);
    sharedObject->handledCounter += 1;
    if(sig == SIGUSR1){
        ftime(&current);
        sumUsr1 +=  (((current.time * 1000) + current.millitm) - lastSigUsr1);
        lastSigUsr1 = (current.time * 1000) + current.millitm;
        countUsr1++;
    } else {
        ftime(&current);
        sumUsr2 +=  (((current.time * 1000) + current.millitm) - lastSigUsr2);
        lastSigUsr2 = (current.time * 1000) + current.millitm;
        countUsr2++;
    }
    if(sharedObject->handledCounter == 10){
        avgUsr1 = 0;
        avgUsr2 = 0;
        avgUsr1 =(sumUsr1/countUsr1);
        avgUsr2 =(sumUsr2/countUsr2);

        /*if (sem_wait(&sharedObject->sem1) == -1)
            errExit("sem_wait");*/
        if (pthread_mutex_lock(&sharedObject->mutex) == -1)
            errExit("sem_wait");
        sentCountSigUsr1 = sharedObject->sigUser1SentCount;
        sentCountSigUsr2 = sharedObject->sigUser2SentCount;
        receivedCountSigUsr1 = sharedObject->sigUser1ReceivedCount;
        receivedCountSigUsr2 = sharedObject->sigUser2ReceivedCount;
        /*if (sem_post(&sharedObject->sem1) == -1)*/
        if (pthread_mutex_unlock(&sharedObject->mutex) == -1)
            errExit("sem_post");
        shouldPrint = 1;
        sharedObject->handledCounter = 0;
        sumUsr1 = 0;
        countUsr1 = 0;
        sumUsr2 = 0;
        countUsr2 = 0;
    }
    pthread_mutex_unlock(&sharedObject->sigMutex);
    if(shouldPrint){
        getCurrentTime(currentTime);
        printf("===============================================\n");
        printf("%s\n",currentTime);
        printf("SIGUSR1 sent : %d\n", sentCountSigUsr1);
        printf("SIGUSR1 received : %d\n", receivedCountSigUsr1);
        printf("SIGUSR1 Average  : %f ms\n", avgUsr1);
        printf("SIGUSR2 sent : %d\n", sentCountSigUsr2);
        printf("SIGUSR2 received : %d\n", receivedCountSigUsr2);
        printf("SIGUSR2 Average  : %f ms\n", avgUsr2);
        printf("===============================================\n");
    }
}

void dummy(int sig) {
    printf("dummy\n");
}

int
main(int argc, char *argv[])
{
    int i = 0, sig;
    pid_t childPids[NUMBER_OF_PROCESSES];
    pid_t pid;
    int sigUsr1 = 1;
    int randomNumber = 0, s =0;
    sigset_t set, signals_to_mask;
    sharedObject = initSharedObject();
    struct timeb current;
    ftime(&current);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    lastSigUsr1= (current.time * 1000) + current.millitm;
    lastSigUsr2= (current.time * 1000) + current.millitm;

    for(i = 0; i < NUMBER_OF_PROCESSES; i++){
        pid = fork();
        if(pid == 0) {
            sharedObject = getSharedObject(); 
            //child
            if (i == 0) {
                //Reporting Process
                signal(SIGUSR1, &sigHandled);
                signal(SIGUSR2, &sigHandled);
                while(1){}
                /*sigfillset(&signals_to_mask);
                sigprocmask(SIG_SETMASK, &signals_to_mask, NULL);
                sigemptyset(&set);
                sigaddset(&set, SIGUSR2);
                sigaddset(&set, SIGUSR1);
                while(!sigwait(&set, &sig)){
                    sigHandled(sig);
                }*/
            } else if ( i == 1 || i == 2 ){
                //Signal handling
                signal(SIGUSR2, SIG_IGN);
                signal(SIGUSR1, &sigUsr1Handler);
                while(1){};
                /*sigfillset(&signals_to_mask);
                sigprocmask(SIG_SETMASK, &signals_to_mask, NULL);
                sigemptyset(&set);
                sigaddset(&set, SIGUSR2);
                while(!sigwait(&set, &sig)){
                    sigUsr2Handler(sig);
                }*/
            } else if ( i == 3 || i == 4 ){
                //Signal handling
                signal(SIGUSR1, SIG_IGN);
                signal(SIGUSR2, &sigUsr2Handler);
                while(1){   };
                /*sigfillset(&signals_to_mask);
                sigprocmask(SIG_SETMASK, &signals_to_mask, NULL);
                sigemptyset(&set);
                sigaddset(&set, SIGUSR1);
                while(!sigwait(&set, &sig)){
                    sigUsr1Handler(sig);
                }*/
            } 
            else {
                srand(time(NULL));
                //Signal Generating Processes
                
                while(1){
                    randomNumber = (rand() % 10) + 1;
                    usleep(randomNumber * 1000);

                    signal(SIGUSR1, SIG_IGN);
                    signal(SIGUSR2, SIG_IGN);
                    if(sigUsr1) {
                        kill(0, SIGUSR1);
                        incrementSentSigCount(SIGUSR1);
                        sigUsr1 = 0;
                    } else {
                        kill(0, SIGUSR2);
                        sigUsr1 = 1;
                        incrementSentSigCount(SIGUSR2);
                    }
                    pthread_mutex_lock(&sharedObject->countMutex);
                    sharedObject->signalsCounter++;
                    if(sharedObject->done){
                        kill(0, SIGTERM);
                    }                
                    pthread_mutex_unlock(&sharedObject->countMutex);
                }
            }
            break;
        }
    }
    
    if(pid != 0){
        /*sleep(30);
        pthread_mutex_lock(&sharedObject->countMutex);
        sharedObject->done = 1;
        pthread_mutex_unlock(&sharedObject->countMutex);*/
        while(1){
            pthread_mutex_lock(&sharedObject->countMutex);
            if(sharedObject->signalsCounter >= NUMBER_OF_SIGNALS){
                sharedObject->done = 1;
            }
            pthread_mutex_unlock(&sharedObject->countMutex);
        }
    
        for(i = 0; i < NUMBER_OF_PROCESSES; i++){
            wait(NULL);
        }
        printf("parent exit\n");
    }

    destroySharedObject();

    exit(EXIT_SUCCESS);
}