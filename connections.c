/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
 /**
  * @file  connections.c
  * @brief Contiene le funzioni che implementano il protocollo
  *        tra i clients ed il server
  *
  * @author Andrea Pelosi 547596
  * Si dichiara che il contenuto di questo file è in ogni sua parte opera
  * originale dell'autore
  *
  */
#ifndef CONNECTIONS_C_
#define CONNECTIONS_C_


#include <message.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <macrosctest.h>
#include <connections.h>
#include <errno.h>
#include <stdlib.h>

#include <stdio.h>


int openConnection(char* path, unsigned int ntimes, unsigned int secs){
    int fd_c;
    int retry = 0; //numero di retry della connessione effettuati

    ntimes = (ntimes > MAX_RETRIES) ? MAX_RETRIES : ntimes;
    secs = (secs > MAX_SLEEPING) ? MAX_SLEEPING : secs;

    struct sockaddr_un sa;

    strncpy(sa.sun_path, path, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;

    fd_c = socket(AF_UNIX, SOCK_STREAM, 0);


    while (-1 == connect(fd_c, (struct sockaddr *)&sa, sizeof(sa))) {
        if (retry++ < ntimes) {
            printf("questo e' un tentativo\n" );
            fflush(stdout);
            sleep(secs);
        } else {
            return -1;
        }
    }

    return fd_c;
}

// -------- server side -----

int readHeader(long connfd, message_hdr_t *hdr){

    return recv(connfd, hdr, sizeof(message_hdr_t), 0);//errno e' settato
}



int readDataHeader(long fd, message_data_hdr_t *data_hdr){

    return recv(fd, data_hdr, sizeof(message_data_hdr_t), 0);
}



int readData(long fd, message_data_t *data){

    int t = readDataHeader(fd, &data->hdr);

    if (t < 0 )return t;

    if (0 != data->hdr.len){

        data->buf = (char *)malloc(data->hdr.len * sizeof(char));
        if (NULL == data->buf) {
            return -1;
        }

        int len = data->hdr.len;
        char *curr = data->buf;

        //leggo i bytes dal file descriptor e li inserisco in data->buf
        while (len > 0){
            t = recv(fd, curr, len, 0);

            if (t < 0)    return t;

            curr += t;
            len -= t;
        }
    } else {
        data->buf = NULL;
    }

    return 1;
}



int readMsg(long fd, message_t *msg){

    if (readHeader(fd, &msg->hdr) <= 0){
        return -1;
    }
    if (readData(fd, &msg->data) <= 0){
        return -1;
    }

    return 1;
}



// ------- client side ------


int sendHeader(long fd, message_hdr_t *hdr){

    return send(fd, hdr, sizeof(message_hdr_t), MSG_NOSIGNAL);
}



int sendDataHeader(long fd, message_data_hdr_t *data_hdr){

    return send(fd, data_hdr, sizeof(message_data_hdr_t), MSG_NOSIGNAL);
}



int sendData(long fd, message_data_t *data){

    int t = sendDataHeader(fd, &data->hdr);
    if (t <= 0) return t;

    int len = data->hdr.len;
    char *curr = data->buf;


    while (len > 0) {
        t = send(fd, curr, len, MSG_NOSIGNAL);
        if (t <= 0) return t;

        curr += t;
        len -= t;
    }

    return 1;

}



int sendRequest(long fd, message_t *msg){

    if (sendHeader(fd, &msg->hdr) <= 0) {
        return -1;
    }

    if (sendData(fd, &msg->data) <= 0) {
        return -1;
    }

    return 1;
}



#endif /* CONNECTIONS_C_ */
