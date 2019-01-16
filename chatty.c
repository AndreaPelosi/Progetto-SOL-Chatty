/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file chatty.c
 * @brief File principale del server chatterbox

    @author
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

/* inserire gli altri include che servono */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>

#if 0
#include "./headers/macrothread.h"
#include "./headers/macrosctest.h"
#include "message.h"
#include "ops.h"
#include "config.h"
#include "conf_parsing.h"
#include "connections.h"
#include "stats.h"
#endif

#include <sys/stat.h>

#include <macrothread.h>
#include <macrosctest.h>
#include <message.h>
#include <ops.h>
#include <config.h>
#include <conf_parsing.h>
#include <connections.h>
#include <stats.h>
#include <queue.h>
#include <icl_hash.h>
#include <user.h>
#include <liste.h>

#ifndef HASHTABLE_LENGTH
#define HASHTABLE_LENGTH 1024
#endif




/* struttura che memorizza le statistiche del server, struct statistics
 * e' definita in stats.h.
 */
struct statistics  chattyStats = { 0,0,0,0,0,0,0 };

/* struttura contenente i valori di configurazione, struct conf_values
   definita in conf_parsing.h
*/
struct conf_values val = { NULL, 0, 0, 0, 0, 0, NULL, NULL };

 //coda condivisa: il listener mette fd e i thread del pool li estraggono
queue_t *forwardQ = NULL;


int sig_halt_received = 0;

fd_set set; //insieme dei fd attivi

icl_hash_t *hashtable = NULL; //tabella hash per la registrazione dei client alla chat

list_t *user_list = NULL; //lista degli utenti connessi

static pthread_mutex_t mtex_stats = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtex_set = PTHREAD_MUTEX_INITIALIZER;



/**
    @function update_stats
    @brief incrementa e decrementa i valori delle statistiche

    @param chattyStats       puntatore a struct statistics
    @param nu,no,nd,nnd,nfd,nfnd,ne       incremento/decremento dei valori memorizzati nella struttura
*/
int update_stats(struct statistics *chattyStats, int nu, int no, int nd, int nnd, int nfd, int nfnd, int ne);



/**
    @function run_listener
    @brief corpo del thread listener

    @param arg      indirizzo assegnato al socket
*/
static void *run_listener(void * arg);


/**
    @function run_pool_element
    @brief corpo dei thread appartenenti al pool che elabora le richieste

    @param arg
*/
static void *run_pool_element(void *arg);


/**
    @function elab_request
    @brief elabora il messaggio di un client

    @param fd       file descriptor del client di cui si elabora il messaggio
    @param message      messaggio da elaborare

    @return 0 se l'operazione ha successo, -1 altrimenti
*/
int elab_request(int fd, message_t message);


/**
    @function sig_manager
    @brief inizializza il gestore dei segnali (anche per sigusr1)

    @param

    @return 0 se l'operazione ha successo, -1 altrimenti
*/
int sig_manager();


/**
    @function usage
    @brief stampa su stderr la modalita d'utilizzo corretta del programma

    @param progname     nome del programma
*/
static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}


int main(int argc, char *argv[]) {
    int conf_verifier = 0;
    struct sockaddr_un sa;
//    struct sigaction siga;
    pthread_t tid_listener;
    pthread_t *thread_pool;

    if (3 != argc){
        usage("chatty");
        exit(EXIT_FAILURE);
    }

    if (0 != strcmp(argv[1], "-f")) {
        usage("chatty");
        exit(EXIT_FAILURE);
    }

    if ( (0 == strcmp(argv[2],"DATA/chatty.conf1")) ||(0 == strcmp(argv[2],"DATA/chatty.conf2")) ){ //verificare se va bene cosi o se si puo perfezionare
        conf_verifier = conf_init(argv[2], &val);
    } else{
        usage("chatty");
        exit(EXIT_FAILURE);
    }

    if (0 != conf_verifier){
        printf("fallimento conf_verifier!\n");
        return conf_verifier;
    }

    strncpy(sa.sun_path, val.UnixPath, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;




    //creazione tabella hash
    hashtable = icl_hash_create(HASHTABLE_LENGTH, NULL, NULL, val.MaxConnections);

    //creazione lista degli utenti connessi
    user_list = createList();

#if 0
    //creazione cartella dove memorizzare i file
    if (0 != mkdir(val.DirName, 0777)){
        perror("creazione cartella per memorizzare i file");
        return -1;
    }
#endif

    thread_pool = (pthread_t *)malloc(val.ThreadsInPool * sizeof(pthread_t)); //creazione del pool di thread che gestiranno le richieste dei client
    forwardQ = createQ(val.MaxConnections); //creo una coda per i fd; il listener pusha fd, i thread del pool poppano

    //printf("ciao\n");

    for (size_t i = 0; i < val.ThreadsInPool; i++) {
        THREAD(pthread_create(&thread_pool[i], NULL, &run_pool_element, NULL), "creando thread_pool");
    }

    //inserire prima la creazione del pool di thread, dopo il main si sospende attendendo
    //la join di tid_listener (?)
    //i thread del pool sono joinable

    THREAD(pthread_create(&tid_listener, NULL, &run_listener, (void*)&sa), "creando tid_listener");

    THREAD(pthread_join(tid_listener, NULL), "joinando tid_listener");

    for (size_t c = 0; c < val.ThreadsInPool; c++) {
        THREAD(pthread_join(thread_pool[c], NULL), "joinando thread_pool");
    }
    printf("successo!\n");


    //fare tutta la pulizia necessaria: chiudere i file descriptor, chiudere fd_skt, distruggere coda, distruggere tutto, fare tutte le free
    //distruggere tabella hash
    return 0;
}



int sig_manager() {

    sigset_t set;
    struct sigaction siga;

    ec_meno1(sigfillset(&set), "sigfillset error"); //includo nel set tutti i segnali
    ec_meno1(pthread_sigmask(SIG_SETMASK, &set, NULL), "sigmask error"); //maschero tutti i segnali in attesa di installare il gestore

    memset(&siga, 0, sizeof(siga));

    //ricordarsi di bloccare i segnali esplicitamente durante la gestione di ogni singolo segnale

    return 0;
}


int update_stats(struct statistics *chattyStats, int nu, int no, int nd, int nnd, int nfd, int nfnd, int ne){

    THREAD(pthread_mutex_lock(&mtex_stats), "lock in update_stats");

    chattyStats->nusers += (nu);
    chattyStats->nonline += (no);
    chattyStats->ndelivered += (nd);
    chattyStats->nnotdelivered += (nnd);
    chattyStats->nfiledelivered += (nfd);
    chattyStats->nfilenotdelivered += (nfnd);
    chattyStats->nerrors += (ne);

    if(chattyStats->nusers < 0 || chattyStats->nonline < 0 || chattyStats->ndelivered < 0 ||
        chattyStats->nnotdelivered < 0 || chattyStats->nfiledelivered < 0 || chattyStats->nfilenotdelivered < 0 ||
        chattyStats->nerrors < 0){
            printf("error: illegal stat value\n");

            THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in update_stats");

            return -1;
    }

    THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in update_stats");

    return 0;
}




static void *run_listener(void * arg){

    int fd_skt, //socket di connessione
        fd_c, //socket di I/O con un client
        fd, //indice per verificare i risultati della select
        fd_num = 0; //massimo fd attivo
    //char *buf = (char *)malloc(100*sizeof(char)); //buffer di prova
    //int nread; //numero di caratteri letti
        fd_set rdset; //insieme dei fd attesi per la lettura
//        wrset; //insieme dei fd su cui scrivere

    //apro la connessione su fd_skt
    fd_skt = socket(AF_UNIX, SOCK_STREAM, 0);

    if (-1 == fd_skt){
        perror("socket thread_listener server");
        return(void *)-1; //controllare se va bene
    }
    if (-1 == bind(fd_skt, (struct sockaddr *)arg, sizeof(struct sockaddr_un)) ){
        perror("bind socket per le connessioni coi client");
        return (void *)-1;
    }
    if (-1 == listen(fd_skt, SOMAXCONN) ){
        perror("listen socket per le connessioni coi client");
        return (void *)-1;
    }

    struct timeval timeout = {0, 10000};

//    printf("fd_num prima : %d\n", fd_num);
    if (fd_skt > fd_num) fd_num = fd_skt;
    FD_ZERO(&set);
    FD_SET(fd_skt,&set);
//    printf("fd_num dopo : %d\n", fd_num);
//    printf("fd_skt : %d\n", fd_skt);
    while (!sig_halt_received) {
        THREAD(pthread_mutex_lock(&mtex_set), "lock in run_listener");
        rdset = set;
        THREAD(pthread_mutex_unlock(&mtex_set), "unlock in run_listener");
//        printf("sono arrivato qui1\n" );

        int sel = select((fd_num + 1), &rdset, NULL, NULL, &timeout);
        if (-1 ==  sel){
            perror("errore select thread listener");
            return (void *)-1;
        } else if (0 != sel){
//            printf("sono arrivato qui2\n" );

            for (fd = 0; fd <= fd_num; fd++) {
                printf("controllo il seguente fd: %d\n", fd);
                if (FD_ISSET(fd, &rdset)) {

                    if (fd == fd_skt) { //nuova connessione con un client
                        THREAD(pthread_mutex_lock(&mtex_stats), "lock in run_listener");
                        if (chattyStats.nonline >= val.MaxConnections){

                            THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in run_listener");

                            THREAD(pthread_mutex_lock(&mtex_set), "lock in run_listener");
                            FD_CLR(fd,&set);
                            THREAD(pthread_mutex_unlock(&mtex_set), "unlock in run_listener");

                            printf("connessione del fd %d rifiutata: limite massimo di connessioni raggiunto\n", fd);
                            update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //si aggiunge 1 al numero degli errori
                        } else{

                            THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in run_listener");

                            fd_c = accept(fd_skt, NULL, 0);

                            THREAD(pthread_mutex_lock(&mtex_set), "lock in run_listener");
                            FD_SET(fd_c, &set);
                            THREAD(pthread_mutex_unlock(&mtex_set), "unlock in run_listener");
                            //il client non e' ancora online
                            printf("nuova connessione con un client, il fd e': %d\n", fd_c);
                            if (fd_c > fd_num) fd_num = fd_c;
                        }
                    } else { //passo il fd ad un thread per elaborare le richieste

                        THREAD(pthread_mutex_lock(&mtex_set), "lock in run_listener");
                        FD_CLR(fd,&set);
                        THREAD(pthread_mutex_unlock(&mtex_set), "unlock in run_listener");

                        //pusho il fd nella coda
                        printf("sto facendo la push del file descriptor %d\n", fd);
                        pushQ(forwardQ, fd); //aggiungere ec_meno1?
//                        printf("al thread listener e' arrivato il seguente fd : %d\n", fd);


                        #if 0
                        nread = read(fd, buf, 100);
        //                ec_meno1(nread, "server read error");
                        if (0 == nread){ //lettura finita
                            FD_CLR(fd, &set); //controllare se basta questo
                            close(fd);
                        } else {
                            printf("server got: %s\n", buf);
        //                    ec_meno1(write(fd, "lettura avvenuta con successo", 30), "server write error");
                        }
                        #endif


                    }
                }
            }
        } else if (0 == sel){
            printf("timeout expired\n");
            timeout.tv_sec = 0;
            timeout.tv_usec = 10000;
        }
    }
    pthread_exit(NULL);
}



static void* run_pool_element(void *arg){

    message_t msg;

    while (!sig_halt_received) {
        int fd = popQ(forwardQ);

        printf("un thread e' arrivato qui il thread %lu\n", pthread_self());

        int r = readHeader(fd, &msg.hdr);

        if (r < 0) {

            printf("errore lettura dell'header\n");
            perror("leggendo header:");
            free(msg.data.buf);

        } else if (r > 0){

            int ans = elab_request(fd, msg);
            free(msg.data.buf);

            if (ans < 0){
                printf("errore nell'elaborazione del messaggio, %d non verra' aggiunto nuovamente alla coda\n", fd);
                //???int control = update_stats(&chattyStats, 0, 1, 0, 0, 0, 0, 0);
                //if (-1 == control) //invia segnale che arresta il programma
            } else if (0 == ans){
                printf("elaborazione del messaggio avvenuta con successo, %d verra' aggiunto nuovamente alla coda\n", fd);

                THREAD(pthread_mutex_lock(&mtex_set), "lock in run_pool_element");
                FD_SET(fd, &set);
                THREAD(pthread_mutex_unlock(&mtex_set), "unlock in run_pool_element");

            } else if (ans > 0) {
                printf("il destinatario di %d ha la history piena, elaborazione del messaggio fallita ma %d verra' aggiunto nuovamente alla coda\n", fd, fd);

                THREAD(pthread_mutex_lock(&mtex_set), "lock in run_pool_element");
                FD_SET(fd, &set);
                THREAD(pthread_mutex_unlock(&mtex_set), "unlock in run_pool_element");
            }

        } else { //se r == 0

            //l'utente viene disconnesso (e quindi eliminato dalla lista dei connessi)
            user_list = deleteFdFromList(user_list, fd);

            int check_stats = update_stats(&chattyStats, 0, -1, 0, 0, 0, 0, 0);//diminuisco il numero degli utenti connessi
            if (-1 == check_stats){
                //segnale che arresta il programma
            }

            printf("lettura su %d terminata, il fd non verra' aggiunto nuovamente alla coda\n", fd);
        }
    }

    pthread_exit(NULL);
}



int elab_request(int fd_c, message_t message){

    message_t reply_msg;

    char *sender = strdup(message.hdr.sender);

    //se la lettura dei dati del messaggio fallisce
    if(readData(fd_c, &message.data) <= 0){
        printf("errore: fallimento lettura dati\n");
        free(message.data.buf);

        update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

        return -1;
    }


    //elaboro le richieste
    switch (message.hdr.op) {



//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//



        case REGISTER_OP: {

            printf("richiesta di registrazione\n" );

            user_data_t *usrdt = (user_data_t *)malloc(sizeof(user_data_t));

            //errore nell'inizializzazione della struttura che contiene i dati dell'utente
            if (-1 == user_data_init(usrdt, fd_c, val.MaxMsgSize, val.MaxFileSize, val.MaxHistMsgs)){
                return -1;
                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
            }

            //e' gia' stato raggiunto MaxConnections: la registrazione fallisce
            THREAD(pthread_mutex_lock(&mtex_stats), "lock in elab_request");

            if (chattyStats.nonline >= val.MaxConnections){

                printf("numero massimo di connessioni contemporanee raggiunto, registrazione fallita\n");
                THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in elab_request");

                setHeader(&reply_msg.hdr, OP_FAIL, ""); //come sender metto "server"?
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }

            THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in elab_request");

            //esiste gia' un utente con quel nickname
            if (NULL != icl_hash_find(hashtable, sender)){

                printf("nickname %s gia' iscritto a chatty\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_ALREADY, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }

            printf("sto inserendo il nick: %s\n", sender);
            //icl_hash_insert fallisce
            if(NULL == icl_hash_insert(hashtable, (void *)sender, (void *) usrdt)){
                printf("errore nella registrazione di %s\n", sender);


                setHeader(&reply_msg.hdr, OP_FAIL, ""); //come sender metto "server"?
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }

            //icl_hash_insert ha avuto successo

            update_stats(&chattyStats, 1, 1, 0, 0, 0, 0, 0); //aumento il numero degli utenti registrati e dei connessi

            user_list = insertListHead(user_list, sender, fd_c); //aggiunge il nick e il fd alla lista degli utenti connessi

            char * buffer = toBuf(user_list); //memorizza nel buffer da inviare la lista degli utenti connessi
            int length = strlen(buffer); //lunghezza del buffer

            //setto Header e Dati per la risposta del server al client
            setHeader(&reply_msg.hdr, OP_OK, "");

            if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                return -1;
            }

            setData(&reply_msg.data, "", buffer, length);

            if (sendData(fd_c, &reply_msg.data) < 0) {
                return -1;
            }

        } break;



//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//



        case CONNECT_OP: {

            printf("richiesta di connessione\n" );

            //il client che tenta di connettersi non e' registrato
            if (NULL == icl_hash_find(hashtable, sender)){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, ""); //come sender metto "server"?
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }


            //e' gia' stato raggiunto MaxConnections: il nuovo tentativo di connessione fallisce
            THREAD(pthread_mutex_lock(&mtex_stats), "lock in elab_request");

            if (chattyStats.nonline >= val.MaxConnections){

                printf("numero massimo di connessioni contemporanee raggiunto, connessione fallita\n");
                THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in elab_request");

                setHeader(&reply_msg.hdr, OP_FAIL, ""); //come sender metto "server"?
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }

            THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in elab_request");


            //il client riesce a connettersi
            update_stats(&chattyStats, 0, 1, 0, 0, 0, 0, 0); //aumento il numero degli utenti connessi

            //non inserisco tra gli utenti connessi se c'e' gia'
            if (NULL == listFind(user_list, sender)){

                user_list = insertListHead(user_list, sender, fd_c); //aggiunge il nick e il fd alla lista degli utenti connessi
            }

            char * buffer = toBuf(user_list); //memorizza nel buffer da inviare la lista degli utenti connessi
            int length = strlen(buffer); //lunghezza del buffer

            //setto Header e Dati per la risposta del server al client
            setHeader(&reply_msg.hdr, OP_OK, "");

            if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                return -1;
            }

            setData(&reply_msg.data, "", buffer, length);

            if (sendData(fd_c, &reply_msg.data) < 0) {
                return -1;
            }

        } break;



//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//



        case POSTTXT_OP: {

            printf("richiesta di invio a un utente di un messaggio\n" );
            //il client che tenta di inviare un messaggio non e' registrato
            if (NULL == icl_hash_find(hashtable, sender)){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, ""); //come sender metto "server"?
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }


            //il client che tenta di inviare un messaggio non e' connesso
            if (NULL == listFind(user_list, sender)){

                printf("il client %s non e' connesso\n", sender);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }


            char *receiver = message.data.hdr.receiver; //nickname del destinatario
            int buf_len = message.data.hdr.len; //lunghezza del messaggio


            //se il nickname del destinatario non e' registrato
            if (NULL == icl_hash_find(hashtable, receiver)) {

                printf("il nickname destinatario scelto non e' iscritto a Chatty\n");

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }


            //se il messaggio e' troppo lungo
            if (buf_len > val.MaxMsgSize) {

                printf("messaggio troppo lungo\n");

                setHeader(&reply_msg.hdr, OP_MSG_TOOLONG, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }

            //operazione ha avuto successo

            user_data_t *dest = icl_hash_find(hashtable, receiver);

            //setta l'header del messaggio di risposta dal server per il mittente di POSTTXT_OP
            setHeader(&reply_msg.hdr, OP_OK, "");

            message_t msg_to_deliver;

            setHeader(&msg_to_deliver.hdr, TXT_MESSAGE, sender);
            setData(&msg_to_deliver.data, receiver, message.data.buf, message.data.hdr.len);

            int ans = sendRequest(dest->fd, &msg_to_deliver);

            if (-1 == ans) { //se il destinatario e' offline

                printf("il destinatario e' offline, aggiungo il messaggio alla lista dei messaggi che deve leggere\n");

                //aggiorno la history del destinatario e marco il messaggio come non letto
                if (-1 == add_to_history(dest, &msg_to_deliver, 0)){
                    return 1; //history del destinatario piena, il fd del mittente verra' rimesso in coda
                }

                update_stats(&chattyStats, 0, 0, 0, 1, 0, 0, 0); //aumento il numero dei messaggi non letti

            } else { //se il destinatario e' online

                printf("il destinatario e' online, messaggio inviato con successo\n");

                //aggiorno la history del destinatario e marco il messaggio come letto
                if (-1 == add_to_history(dest, &msg_to_deliver, 1)){
                    return 1; //history del destinatario piena, il fd del mittente verra' rimesso in coda
                }

                update_stats(&chattyStats, 0, 0, 1, 0, 0, 0, 0);//aumento il numero dei messaggi letti
            }


            if (sendHeader(fd_c, &reply_msg.hdr) <= 0)
                return -1;

        } break;



//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//



        case POSTTXTALL_OP: {
#if 0
            printf("richiesta di invio a tutti di un messaggio\n");

            //il client che tenta di inviare un messaggio non e' registrato
            if (NULL == icl_hash_find(hashtable, sender)){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, ""); //come sender metto "server"?
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }


            //il client che tenta di inviare un messaggio non e' connesso
            if (NULL == listFind(user_list, sender)){

                printf("il client %s non e' connesso\n", sender);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }

            int buf_len = message.data.hdr.len;

            //se il messaggio e' troppo lungo
            if (buf_len > val.MaxMsgSize) {

                printf("messaggio troppo lungo\n");

                setHeader(&reply_msg.hdr, OP_MSG_TOOLONG, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }


            //operazione ha avuto successo
            int i;
            icl_entry_t *j;
            char *kp;
            user_data_t *dp;

            //setta l'header del messaggio di risposta dal server per il mittente di POSTTXTALL_OP
            setHeader(&reply_msg.hdr, OP_OK, "");

            icl_hash_foreach(hashtable, i, j, kp, dp){ //per ogni elemento della tabella hash

                message_t msg_to_deliver; //messaggio da inviare al destinatario

                setHeader(&msg_to_deliver.hdr, TXT_MESSAGE, sender);
                setData(&msg_to_deliver.data, curr->word, message.data.buf, message.data.hdr.len);

                int ans = sendRequest(curr->fd, &msg_to_deliver);

                if (-1 == ans) { //il destinatario e' offline

                    printf("il destinatario e' offline, aggiungo il messaggio alla lista dei messaggi che deve leggere\n");

                    //aggiorno la history del destinatario e marco il messaggio come non letto
                    if (-1 == add_to_history(dest, &msg_to_deliver, 0)){
                        return 1; //history del destinatario piena, il fd del mittente verra' rimesso in coda
                    }

                    update_stats(&chattyStats, 0, 0, 0, 1, 0, 0, 0);//aumento il numero dei messaggi testuali non consegnati

                } else { //il destinatario e' online
                    printf("il destinatario e' online, messaggio inviato con successo\n");

                    //aggiorno la history del destinatario e marco il messaggio come letto
                    if (-1 == add_to_history(dest, &msg_to_deliver, 1)){
                        return 1; //history del destinatario piena, il fd del mittente verra' rimesso in coda
                    }

                    update_stats(&chattyStats, 0, 0, 1, 0, 0, 0, 0);//aumento il numero dei messaggi testuali consegnati
                }

            }


            if (sendHeader(fd_c, &reply_msg.hdr) <= 0)
                return -1;
#endif
        } break;



//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//



        case POSTFILE_OP: {


            printf("richiesta di post di un file\n");

            //il client che tenta di inviare il file non e' registrato
            if (NULL == icl_hash_find(hashtable, sender)){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, ""); //come sender metto "server"?
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }


            //il client che tenta di inviare il file non e' connesso
            if (NULL == listFind(user_list, sender)){

                printf("il client %s non e' connesso\n", sender);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }


            message_data_t fileData; //ci memorizzo i dati del file
            readData(fd_c, &fileData);

            //printf("ecco il contenuto del file :%s\n", fileData.buf);

            printf("questa e' la size del file che si vuole inviare: %d\n", fileData.hdr.len);

            //se il file e' troppo grande per essere inviato
            if (fileData.hdr.len > val.MaxFileSize) {


                printf("errore: file troppo grande per essere inviato\n");

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1);

                setHeader(&reply_msg.hdr, OP_MSG_TOOLONG, "");
                if(sendHeader(fd_c, &reply_msg.hdr) <= 0){
                    return -1;
                }

                return -1;
            }



            //il file puo' essere inviato
            int pathlen = strlen(val.DirName) + strlen(message.data.buf) + 2; // byte aggiuntivo per lo slash l'altro per il terminatore

            //costruisco il path del file
            char *path = (char *)malloc(pathlen * sizeof(char));

            strncat(path, val.DirName, strlen(val.DirName));
            strcat(path, "/");

            char *slash  = strrchr(message.data.buf, '/'); //controllo se esiste uno slash in message.data.buf

            if (NULL == slash) {
                strncat(path, message.data.buf, message.data.hdr.len);
            } else {
                slash ++;
                int len = message.data.hdr.len - (slash - message.data.buf);
                strncat(path, slash, len);
            }

            printf("questo e' il percorso dove viene salvato il file: %s\n", path);


            //apro il file
            FILE *file_to_deliver = fopen(path, "w");

            if (NULL == file_to_deliver){ //se fallisce l'apertura
                printf("errore nell'open del file in %s\n", path);
                return -1;
            }


            int x = fwrite(fileData.buf, sizeof(char), fileData.hdr.len, file_to_deliver);

            printf("%d\n", x);
            if (x != fileData.hdr.len) { //se c'e' un errore nella scrittura
                printf("errore nella scrittura\n");
                return -1;
            }

            if (fclose(file_to_deliver) != 0) {
                printf("errore nella chiusura del file da inviare\n");
                return -1;
            }

            char *receiver = message.data.hdr.receiver; //nickname del destinatario
            //int buf_len = message.data.hdr.len;


            user_data_t *dest = icl_hash_find(hashtable, receiver);
            //se il nickname del destinatario non e' registrato a Chatty
            if (NULL == dest) {

                printf("il nickname destinatario scelto non e' iscritto a Chatty\n");

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }


            //l'operazione ha successo


            //setto l'header del messaggio di risposta dal server per il mittente di POSTFILE_OP
            setHeader(&reply_msg.hdr, OP_OK, "");


            message_t msg_to_deliver;

            //setto header con operazione FILE_MESSAGE
            setHeader(&msg_to_deliver.hdr, FILE_MESSAGE, sender);

            //e dati dove scrivo nel buffer il nome del file
            setData(&msg_to_deliver.data, receiver, message.data.buf, message.data.hdr.len);

            int ans = sendRequest(dest->fd, &msg_to_deliver);


            if (-1 == ans) { //se il destinatario e' offline

                printf("il destinatario e' offline, aggiungo il messaggio file alla lista dei messaggi che deve leggere\n");

                //aggiorno la history del destinatario e marco il messaggio come non letto
                if (-1 == add_to_history(dest, &msg_to_deliver, 0)){
                    return 1; //history del destinatario piena, il fd del mittente verra' rimesso in coda
                }


                update_stats(&chattyStats, 0, 0, 0, 1, 0, 1, 0); //aumento il numero dei messaggi non letti e dei file non consegnati


            } else { //se il destinatario e' online

                printf("il destinatario e' online, messaggio file inviato con successo\n");

                //aggiorno la history del destinatario e marco il messaggio come letto
                if (-1 == add_to_history(dest, &msg_to_deliver, 1)){
                    return 1; //history del destinatario piena, il fd del mittente verra' rimesso in coda
                }

                update_stats(&chattyStats, 0, 0, 0, 0, 1, 0, 0);
            }


            if(sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                return -1;
            }

        } break;



//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//



        case GETFILE_OP: {

        } break;



//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//



        case GETPREVMSGS_OP: {

        } break;



//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//



        case USRLIST_OP: {

            //il client che tenta di ottenere la lista degli utenti non e' registrato
            if (NULL == icl_hash_find(hashtable, sender)){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, ""); //come sender metto "server"?
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }


            //il client che tenta di ottenere la lista degli utenti non e' connesso
            if (NULL == listFind(user_list, sender)){

                printf("il client %s non e' connesso\n", sender);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }


            //operazione ha avuto successo
            char * buffer = toBuf(user_list); //memorizza nel buffer da inviare la lista degli utenti connessi
            int length = strlen(buffer); //lunghezza del buffer

            //setto Header e Dati per la risposta del server al client
            setHeader(&reply_msg.hdr, OP_OK, "");

            if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                return -1;
            }

            setData(&reply_msg.data, "", buffer, length);

            if (sendData(fd_c, &reply_msg.data) < 0) {
                return -1;
            }



        } break;



//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//



        case UNREGISTER_OP: {

            //il client che tenta di deregistrarsi non e' registrato
            if (NULL == icl_hash_find(hashtable, sender)){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, ""); //come sender metto "server"?
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }


            //il client che tenta di deregistrarsi non e' connesso
            if (NULL == listFind(user_list, sender)){

                printf("il client %s non e' connesso\n", sender);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }

            //l'operazione ha successo

            //l'utente viene disconnesso (e quindi eliminato dalla lista dei connessi)
            user_list = deleteNameFromList(user_list, sender);

            int check_stats = update_stats(&chattyStats, 0, -1, 0, 0, 0, 0, 0);//diminuisco il numero degli utenti connessi
            if (-1 == check_stats){
                //segnale che arresta il programma
            }

            //l'utente viene deregistrato
            if (0 != icl_hash_delete(hashtable, sender, NULL, NULL)){

                printf("utente %s deregistrazione fallita\n", sender);
                return -1;
            }

            check_stats = update_stats(&chattyStats, -1, 0, 0, 0, 0, 0, 0);//diminuisco il numero degli utenti registrati
            if (-1 == check_stats){
                //segnale che arresta il programma
            }


            setHeader(&reply_msg.hdr, OP_OK, "");
            if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                return -1;
            }


        } break;



//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//



        case DISCONNECT_OP: {


            //il client che tenta di disconnettersi non e' registrato
            if (NULL == icl_hash_find(hashtable, sender)){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, ""); //come sender metto "server"?
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }


            //il client che tenta di disconnettersi non e' connesso
            if (NULL == listFind(user_list, sender)){

                printf("il client %s non e' connesso\n", sender);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                return -1;
            }


            //l'operazione ha successo

            //l'utente viene disconnesso (e quindi eliminato dalla lista dei connessi)
            user_list = deleteNameFromList(user_list, sender);

            int check_stats = update_stats(&chattyStats, 0, -1, 0, 0, 0, 0, 0);//diminuisco il numero degli utenti connessi
            if (-1 == check_stats){
                //segnale che arresta il programma
            }


            setHeader(&reply_msg.hdr, OP_OK, "");
            if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                return -1;
            }

        } break;



//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//



        default: {

            printf("operazione non riconosciuta\n"); //printf? in questo modo lo stampa lato server
            setHeader(&reply_msg.hdr, OP_FAIL, ""); //come sender metto "server"?
            sendHeader(fd_c, &reply_msg.hdr);
            free(message.data.buf);

            return -1;
        } break;



    }

    return 0;
}
