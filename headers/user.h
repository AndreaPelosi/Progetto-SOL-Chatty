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
#include <pthread.h>
#include <icl_hash.h>

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

    @param username  username dell'utente
    @param fd       file descriptor associato all'utente
    @param mtex     lock per l'accesso in mutua esclusione all'utente
    @param size     numero di elementi nella history
    @param hist      history dei messaggi ricevuti
    @param index_to_write       indice dell'elemento di hist in cui aggiungere il prossimo messaggio
    @param max_hist_msgs        dimensione di hist
*/
typedef struct userdata{
    char *username;
    int fd;
    //qualcosa per memorizzare i file ricevuti
    pthread_mutex_t mtex;
    history_t *hist;
    int size;
    int index_to_write;
    int max_hist_msgs;
}user_data_t;




/**
    @function user_data_init
    @brief inizializza la struttura che contiene i dati di un utente

    @param usrdt        struttura che contiene i dati
    @param username     nome dell'utente
    @param fd            file descriptor del client con cui si e' collegato l'utente
    @param MaxMsgSize       dim massima di un messaggio testuale in ricezione
    @param MaxFileSize      dim massima di un file in ricezione
    @param MaxHistMsgs      massimo numero di messaggi da ricordare

    @return -1 in caso di errore, 0 altrimenti
*/
int user_data_init(user_data_t *usrdt, char *username, int fd, int MaxMsgSize, int MaxFileSize, int MaxHistMsgs);



/**
    @function add_to_history
    @brief aggiunge un messaggio alla history dicendo se e' stato letto dall'utente

    @param hashtable        struttura dati che contiene le informazioni relative agli utenti
    @param user             nome dell'utente a cui aggiungere il messaggio
    @param msg_to_add       messaggio da aggiungere allo storico
    @param read             1 se il messaggio da aggiungere e' stato letto dall'utente, 0 altrimenti
    @param already_locked   1 se il thread esecutore possiede gia' la lock su usrdt, 0 altrimenti

    @return -1 in caso di errore, 0 altrimenti
*/
int add_to_history(user_data_t *usrdt, message_t *msg_to_add, int read, int already_locked);



/**
    @function add_to_history_all
    @brief aggiunge un messaggio alla history di ogni utente dicendo se e' stato letto

    @param hashtable        struttura dati che contiene le informazioni relative agli utenti
    @param user             nome del mittente di msg_to_add
    @param msg_to_add       messaggio da aggiungere allo storico
    @param nonline          aumenta di uno per ogni messaggio consegnato con successo
    @param noffline         aumenta di uno per ogni messaggio aggiunto alla history di un utente ma non ancora consegnato

    @return 1 in caso di errore, 0 altrimenti
*/
int add_to_history_all(icl_hash_t *hashtable, char *user, message_t *msg_to_add, int *nonline, int *noffline);



/**
    @function retrieve_from_history
    @brief restituisce l'iesimo messaggio nella history dell'utente, setta read_bit relativo al messaggio a 1

    @param hashtable    struttura dati che contiene le informazioni relative agli utenti
    @param user         nome dell'utente da cui recuperare il messaggio
    @param index        indice relativo al messaggio da recuperare
    @param read_now     incrementa di 1 *read_now se il messaggio in questione e' consegnato per la prima volta
    @return history dell'utente associato a usrdt
*/
message_t * retrieve_from_history(user_data_t *usrdt, int index, int *read_now);


/**
    @function update_user_fd
    @brief aggiorna il fd sul quale avviene la comunicazione tra server e l'utente con dati usrdt

    @param usrdt        dati relativi all'utente
    @param new_fd       nuovo file descriptor col quale aggiornare quello in usrdt
*/
void update_user_fd(user_data_t *usrdt, int new_fd);


#endif //USER_H
