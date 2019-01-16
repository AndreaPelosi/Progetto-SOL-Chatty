/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file conf_parsing.h
 * @brief dichiarazione strutture e funzioni necessarie per il parsing del file
            di configurazione del server
 */

#ifndef CONF_PARSING_H
#define CONF_PARSING_H

#define MAX_CONF_FILE_LINE_SIZE 300 //lunghezza massima accettata di una riga del file di configurazione


/**
    @struct conf_values
    @brief struttura che memorizza i valori di configurazione che il server deve rispettare

    @var UnixPath       path utilizzato per la creazione del socket AF_UNIX
    @var MaxConnections     numero massimo di connessioni concorrenti gestite dal server
    @var ThreadsInPool      numero di thread nel pool
    @var MaxMsgSize     dimensione massima di un messaggio testuale (numero di caratteri)
    @var MaxFileSize        dimensione massima di un file accettato dal server (kilobytes)
    @var MaxHistMsgs        numero massimo di messaggi che il server 'ricorda' per ogni client
    @var DirName        directory dove memorizzare i files da inviare agli utenti
    @var StatFileName       file nel quale verranno scritte le statistiche
*/
struct conf_values{
        char *UnixPath;
        int MaxConnections;
        int ThreadsInPool;
        int MaxMsgSize;
        int MaxFileSize;
        int MaxHistMsgs;
        char *DirName;
        char *StatFileName;
};


/**
    @function conf_init
    @brief parsa uno dei file di configurazione e setta il server con i valori del file

    @param conffile     percorso del file di configurazione
    @param val        struttura dove vengono memorizzati i valori di configurazione

    @return 0 se ha successo, EXIT_FAILURE atrimenti
*/
int conf_init( char *conffile, struct conf_values *val);

#endif
