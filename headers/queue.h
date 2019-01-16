/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
    @file queue.h
    @brief definizione di strutture e funzioni per gestire una coda di fd

    @author
*/
#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

/**
    @struct queue_t
    @brief coda di file descriptors

    @var fds       array che contiene i file descriptors
    @var first      indice del primo spazio libero della coda
    @var back       indice dell'elemento piu' anziano della coda
    @var length     lunghezza dell'array
    @var mtex        lock per la coda
    @var cond       variabile di condizione per la coda
*/
typedef struct {

    int *fds;
    int first;
    int elder;
    int length;

    pthread_mutex_t mtex;
    pthread_cond_t cond;
} queue_t;

/**
    @function createQ
    @brief crea una coda vuota di tipo queue_t

    @param length       lunghezza desiderata della coda
    @return puntatore a queue_t
*/
queue_t *createQ(int length);


/**
    @function pushQ
    @brief effettua un push sulla coda

    @param Q        coda sulla quale si effettua il push
    @param fd       file descriptor che viene pushato
    @return >=0 (valore del file descriptor pushato)
*/
int pushQ(queue_t* q, int fd);


/**
    @function popQ
    @brief effettua un pop dalla coda, se la coda e' vuota si mette in attesa

    @param Q         coda dalla quale si effettua il pop
    @return >=0 (valore del file descriptor poppato)
*/
int popQ(queue_t *q);


/**
    @function deleteQ
    @brief distrugge la coda e libera la memoria

    @param q         coda da distruggere
*/
void deleteQ(queue_t *q);


#endif
