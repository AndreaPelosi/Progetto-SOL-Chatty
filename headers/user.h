/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
 /**
    @file user.h
    @brief contiene strutture e funzioni utili per la gestione dei dati degli utenti in Chatty

    @author
 */
#ifndef USER_H
#define USER_H

#include <message.h>



/**
    @struct history
    @brief contiene un messaggio dello storico e un bit che certifica se e' stato letto dal client

    @param msg_history       messaggio ricevuto
    @param read_bit     bit che certifica se il messaggio e' stato letto dal client
*/
typedef struct history {
        message_t *msg_history;
        int read_bit;
} history_t;


/**
    @struct userdata
    @brief contiene le informazioni relative ad un singolo utente di Chatty

    @param fd       file descriptor associato all'utente
    @param
    @param hist      history dei messaggi ricevuti
    @param index_to_write       indice dell'elemento di hist in cui aggiungere il prossimo messaggio
    @param max_hist_msgs        dimensione di hist
*/
typedef struct userdata{
    int fd;
    //qualcosa per memorizzare i file ricevuti
    history_t *hist;
    int index_to_write;
    int max_hist_msgs;
}user_data_t;




/**
    @function user_data_init
    @brief inizializza la struttura che contiene i dati di un utente

    @param usrdt        struttura che contiene i dati
    @param fd            file descriptor del client
    @param MaxMsgSize       dim massima di un messaggio testuale in ricezione
    @param MaxFileSize      dim massima di un file in ricezione
    @param MaxHistMsgs      massimo numero di messaggi da ricordare

    @return -1 in caso di errore, 0 altrimenti
*/
int user_data_init(user_data_t *usrdt, int fd, int MaxMsgSize, int MaxFileSize, int MaxHistMsgs);



/**
    @function add_to_history
    @brief aggiunge un messaggio alla history dicendo se e' stato letto dal client

    @param usrdt        struttura che contiene i dati
    @param msg_to_add       messaggio da aggiungere allo storico
    @param read     1 se il messaggio da aggiungere e' stato letto dal client, 0 altrimenti

    @return -1 in caso di errore, 0 altrimenti
*/
int add_to_history(user_data_t *usrdt, message_t *msg_to_add, int read);

#endif //USER_H
