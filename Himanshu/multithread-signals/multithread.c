#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include<sys/wait.h> 
#include "multithread.h"
#include <time.h>
#include <sys/time.h>
#include <sys/timeb.h>

const char *shmpath = "/multiprocess";

time_t countUsr1 = 0, countUsr2 = 0;
time_t  lastSigUsr1, lastSigUsr2;
double sumUsr1 = 0, sumUsr2 = 0;
struct shmbuf *sharedObject;

pthread_t tids[NUMBER_OF_THREADS];

struct shmbuf* initSharedObject(){
    struct shmbuf *shmp = calloc(1, sizeof(struct shmbuf));

    shmp->sigUser1SentCount = 0;
    shmp->sigUser2SentCount = 0;
    shmp->sigUser1ReceivedCount = 0;
    shmp->sigUser2ReceivedCount = 0;
    shmp->handledCounter = 0;
    shmp->signalsCounter = 0;

    pthread_mutex_init(&shmp->mutex, NULL);
    pthread_mutex_init(&shmp->sigMutex, NULL);
    pthread_mutex_init(&shmp->countMutex, NULL);

    return shmp;
}

void destroySharedObject(){
    pthread_mutex_destroy(&sharedObject->mutex);
    pthread_mutex_destroy(&sharedObject->sigMutex);
    pthread_mutex_destroy(&sharedObject->countMutex);
    free(sharedObject);
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
    sharedObject->handledCounter += number;
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

void sigHandler(int sig)
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

void *reporting_thread(void *arg){
    sigset_t set;
    int sig;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGQUIT);
    while(!sigwait(&set, &sig)){
        
        if(sig == SIGQUIT){
            printf("Killing\n");
            break;
        }
        sigHandled(sig);
    }
}

void *signal_usr1_receiving_thread(void *arg){
    sigset_t set;
    int sig;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGQUIT);
    while(!sigwait(&set, &sig)){
        if(sig == SIGQUIT){
            break;
        }
        sigHandler(sig);
    }
}

void *signal_usr2_receiving_thread(void *arg){
    sigset_t set;
    int sig;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGQUIT);
    while(!sigwait(&set, &sig)){
        if(sig == SIGQUIT){
            break;
        }
        sigHandler(sig);
    }
}

void *signal_generating_thread(void *arg){
    int randomNumber = 0, sigUsr1 = 1, i =0;
    srand(time(NULL));
    //Signal Generating Processes
    
    while(1){
        randomNumber = (rand() % 10) + 1;
        usleep(randomNumber * 1000);
        if(sigUsr1) {
            for(i = 0; i <= 4; i++){
                pthread_kill(tids[i], SIGUSR1);
            }
            incrementSentSigCount(SIGUSR1);
            sigUsr1 = 0;
        } else {
            for(i = 0; i <= 4; i++){
                pthread_kill(tids[i], SIGUSR2);
            }
            sigUsr1 = 1;
            incrementSentSigCount(SIGUSR2);
        }
        pthread_mutex_lock(&sharedObject->countMutex);
        sharedObject->signalsCounter++;                
        pthread_mutex_unlock(&sharedObject->countMutex);
    }

}


int
main(int argc, char *argv[])
{
    int i = 0, sig;
    sigset_t set;
    int sigUsr1 = 1;
    int randomNumber = 0, s = 0;
    sigset_t signals_to_mask;
    sharedObject = initSharedObject();
    struct timeb current;
    ftime(&current);
    lastSigUsr1 = (current.time * 1000) + current.millitm;
    lastSigUsr2 = (current.time * 1000) + current.millitm;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGQUIT);
    s = pthread_sigmask(SIG_BLOCK, &set, NULL);

    for(i = 0; i < NUMBER_OF_THREADS; i++){
        //child
        if (i == 0) {
            pthread_create(&tids[i], NULL, &reporting_thread, NULL);
        } else if ( i >= 1 && i <= 2 ){
            pthread_create(&tids[i], NULL, &signal_usr1_receiving_thread, NULL);
        } else if ( i >= 3 && i <= 4 ){
            pthread_create(&tids[i], NULL, &signal_usr2_receiving_thread, NULL);
        } else {
            pthread_create(&tids[i], NULL, &signal_generating_thread, NULL);
        }
    }

    /*while(1){
        pthread_mutex_lock(&sharedObject->countMutex);
        if(sharedObject->signalsCounter >= NUMBER_OF_SIGNALS){
            raise(SIGTERM);
            break;
        }
        pthread_mutex_unlock(&sharedObject->countMutex);

    }*/
    sleep(30);
    printf("Killing\n");
    raise(SIGTERM);
    for(i = 0; i < NUMBER_OF_THREADS; i++){
        pthread_join(tids[i], NULL);
    }

    printf("parent exit\n");

    destroySharedObject();

    exit(EXIT_SUCCESS);
}