/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
 /**
  * @file  connections.h
  * @brief Contiene le funzioni che implementano il protocollo
  *        tra i clients ed il server
  *
  * @author Andrea Pelosi 547596
  * Si dichiara che il contenuto di questo file è in ogni sua parte opera
  * originale dell'autore
  *
  */
#ifndef CONNECTIONS_H_
#define CONNECTIONS_H_

#define MAX_RETRIES     10
#define MAX_SLEEPING     3
#if !defined(UNIX_PATH_MAX)
#define UNIX_PATH_MAX  64
#endif

#include <message.h>


/**
 * @function openConnection
 * @brief Apre una connessione AF_UNIX verso il server
 *
 * @param path Path del socket AF_UNIX
 * @param ntimes numero massimo di tentativi di retry
 * @param secs tempo di attesa tra due retry consecutive
 *
 * @return il descrittore associato alla connessione in caso di successo
 *         -1 in caso di errore
 */
int openConnection(char* path, unsigned int ntimes, unsigned int secs);

// -------- server side -----
/**
 * @function readHeader
 * @brief Legge l'header del messaggio
 *
 * @param fd     descrittore della connessione
 * @param hdr    puntatore all'header del messaggio da ricevere
 *
 * @return <=0 se c'e' stato un errore
 */
int readHeader(long connfd, message_hdr_t *hdr);
/**
 *    @function readDataHeader
 *    @brief Legge l'header dei dati del messaggio
 *
 *    @param fd       descrittore della connessione
 *    @param data_hdr     puntatore all'header dei dati del messaggio
 *
 *    @return <=0 se c'e' stato un errore
 */
int readDataHeader(long fd, message_data_hdr_t *data_hdr);
/**
 * @function readData
 * @brief Legge il body del messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al body del messaggio
 *
 * @return <=0 se c'e' stato un errore
 */
int readData(long fd, message_data_t *data);

/**
 * @function readMsg
 * @brief Legge l'intero messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al messaggio
 *
 * @return <=0 se c'e' stato un errore
 */
int readMsg(long fd, message_t *msg);

/* da completare da parte dello studente con altri metodi di interfaccia */


// ------- client side ------
/**
 * @function sendRequest
 * @brief Invia un messaggio di richiesta al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendRequest(long fd, message_t *msg);

/**
    @function sendDataHeader
    @brief Invia l'header dei dati del messaggio

    @param fd       descrittore della connessione
    @param data_hdr     puntatore all'header dei dati del messaggio da inviare

    @return <=0 se c'e' stato un errore
*/
int sendDataHeader(long fd, message_data_hdr_t *data_hdr);
/**
 * @function sendData
 * @brief Invia il body del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendData(long fd, message_data_t *msg);

/**
    @function sendHeader
    @brief Invia l'header del messaggio al server

    @param fd       descrittore della connessione
    @param msg      puntatore al messaggio da inviare

    @return <=0 se c'e' stato un errore
 */
int sendHeader(long fd, message_hdr_t *msg);

/* da completare da parte dello studente con eventuali altri metodi di interfaccia */


#endif /* CONNECTIONS_H_ */
