/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
 /**
 *   @file user.c
 *   @brief contiene funzioni utili per la gestione dei dati degli utenti in Chatty
 *
 * @author Andrea Pelosi 547596
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */

#include <stdlib.h>
#include <stdio.h>
#include <user.h>
#include <message.h>
#include <string.h>
#include <config.h>
#include <pthread.h>
#include <macrothread.h>
#include <icl_hash.h>
#include <connections.h>

int user_data_init(user_data_t *usrdt, char *username, int fd, int MaxMsgSize, int MaxFileSize, int MaxHistMsgs){

    int len = strlen(username);
    usrdt->username = (char *)malloc((len+1) * sizeof(char));
    memset(usrdt->username, 0, (len+1) * sizeof(char));

    if (!usrdt->username) return -1;

    strncpy(usrdt->username, username, strlen(username)+1);

    usrdt->fd = fd;
    if (fd < 0) return -1;

    //alloco lo spazio per un array di coppie (messaggio, bit di lettura)
    usrdt->hist = (history_t *)malloc(MaxHistMsgs * sizeof(history_t));
    memset(usrdt->hist, 0, MaxHistMsgs * sizeof(history_t));

    if (!(usrdt->hist)) return -1;


    for (size_t i = 0; i < MaxHistMsgs; i++) {

        ((usrdt->hist)[i]).msg_history = NULL;
        ((usrdt->hist)[i]).read_bit = 1; //permette agli elementi di essere sovrascritti
    }

    THREAD(pthread_mutex_init(&usrdt->mtex, NULL),"mutex_init in user_data_init");

    usrdt->size = 0;
    usrdt->index_to_write = 0;
    usrdt->max_hist_msgs = MaxHistMsgs;

    return 0;
}


int add_to_history(user_data_t *usrdt, message_t *msg_to_add, int read, int already_locked) {

    if (!already_locked)
        THREAD(pthread_mutex_lock(&usrdt->mtex), "lock in add_to_history");

    //se una scrittura rischia di sovrascrivere dei messaggi non ancora letti
    if (0 == ((usrdt->hist)[usrdt->index_to_write]).read_bit){
        printf("inserimento nella history fallito, lo spazio e' occupato da tutti messaggi ancora da leggere\n");

        if(!already_locked)
            THREAD(pthread_mutex_unlock(&usrdt->mtex), "unlock in add_to_history");
        return -1;
    }

    if (NULL != ((usrdt->hist)[usrdt->index_to_write]).msg_history) {

        free(((usrdt->hist)[usrdt->index_to_write]).msg_history->data.buf);
        free(((usrdt->hist)[usrdt->index_to_write]).msg_history);
    }


    message_t *msg = (message_t *)malloc(sizeof(message_t));
    memset(msg, 0, sizeof(message_t));

    msg->hdr.op = msg_to_add->hdr.op;

    strncpy(msg->hdr.sender, msg_to_add->hdr.sender, MAX_NAME_LENGTH + 1);
    strncpy(msg->data.hdr.receiver, msg_to_add->data.hdr.receiver, MAX_NAME_LENGTH + 1);
    msg->data.hdr.len = msg_to_add->data.hdr.len;

    msg->data.buf = (char *)malloc(msg_to_add->data.hdr.len * sizeof(char));
    memset(msg->data.buf, 0, msg_to_add->data.hdr.len * sizeof(char));

    msg->data.buf = strncpy(msg->data.buf, msg_to_add->data.buf, msg_to_add->data.hdr.len);


    //aggiungo il messaggio alla history
    ((usrdt->hist)[usrdt->index_to_write]).msg_history = msg;
    ((usrdt->hist)[usrdt->index_to_write]).read_bit = read;

    if (usrdt->size < usrdt->max_hist_msgs)
        usrdt->size++;

    usrdt->index_to_write = (usrdt->index_to_write + 1) % (usrdt->max_hist_msgs);

    if (!already_locked)
        THREAD(pthread_mutex_unlock(&usrdt->mtex), "unlock in add_to_history");
    return 0;
}


int add_to_history_all(icl_hash_t *hashtable, char *user, message_t *msg_to_add, int *nonline, int *noffline) {

    int i;
    icl_entry_t *entry;
    char *kp;
    user_data_t *dp;

    message_t *msg = (message_t *)malloc(sizeof(message_t));
    memset(msg, 0, sizeof(message_t));

    setHeader(&msg->hdr, TXT_MESSAGE, user);

    strncpy(msg->hdr.sender, msg_to_add->hdr.sender, MAX_NAME_LENGTH + 1);

    msg->data.hdr.len = msg_to_add->data.hdr.len;

    msg->data.buf = (char *)malloc(msg_to_add->data.hdr.len * sizeof(char));
    memset(msg->data.buf, 0, msg_to_add->data.hdr.len * sizeof(char));

    msg->data.buf = strncpy(msg->data.buf, msg_to_add->data.buf, msg_to_add->data.hdr.len);


    icl_hash_foreach(hashtable, i, entry, kp, dp){ //per ogni elemento della tabella hash

        if (0 == strcmp(kp, user)) { //il mittente non manda il messaggio a se stesso
            continue;
        }

        THREAD(pthread_mutex_lock(&dp->mtex), "lock in add_to_history_all");

        int ans = sendRequest(dp->fd, msg);

        if (-1 == ans) { //il destinatario e' offline

            printf("il destinatario e' offline, aggiungo il messaggio alla lista dei messaggi che deve leggere\n");

            (*noffline)++;

            add_to_history(dp, msg, 0, 1);
            //aggiorno la history del destinatario e marco il messaggio come non letto
            /*if (-1 == add_to_history(dp, msg, 0, 1)){
                THREAD(pthread_mutex_unlock(&dp->mtex), "unlock in add_to_history_all");

                free(msg->data.buf);
                free(msg);
                return 1; //history del destinatario piena, il fd del mittente verra' rimesso in coda
            }*/


        } else { //il destinatario e' online
            printf("il destinatario e' online, messaggio inviato con successo\n");

            (*nonline)++;
            add_to_history(dp, msg, 1, 1);
            //aggiorno la history del destinatario e marco il messaggio come letto
            /*if (-1 == add_to_history(dp, msg, 1, 1)){
                THREAD(pthread_mutex_unlock(&dp->mtex), "unlock in add_to_history_all");

                free(msg->data.buf);
                free(msg);
                return 1; //history del destinatario piena, il fd del mittente verra' rimesso in coda
            }*/
        }
        THREAD(pthread_mutex_unlock(&dp->mtex), "unlock in add_to_history_all");
    }

    free(msg->data.buf);
    free(msg);
    return 0;
}


message_t * retrieve_from_history(user_data_t *usrdt, int index, int *read_now) {

    THREAD(pthread_mutex_lock(&usrdt->mtex), "lock in retrieve_from_history");

    message_t *msg = ((usrdt->hist)[index]).msg_history;
    if (0 == ((usrdt->hist)[index]).read_bit) {
        ((usrdt->hist)[index]).read_bit = 1;
        (*read_now)++;
    }

    THREAD(pthread_mutex_unlock(&usrdt->mtex), "unlock in retrieve_from_history");

    return msg;
}


void update_user_fd(user_data_t *usrdt, int new_fd) {

    THREAD(pthread_mutex_lock(&usrdt->mtex), "lock in update_user_fd");
    usrdt->fd = new_fd;
    THREAD(pthread_mutex_unlock(&usrdt->mtex), "unlock in update_user_fd");
}


void free_userdata(void *usrdt) {

    user_data_t *usrdt1 = (user_data_t *)usrdt;

    THREAD(pthread_mutex_lock(&usrdt1->mtex), "lock in free_hist");

    for (size_t i = 0; i < usrdt1->size; i++) {
        free(((usrdt1->hist)[i]).msg_history->data.buf);
        free(((usrdt1->hist)[i]).msg_history);
    }
    free(usrdt1->username);
    free(usrdt1->hist);

    THREAD(pthread_mutex_unlock(&usrdt1->mtex), "unlock in free_hist");

    free(usrdt);
}
