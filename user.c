/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
 /**
    @file user.c
    @brief contiene funzioni utili per la gestione dei dati degli utenti in Chatty

    @author
 */

#include <stdlib.h>
#include <stdio.h>
#include <user.h>
#include <message.h>
#include <string.h>
#include <config.h>

int user_data_init(user_data_t *usrdt, int fd, int MaxMsgSize, int MaxFileSize, int MaxHistMsgs){ // da completare

    //usrdt = (user_data_t *)malloc(sizeof(user_data_t));
    //if (!usrdt) return -1;


    usrdt->fd = fd;
    if (fd < 0) return -1;


    usrdt->hist = (history_t *)malloc(MaxHistMsgs * sizeof(history_t));
    if (!(usrdt->hist)) return -1;


    for (size_t i = 0; i < MaxHistMsgs; i++) {

        ((usrdt->hist)[i]).msg_history = (message_t *)malloc(sizeof(message_t));

        ((usrdt->hist)[i]).msg_history->data.buf = (char *)malloc(MaxMsgSize * sizeof(char));

        ((usrdt->hist)[i]).read_bit = 1; //permette agli elementi di essere sovrascritti
    }

    usrdt->index_to_write = 0;
    usrdt->max_hist_msgs = MaxHistMsgs;

    return 0;
}


int add_to_history(user_data_t *usrdt, message_t *msg_to_add, int read){

    message_t msg;

    strncpy(msg.hdr.sender, msg_to_add->hdr.sender, MAX_NAME_LENGTH + 1);
    msg.data.hdr.len = msg_to_add->data.hdr.len;

    msg.data.buf = (char *)malloc(msg.data.hdr.len * sizeof(char));

    msg.data.buf = strncpy(msg.data.buf, msg_to_add->data.buf, msg.data.hdr.len);


    //se una scrittura rischiasse di sovrascrivere dei messaggi non ancora letti
    if (0 == ((usrdt->hist)[usrdt->index_to_write]).read_bit){
        printf("inserimento nella history fallito, lo spazio e' occupato da tutti messaggi ancora da leggere\n");
        return -1;
    }

    //aggiungo il messaggio alla history
    ((usrdt->hist)[usrdt->index_to_write]).msg_history = &msg;
    ((usrdt->hist)[usrdt->index_to_write]).read_bit = read;


    usrdt->index_to_write = (usrdt->index_to_write + 1) % (usrdt->max_hist_msgs);

    return 0;
}
