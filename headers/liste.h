/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file liste.h
 * @brief contiene funzioni utili per la gestione della lista di utenti connessi in chatty
 *
 * @author Andrea Pelosi 547596
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <pthread.h>

/**
    @struct node
    @brief nodo della Lista

    @param fd       file descriptor associato al nodo della lista
    @param word     nome utente associato al nodo della lista
    @param next     puntatore al nodo successivo della lista
*/
typedef struct node{
    int fd;
    char *word;
    struct node *next;
} node_t;

/**
    @struct list
    @brief lista degli utenti connessi

    @param list     puntatore alla lista
    @param numb_elems       numero di elementi della lista
    @param mtex_list        lock per la mutua esclusione
*/
typedef struct list{
    node_t *list;
    int numb_elems;
    pthread_mutex_t mtex_list;
} list_t;


/**
    @function createList
    @brief crea una lista vuota

    @return puntatore alla lista creata
*/
list_t *createList();


/**
    @function destroyList
    @brief libera tutta la memoria allocata dalla lista

    @param L        puntatore alla lista da liberare
*/
void destroyList(list_t *L);


/**
    @function insertListHead
    @brief inserisce un utente nella lista in testa

    @param L        puntatore alla lista
    @param str      nome utente
    @param fd       fd col quale sta comunicando l'utente

    @return         puntatore alla nuova lista
*/
list_t *insertListHead(list_t *L, const char *str, int fd);


/**
    @function listFind
    @brief cerca l'utente str nella lista

    @param L        puntatore alla lista
    @param str      nome utente da cercare nella lista

    @return         nodo dove si trova str se presente, NULL altrimenti
*/
node_t *listFind(list_t *L, const char *str);


/**
    @function deleteNameFromList
    @brief elimina dalla lista il nodo contenente l'utente str

    @param L        puntatore alla lista
    @param str      nome utente

    @return         puntatore alla nuova lista
*/
list_t *deleteNameFromList(list_t *L, const char *str);


/**
    @function deleteFdFromList
    @brief elimina dalla lista il nodo contenente il file descriptor fd

    @param L        puntatore alla lista
    @param fd       file descriptor

    @return         puntatore alla nuova lista
*/
list_t *deleteFdFromList(list_t *L, int fd);


/**
    @function toBuf
    @brief concatena i nomi degli utenti dei nodi della lista in un'unica stringa

    @param L        puntatore alla lista

    @return stringa cosi' formata
*/
char *toBuf(list_t *L);
