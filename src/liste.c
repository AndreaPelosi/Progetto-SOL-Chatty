/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file liste.c
 * @brief contiene funzioni utili per la gestione della lista di utenti connessi in chatty
 *
 * @author Andrea Pelosi 547596
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <config.h>
#include <liste.h>
#include <macrothread.h>
#include <pthread.h>


list_t *createList(){

    list_t *hd = (list_t *)malloc(sizeof(list_t));
    memset(hd, 0, sizeof(list_t));

    pthread_mutex_init(&(hd->mtex_list), NULL);
    hd->numb_elems = 0;
    hd->list = NULL;

    return hd;
}



list_t *insertListHead(list_t *L, const char *str, int fd){

    node_t *curr = (node_t *)malloc(sizeof(node_t));
    memset(curr, 0, sizeof(node_t));

    curr->word = (char *)malloc((MAX_NAME_LENGTH + 1) * sizeof(char));
    memset(curr->word, 0, (MAX_NAME_LENGTH + 1) * sizeof(char));

    int len = strlen(str);

    strncpy(curr->word, str, len);
    for (size_t i = len; i < MAX_NAME_LENGTH + 1; i++) {
        (curr->word)[i] = ' ';
    }

    THREAD(pthread_mutex_lock(&(L->mtex_list)),"lock in insertListHead");
    curr->fd = fd;

    curr->next = L->list;
    L->list = curr;
    L->numb_elems ++;

    THREAD(pthread_mutex_unlock(&(L->mtex_list)),"unlock in insertListHead");
    return L;
}



node_t *listFind(list_t *L, const char *str){

    node_t *curr;
    char *cpy; //variabile che prende ogni utente della lista senza considerare gli spazi
    int len = strlen(str);

    THREAD(pthread_mutex_lock(&(L->mtex_list)),"lock in listFind");
    for (curr = L->list; curr != NULL; curr = curr->next) {

        cpy = (char *)malloc((len + 1) * sizeof(char));
        memset(cpy, 0, (len + 1) * sizeof(char));

        strncpy(cpy, curr->word, len);
        if (0 == strcmp(cpy, str)){ //si controlla se str e' un el. della lista
            free(cpy);
            THREAD(pthread_mutex_unlock(&(L->mtex_list)),"unlock in listFind");
            return curr;
        }
        free(cpy);
    }
    THREAD(pthread_mutex_unlock(&(L->mtex_list)),"unlock in listFind");
    return NULL;
}


list_t *deleteNameFromList(list_t *L, const char *str){

    int trovato = 0;
    node_t *curr, *prec;
    curr = L->list;

    int len = strlen(str);
    char *string = (char *)malloc((MAX_NAME_LENGTH + 1) * sizeof(char));
    memset(string, 0, (MAX_NAME_LENGTH + 1) * sizeof(char));
    strncpy(string, str, len);

    for (size_t i = len; i < MAX_NAME_LENGTH + 1; i++) {
        string[i] = ' ';
    }

    THREAD(pthread_mutex_lock(&(L->mtex_list)),"lock in deleteNameFromList");

    //se la stringa da cancellare e' nel primo nodo della lista
    if (strcmp(curr->word, string) == 0){
        L->numb_elems--;
        L->list = curr->next;

        free(curr->word);
        free(curr);
        free(string);
        curr = NULL;

        THREAD(pthread_mutex_unlock(&(L->mtex_list)),"unlock in deleteNameFromList");
        return L;
    }

    //altrimenti
    while ((NULL != curr) && (0 == trovato)) {
        prec = curr;
        curr = curr->next;
        if (NULL == curr){
            free(string);
            THREAD(pthread_mutex_unlock(&(L->mtex_list)),"unlock in deleteNameFromList");
            return L;
        }
        if (strcmp(curr->word, string) == 0){
            prec->next = curr->next;
            free(curr->word);
            free(curr);
            free(string);
            curr = NULL;
            L->numb_elems--;
            trovato = 1;
        }
    }
    THREAD(pthread_mutex_unlock(&(L->mtex_list)),"unlock in deleteNameFromList");
    return L;
}



list_t *deleteFdFromList(list_t *L, int fd){

    int trovato = 0;
    node_t *curr, *prec;
    curr = L->list;

    THREAD(pthread_mutex_lock(&(L->mtex_list)),"lock in deleteFdFromList");

    //se il fd da cancellare e' nel primo nodo della lista
    if (curr->fd == fd){
        L->numb_elems--;
        L->list = curr->next;

        free(curr->word);
        free(curr);
        curr = NULL;

        THREAD(pthread_mutex_unlock(&(L->mtex_list)),"unlock in deleteFdFromList");
        return L;
    }

    //altrimenti
    while ((NULL != curr) && (0 == trovato)) {
        prec = curr;
        curr = curr->next;
        if (NULL == curr){
            THREAD(pthread_mutex_unlock(&(L->mtex_list)),"unlock in deleteFdFromList");
            return L;
        }
        if (curr->fd == fd){
            prec->next = curr->next;
            free(curr->word);
            free(curr);
            curr = NULL;
            L->numb_elems--;
            trovato = 1;
        }
    }

    THREAD(pthread_mutex_unlock(&(L->mtex_list)),"unlock in deleteFdFromList");
    return L;
}



void destroyList(list_t *L){

    node_t *curr, *prec;

    curr = L->list;

    while (NULL != curr) {
        prec = curr;
        curr = curr->next;
        free(prec->word);
        free(prec);
    }

    pthread_mutex_destroy(&(L->mtex_list));
    free(L);
}


char *toBuf(list_t *L) {
    char *buffer = (char *)malloc((L->numb_elems * (MAX_NAME_LENGTH + 1) + 1) * sizeof(char));
    memset(buffer, 0, (L->numb_elems * (MAX_NAME_LENGTH + 1) + 1) * sizeof(char));

    char *aux = buffer;
    node_t *curr = L->list;

    THREAD(pthread_mutex_lock(&(L->mtex_list)),"lock in toBuf");

	for (size_t i = 0; i < L->numb_elems; i++) {
			strncpy(aux, curr->word, MAX_NAME_LENGTH + 1);
			aux += MAX_NAME_LENGTH;
            *aux = '\0';
            aux++;
            curr = curr->next;
	}
    buffer[L->numb_elems * (MAX_NAME_LENGTH + 1)] = '\0'; //aggiungo il terminatore alla stringa

    THREAD(pthread_mutex_unlock(&(L->mtex_list)),"unlock in toBuf");

    return buffer;
}
