/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
    @file queue.c
    @brief implementazione delle funzioni per la gestione di una coda

    @author
*/

#include <queue.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
//#include <stdio.h>
#include <macrothread.h>


//static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

queue_t *createQ(int length){
    queue_t *queue;

    queue = (queue_t *)malloc(sizeof(queue_t));
    queue->fds = (int *)malloc(length * sizeof(int));
    for (size_t i = 0; i < length; i++) { //quando la coda e' logicamente vuota,
        (queue->fds)[i] = -1; //tutte le posizioni dell'array contengono -1
    }

    THREAD(pthread_mutex_init(&queue->mtex, NULL),"mutex_init in createQ");
    THREAD(pthread_cond_init(&queue->cond, NULL),"cond_init in createQ");

    queue->first = 0;
    queue->elder = 0; // quando la coda e' vuota ha lo stesso valore di first

    queue->length = length;

    //queue->mtex = PTHREAD_MUTEX_INITIALIZER; //controllare se e' corretto
    //queue->cond = PTHREAD_COND_INITIALIZER;

    return queue;
}


int pushQ(queue_t* q, int fd){

    THREAD(pthread_mutex_lock(&q->mtex), "lock in pushQ");
    while ((0 == (q->first - q->elder)) && (-1 != (q->fds)[q->first])) { //verificare se in realta basta soltanto la seconda clausola dell'if
        //printf("coda piena, in attesa push in attesa\n");
        //fflush(stdout);
        THREAD(pthread_cond_wait(&q->cond, &q->mtex), "wait in pushQ");
    }

    int val = fd;
    (q->fds)[q->first] = fd;
    q->first = (q->first + 1) % (q->length);

    THREAD(pthread_cond_signal(&q->cond),"signal in pushQ");
    THREAD(pthread_mutex_unlock(&q->mtex),"unlock in pushQ");

    return val;
}


int popQ(queue_t *q){

    THREAD(pthread_mutex_lock(&q->mtex), "lock in popQ");
    while ((0 == (q->first - q->elder)) || (-1 == (q->fds)[q->elder])) {
        THREAD(pthread_cond_wait(&q->cond, &q->mtex), "wait in popQ");
    }

    int fd = (q->fds)[q->elder];
    (q->fds)[q->elder] = -1;
    q->elder = (q->elder + 1) % (q->length);
    printf("sto facendo la pop del fd %d\n", fd);

    THREAD(pthread_cond_signal(&q->cond),"signal in popQ");
    THREAD(pthread_mutex_unlock(&q->mtex),"unlock in popQ");

    return fd;
}


void deleteQ(queue_t *q){
    free(q->fds);

    THREAD(pthread_mutex_destroy(&q->mtex),"mutex_destroy in deleteQ");
    THREAD(pthread_cond_destroy(&q->cond),"cond_destroy in deleteQ");

    free(q);
}
