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
 *
 * @author Andrea Pelosi 547596
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
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
#include <fcntl.h>

#include <sys/stat.h>

#include <macrothread.h>
#include <macrosctest.h>
#include <message.h>
#include <ops.h>
#include <config.h>
#include <conf_parsing.h>
#include <connections.h>
#include <stats.h>
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
static struct conf_values val = { NULL, 0, 0, 0, 0, 0, NULL, NULL };

/* pipe di comunicazione tra thread listener e thread_pool; listener scrive un fd
 * dal quale e' pronta la lettura, un thread del pool legge dalla pipe tale fd
*/
static int comm_pipe[2];

//viene settata a 1 alla ricezione di un segnale tra SIGINT, SIGTERM, SIGQUIT
volatile sig_atomic_t sigstop = 0;
//viene settata a 1 alla ricezione di SIGUSR1
volatile sig_atomic_t sigusr = 0;

static fd_set set; //insieme dei fd attivi

static icl_hash_t *hashtable = NULL; //tabella hash per la registrazione dei client alla chat

static list_t *user_list = NULL; //lista degli utenti connessi

static pthread_mutex_t mtex_stats = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtex_set = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtex_pipe = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t cond_pipe = PTHREAD_COND_INITIALIZER;


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
*/
void sig_manager();

/**
    @function termination_handler
    @brief gestore per i segnali SIGQUIT, SIGINT, SIGTERM

    @param sig       segnale ricevuto
*/
void termination_handler(int sig);

/**
    @function stats_handler
    @brief gestore per il segnale SIGUSR1

    @param sig      segnale ricevuto
*/
void stats_handler(int sig);

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
    int conf_verifier = 0; //diventa -1 se c'e' un fallimento nel parsing del file di configurazione
    int dim_array_mtex; //size dell'array di mutex per gestire la concorrenza in hashtable
    struct sockaddr_un sa;

    pthread_t tid_listener;
    pthread_t *thread_pool;

    sig_manager();
    printf("installazione del gestore dei segnali avvenuta con successo!\n");

    if (3 != argc){
        usage("chatty");
        exit(EXIT_FAILURE);
    }

    if (0 != strcmp(argv[1], "-f")) {
        usage("chatty");
        exit(EXIT_FAILURE);
    }

    if ( (0 == strcmp(argv[2],"DATA/chatty.conf1")) ||(0 == strcmp(argv[2],"DATA/chatty.conf2")) ){
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


    /***** CREAZIONE E INIZIALIZZAZIONE STRUTTURE DATI *****/

    //creazione tabella hash
    hashtable = icl_hash_create(HASHTABLE_LENGTH, NULL, NULL, val.MaxConnections, &dim_array_mtex);

    //creazione lista degli utenti connessi
    user_list = createList();

    //creazione del pool di thread che gestiranno le richieste dei client
    thread_pool = (pthread_t *)malloc(val.ThreadsInPool * sizeof(pthread_t));

    if (0 != pipe(comm_pipe)) {
        printf("fallimento nella creazione della pipe di comunicazione tra listener e thread_pool\n");
        exit(EXIT_FAILURE);
    }

    if (fcntl(comm_pipe[0], F_SETFL, O_NONBLOCK) < 0) { //la pipe in lettura sara' non bloccante
        perror("fcntl error");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < val.ThreadsInPool; i++) {
        THREAD(pthread_create(&thread_pool[i], NULL, &run_pool_element, NULL), "creando thread_pool");
    }

    THREAD(pthread_create(&tid_listener, NULL, &run_listener, (void*)&sa), "creando tid_listener");


    THREAD(pthread_join(tid_listener, NULL), "joinando tid_listener");

    for (size_t c = 0; c < val.ThreadsInPool; c++) {
        THREAD(pthread_join(thread_pool[c], NULL), "joinando thread_pool");
    }



    /***** LIBERAZIONE DELLA MEMORIA DINAMICA ALLOCATA *****/

    free(thread_pool);

    destroyList(user_list); //distruggo la lista degli utenti online

    icl_hash_destroy(hashtable, NULL, free_userdata, dim_array_mtex); //distruggo la tab hash degli utenti registrati

    ec_meno1(close(comm_pipe[0]), "close in main"); //chiudo la pipe sia in lett. che in scritt.
    ec_meno1(close(comm_pipe[1]), "close in main");

    free(val.UnixPath);
    free(val.DirName);
    free(val.StatFileName);

    val.UnixPath = NULL;
    val.DirName = NULL;
    val.StatFileName = NULL;

    printf("Chatty terminato con successo!\n");
    fflush(stdout);

    return 0;
}



void sig_manager() {

    sigset_t set;
    struct sigaction siga;

    ec_meno1(sigfillset(&set), "sigfillset error"); //includo nel set tutti i segnali
    ec_meno1(pthread_sigmask(SIG_SETMASK, &set, NULL), "sigmask error"); //maschero tutti i segnali in attesa di installare il gestore

    memset(&siga, 0, sizeof(siga));

    siga.sa_handler = SIG_IGN;

    ec_meno1(sigaction(SIGPIPE, &siga, NULL), "sigaction error"); //ignoro SIGPIPE

    siga.sa_handler = termination_handler;
    //installo termination_handler per SIGINT, SIGTERM, SIGQUIT
    ec_meno1(sigaction(SIGINT, &siga, NULL), "sigaction error");
    ec_meno1(sigaction(SIGTERM, &siga, NULL), "sigaction error");
    ec_meno1(sigaction(SIGQUIT, &siga, NULL), "sigaction error");

    siga.sa_handler = stats_handler; //installo stats_handler per SIGUSR1

    ec_meno1(sigaction(SIGUSR1, &siga, NULL), "sigaction error");

    ec_meno1(sigemptyset(&set), "sigemptyset error"); //svuoto il set
    ec_meno1(pthread_sigmask(SIG_SETMASK, &set, NULL), "sigmask error");
}


void termination_handler(int sig) {

    struct {
        int signum;
        char *msg;
    } sigmsg[] = {
        { SIGTERM, "segnale di terminazione catturato\n"},
        { SIGINT, "segnale di interruzione catturato\n"},
        { SIGQUIT, "segnale di quit catturato\n"},
        { 0, NULL}
    };

    sigstop = 1; //il programma deve terminare
    pthread_cond_broadcast(&cond_pipe);

    for (int i = 0; sigmsg[i].signum > 0; i++) { //scrivo quale segnale ho catturato
        if (sigmsg[i].signum == sig) {
            write(1, sigmsg[i].msg, strlen(sigmsg[i].msg));
            write(1, "\n", 1);
            break;
        }
    }
}

void stats_handler(int sig) {

    sigusr = 1; //il programma deve stampare in un file le statistiche
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

    if (chattyStats->nusers < 0 || chattyStats->nonline < 0 || chattyStats->ndelivered < 0 ||
        chattyStats->nnotdelivered < 0 || chattyStats->nfiledelivered < 0 || chattyStats->nfilenotdelivered < 0 ||
        chattyStats->nerrors < 0){
            printf("error: illegal stat value\n");

            THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in update_stats");

            return -1;
    }

    THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in update_stats");

    return 0;
}



void *run_listener(void * arg){

    int fd_skt, //socket di connessione
        fd_c, //socket di I/O con un client
        fd, //indice per verificare i risultati della select
        fd_num = 0; //massimo fd attivo

    fd_set rdset; //insieme dei fd attesi per la lettura

    //apro la connessione su fd_skt
    fd_skt = socket(AF_UNIX, SOCK_STREAM, 0);

    if (-1 == fd_skt){
        perror("socket thread_listener server");
        return(void *)-1;
    }
    if (-1 == bind(fd_skt, (struct sockaddr *)arg, sizeof(struct sockaddr_un)) ){
        perror("bind socket per le connessioni coi client");
        return(void *)-1;
    }
    if (-1 == listen(fd_skt, SOMAXCONN) ){
        perror("listen socket per le connessioni coi client");
        return(void *)-1;
    }

    struct timeval timeout = {0, 10000};

    if (fd_skt > fd_num) fd_num = fd_skt;
    FD_ZERO(&set);
    FD_SET(fd_skt,&set);

    while (!sigstop) {

        THREAD(pthread_mutex_lock(&mtex_set), "lock in run_listener");
        rdset = set;
        THREAD(pthread_mutex_unlock(&mtex_set), "unlock in run_listener");

        if (sigusr) {
            FILE *statfile = fopen(val.StatFileName, "a+");
            sigusr = 0;

            if(!statfile) {
                printf("open del file delle statistiche fallita, le statistiche richieste non verranno appese al file\n");
            } else {
                THREAD(pthread_mutex_lock(&mtex_stats), "lock in run_listener");
                printStats(statfile);
                THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in run_listener");
            }
        }

        int sel = select((fd_num + 1), &rdset, NULL, NULL, &timeout);
        if (-1 ==  sel){
            perror("errore select thread listener");

        } else if (0 != sel){

            for (fd = 0; fd <= fd_num; fd++) {
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

                            if (fd_c > fd_num) fd_num = fd_c;
                        }
                    } else { //passo il fd ad un thread per elaborare le richieste

                        THREAD(pthread_mutex_lock(&mtex_set), "lock in run_listener");
                        FD_CLR(fd,&set);
                        THREAD(pthread_mutex_unlock(&mtex_set), "unlock in run_listener");

                        if (-1 == write(comm_pipe[1], &fd, sizeof(int))) {
                            perror("write nel listener");
                            continue;
                        }
                        //risveglia un thread del pool in attesa di leggere dalla pipe
                        THREAD(pthread_cond_signal(&cond_pipe),"signal in run_listener");
                    }
                }
            }
        } else if (0 == sel){
            timeout.tv_sec = 0;
            timeout.tv_usec = 10000;
        }
    }


    for (size_t i = 0; i <= fd_num; i++) {
        //chiudo i fd che potrebbero essere rimasti aperti sia in lettura che in scrittura
        if (-1 == shutdown(i, SHUT_RDWR)) {
            if (errno != ENOTSOCK) { //lasciare questo?
                perror("shutdown in thread_listener");
            }
        }
    }

    if (-1 == close(fd_skt)) {
        perror("close in thread_listener");
    }

    pthread_exit(NULL);
}



void* run_pool_element(void *arg){

    message_t msg;

    while (!sigstop) {

        int fd = -1;
        int res = -1;

        THREAD(pthread_mutex_lock(&mtex_pipe), "lock in run_pool_element");

        while (-1 == res) {
            res = read(comm_pipe[0], &fd, sizeof(int)); //la read non e' bloccante

            if (-1 == res) {

                if (errno == EAGAIN) { //non e' stato letto nulla dal thread
                    THREAD(pthread_cond_wait(&cond_pipe, &mtex_pipe), "wait in run_pool_element");

                    if (sigstop) { //se e' stato inviato un segnale e il processo deve terminare
                        THREAD(pthread_mutex_unlock(&mtex_pipe), "unlock in run_pool_element");
                        pthread_exit(NULL);
                    }
                } else {
                    perror("read in un worker; fallimento nella retrieve di un fd");
                    pthread_exit(NULL);
                }
            }
        }

        THREAD(pthread_mutex_unlock(&mtex_pipe), "unlock in run_pool_element");


        int r = readHeader(fd, &msg.hdr); //legge l'header del messaggio dal fd

        if (r > 0){

            int ans = elab_request(fd, msg);

            if (ans < -2) { //deregistrazione e disconnessione implicita dell'utente

                printf("una deregistrazione e' avvenuta con successo\n");
            } else if (-2 == ans) { //utente ha provato a fare operazioni senza essere registrato/connesso

                printf("richiesta illegale, la statistica degli utenti connessi non deve essere aggiornata\n");
            } else if (-1 == ans){ //operazione richiesta dall'utente fallita

                printf("errore nell'elaborazione del messaggio, %d non verra' aggiunto nuovamente alla coda\n", fd);

                if (user_list->numb_elems > 0) {
                    //l'utente viene disconnesso (e quindi eliminato dalla lista dei connessi)
                    user_list = deleteFdFromList(user_list, fd);

                    int check_stats = update_stats(&chattyStats, 0, -1, 0, 0, 0, 0, 0);//diminuisco il numero degli utenti connessi
                }

            } else if (0 == ans){
                //printf("elaborazione del messaggio avvenuta con successo, %d verra' aggiunto nuovamente alla coda\n", fd);

                THREAD(pthread_mutex_lock(&mtex_set), "lock in run_pool_element");
                FD_SET(fd, &set);
                THREAD(pthread_mutex_unlock(&mtex_set), "unlock in run_pool_element");

            } else if (ans > 0) {
                printf("il destinatario di %d ha la history piena, elaborazione del messaggio fallita ma %d verra' aggiunto nuovamente alla coda\n", fd, fd);

                THREAD(pthread_mutex_lock(&mtex_set), "lock in run_pool_element");
                FD_SET(fd, &set);
                THREAD(pthread_mutex_unlock(&mtex_set), "unlock in run_pool_element");
            }

        } else { //se r <= 0

            //l'utente viene disconnesso (e quindi eliminato dalla lista dei connessi)
            user_list = deleteFdFromList(user_list, fd);

            if (0 < chattyStats.nonline) {
                update_stats(&chattyStats, 0, -1, 0, 0, 0, 0, 0);//diminuisco il numero degli utenti connessi
            }

            printf("lettura su %d terminata, il fd non verra' aggiunto nuovamente alla coda\n", fd);
        }
    }

    pthread_exit(NULL);
}



int elab_request(int fd_c, message_t message){

    message_t reply_msg;

    char *sender = strdup(message.hdr.sender);

    printf("l'utente %s sta lavorando sul fd %d\n", sender, fd_c);

    //se la lettura dei dati del messaggio fallisce
    if(readData(fd_c, &message.data) <= 0){
        printf("errore: fallimento lettura dati\n");
        free(message.data.buf);
        free(sender);
        update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

        return -1;
    }

    //elaboro le richieste
    switch (message.hdr.op) {


//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//


        case REGISTER_OP: {

            printf("%s: richiesta di registrazione\n", sender);

            user_data_t *usrdt = (user_data_t *)malloc(sizeof(user_data_t));
            memset(usrdt, 0, sizeof(user_data_t));

            //errore nell'inizializzazione della struttura che contiene i dati dell'utente
            if (-1 == user_data_init(usrdt, sender, fd_c, val.MaxMsgSize, val.MaxFileSize, val.MaxHistMsgs)){

                free(message.data.buf);
                free(sender);
                free_userdata(usrdt);

                return -2;
                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
            }


            THREAD(pthread_mutex_lock(&mtex_stats), "lock in elab_request");
            //e' gia' stato raggiunto MaxConnections: la registrazione fallisce
            if (chattyStats.nonline >= val.MaxConnections){

                printf("numero massimo di connessioni contemporanee raggiunto, registrazione fallita\n");
                THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in elab_request");

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);
                free_userdata(usrdt);

                return -2;
            }

            THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in elab_request");

            //esiste gia' un utente con quel nickname
            if (NULL != icl_hash_find(hashtable, sender)){

                printf("nickname %s gia' iscritto a chatty\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_ALREADY, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);
                free_userdata(usrdt);

                return -2;
            }

            printf("sto inserendo il nick: %s\n", sender);
            //icl_hash_insert fallisce
            if(NULL == icl_hash_insert(hashtable, (void *)sender, (void *) usrdt)){
                printf("errore nella registrazione di %s\n", sender);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);
                free_userdata(usrdt);

                return -2;
            }

            //icl_hash_insert ha avuto successo

            update_stats(&chattyStats, 1, 1, 0, 0, 0, 0, 0); //aumento il numero degli utenti registrati e dei connessi

            user_list = insertListHead(user_list, sender, fd_c); //aggiunge il nick e il fd alla lista degli utenti connessi

            char *buffer = toBuf(user_list); //memorizza nel buffer da inviare la lista degli utenti connessi
            int length = user_list->numb_elems * (MAX_NAME_LENGTH + 1); //lunghezza del buffer

            //setto Header e Dati per la risposta del server al client
            setHeader(&reply_msg.hdr, OP_OK, "");

            if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                free(message.data.buf);
                free(buffer);
                free(sender);

                return -2;
            }

            setData(&reply_msg.data, "", buffer, length);

            if (sendData(fd_c, &reply_msg.data) < 0) {
                free(message.data.buf);
                free(buffer);
                free(sender);

                return -2;
            }
            free(message.data.buf);
            free(buffer);
            free(sender);

        } break;


//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//


        case CONNECT_OP: {

            printf("%s: richiesta di connessione\n", sender);

            user_data_t *usrdt = (user_data_t *)icl_hash_find(hashtable, sender);

            //il client che tenta di connettersi non e' registrato
            if (NULL == usrdt){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, ""); //come sender metto "server"?
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }


            THREAD(pthread_mutex_lock(&mtex_stats), "lock in elab_request");
            //e' gia' stato raggiunto MaxConnections: il nuovo tentativo di connessione fallisce
            if (chattyStats.nonline >= val.MaxConnections){

                printf("numero massimo di connessioni contemporanee raggiunto, connessione fallita\n");
                THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in elab_request");

                setHeader(&reply_msg.hdr, OP_FAIL, ""); //come sender metto "server"?
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }

            THREAD(pthread_mutex_unlock(&mtex_stats), "unlock in elab_request");


            //il client riesce a connettersi
            update_stats(&chattyStats, 0, 1, 0, 0, 0, 0, 0); //aumento il numero degli utenti connessi

            user_list = insertListHead(user_list, sender, fd_c); //aggiunge il nick e il fd alla lista degli utenti connessi

            char * buffer =  toBuf(user_list); //memorizza nel buffer da inviare la lista degli utenti connessi
            int length = user_list->numb_elems * (MAX_NAME_LENGTH + 1); //lunghezza del buffer

            //aggiorno il fd relativo all'utente col nuovo fd sulla quale avverra' la comunicazione client-server
            update_user_fd(usrdt, fd_c);

            //setto Header e Dati per la risposta del server al client
            setHeader(&reply_msg.hdr, OP_OK, "");

            if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                free(message.data.buf);
                free(buffer);
                free(sender);

                return -1;
            }

            setData(&reply_msg.data, "", buffer, length);

            if (sendData(fd_c, &reply_msg.data) < 0) {
                free(message.data.buf);
                free(sender);
                free(buffer);

                return -1;
            }
            free(buffer);
            free(message.data.buf);
            printf("%s: connessione riuscita, egli verra' ascoltato di nuovo\n", sender);
            free(sender);

        } break;


//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//


        case POSTTXT_OP: {

            printf("%s: richiesta di invio a un utente di un messaggio\n", sender);
            //il client che tenta di inviare un messaggio non e' registrato
            if (NULL == icl_hash_find(hashtable, sender)){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }


            //il client che tenta di inviare un messaggio non e' connesso
            if (NULL == listFind(user_list, sender)){

                printf("il client %s non e' connesso\n", sender);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }


            char *receiver = message.data.hdr.receiver; //nickname del destinatario
            int buf_len = message.data.hdr.len; //lunghezza del messaggio


            user_data_t *dest = icl_hash_find(hashtable, receiver);

            //se il nickname del destinatario non e' registrato
            if (NULL == dest) {

                printf("il nickname destinatario scelto non e' iscritto a Chatty\n");

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -1;
            }


            //se il messaggio e' troppo lungo
            if (buf_len > val.MaxMsgSize) {

                printf("messaggio troppo lungo\n");

                setHeader(&reply_msg.hdr, OP_MSG_TOOLONG, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -1;
            }

            //operazione ha avuto successo

            //setta l'header del messaggio di risposta dal server per il mittente di POSTTXT_OP
            setHeader(&reply_msg.hdr, OP_OK, "");

            message_t msg_to_deliver;

            setHeader(&msg_to_deliver.hdr, TXT_MESSAGE, sender);
            setData(&msg_to_deliver.data, receiver, message.data.buf, message.data.hdr.len);

            int ans = sendRequest(dest->fd, &msg_to_deliver);

            if (-1 == ans) { //se il destinatario e' offline

                printf("il destinatario e' offline, aggiungo il messaggio alla lista dei messaggi che deve leggere\n");

                update_stats(&chattyStats, 0, 0, 0, 1, 0, 0, 0); //aumento il numero dei messaggi non letti

                //aggiorno la history del destinatario e marco il messaggio come non letto
                if (-1 == add_to_history(dest, &msg_to_deliver, 0, 0)){

                    if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                        free(message.data.buf);
                        printf("%s: richiesta di post messaggio fallita errore lettura header\n", sender);
                        free(sender);
                        return -1;
                    }
                    free(message.data.buf);
                    printf("%s: richiesta di post messaggio fallita history piena\n", sender);
                    free(sender);

                    return 1; //history del destinatario piena, il fd del mittente verra' rimesso in coda
                }


            } else { //se il destinatario e' online

                printf("il destinatario e' online, messaggio inviato con successo\n");

                update_stats(&chattyStats, 0, 0, 1, 0, 0, 0, 0);//aumento il numero dei messaggi letti

                //aggiorno la history del destinatario e marco il messaggio come letto
                if (-1 == add_to_history(dest, &msg_to_deliver, 1, 0)){

                    if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                        free(message.data.buf);
                        free(sender);
                        printf("%s: richiesta di post messaggio fallita errore header\n", sender);
                        return -1;
                    }
                    free(message.data.buf);
                    printf("%s: richiesta di post messaggio fallita history piena\n", sender);
                    free(sender);

                    return 1; //history del destinatario piena, il fd del mittente verra' rimesso in coda
                }
            }


            if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                free(message.data.buf);
                free(sender);
                return -1;
            }
            free(message.data.buf);
            printf("%s: post messaggio riuscita, egli verra' ascoltato di nuovo\n", sender);
            free(sender);

        } break;


//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//


        case POSTTXTALL_OP: {

            printf("%s: richiesta di invio a tutti di un messaggio\n", sender);

            //il client che tenta di inviare un messaggio non e' registrato
            if (NULL == icl_hash_find(hashtable, sender)){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }


            //il client che tenta di inviare un messaggio non e' connesso
            if (NULL == listFind(user_list, sender)){

                printf("il client %s non e' connesso\n", sender);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }

            int buf_len = message.data.hdr.len;

            //se il messaggio e' troppo lungo
            if (buf_len > val.MaxMsgSize) {

                printf("messaggio troppo lungo\n");

                setHeader(&reply_msg.hdr, OP_MSG_TOOLONG, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -1;
            }

            //operazione ha avuto successo

            //setta l'header del messaggio di risposta dal server per il mittente di POSTTXTALL_OP
            setHeader(&reply_msg.hdr, OP_OK, "");

            message_t msg_to_deliver; //messaggio da inviare a tutti gli utenti registrati

            setHeader(&msg_to_deliver.hdr, TXT_MESSAGE, sender);
            setData(&msg_to_deliver.data, "", message.data.buf, message.data.hdr.len);

            int nonline = 0;
            int noffline = 0; //numero di utenti online e offline al momento dell'invio a tutti del messaggio


            add_to_history_all(hashtable, sender, &msg_to_deliver, &nonline, &noffline);

            //aumento sia il numero dei messaggi consegnati che quelli non consegnati
            //in accordo all'aggiornamento dei contatori nonline e noffline fatto da add_to_history_all
            update_stats(&chattyStats, 0, 0, nonline, noffline, 0, 0, 0);

            printf("i messaggi consegnati con successo sono %d, quelli non consegnati sono %d\n", nonline, noffline);

            if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                free(message.data.buf);
                free(sender);

                return -1;
            }
            free(message.data.buf);
            printf("%s: post messaggio a tutti riuscita, egli verra' ascoltato di nuovo\n", sender);
            free(sender);

        } break;


//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//


        case POSTFILE_OP: {

            printf("%s: richiesta di post di un file\n", sender);

            //il client che tenta di inviare il file non e' registrato
            if (NULL == icl_hash_find(hashtable, sender)){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, "");
                sendHeader(fd_c, &reply_msg.hdr);

                printf("%s: richiesta di post di un file fallita perche l'utente non e; regisrato\n", sender);
                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }


            //il client che tenta di inviare il file non e' connesso
            if (NULL == listFind(user_list, sender)){

                printf("il client %s non e' connesso\n", sender);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                printf("%s: richiesta di post di un file fallita perche l'utente non e; connesso\n", sender);
                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }


            message_data_t fileData; //ci memorizzo i dati del file
            readData(fd_c, &fileData);

            printf("questa e' la size del file che si vuole inviare: %d\n", fileData.hdr.len);

            //se il file e' troppo grande per essere inviato
            if (fileData.hdr.len > (val.MaxFileSize * 1024)) { //moltiplico per 1024 perche' MaxFileSize e' espressa in Kbytes

                printf("errore: file troppo grande per essere inviato\n");

                free(fileData.buf);
                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1);

                setHeader(&reply_msg.hdr, OP_MSG_TOOLONG, "");
                if(sendHeader(fd_c, &reply_msg.hdr) <= 0){
                    printf("%s: richiesta di post di un file fallita perche il file e' trppo grande\n", sender);

                    free(message.data.buf);
                    free(sender);

                    return -1;
                }
                printf("%s: richiesta di post di un file fallita perche il file e' trppo grande\n", sender);
                free(message.data.buf);
                free(sender);

                return -1;
            }


            char *receiver = message.data.hdr.receiver; //nickname del destinatario

            user_data_t *dest = icl_hash_find(hashtable, receiver);

            //se il nickname del destinatario non e' registrato a Chatty
            if (NULL == dest) {

                printf("il nickname destinatario scelto non e' iscritto a Chatty\n");

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, "");
                sendHeader(fd_c, &reply_msg.hdr);

                free(fileData.buf);
                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori

                printf("%s: richiesta di post di un file fallita perche il destinatario non e; registrato a chatty\n", sender);
                free(message.data.buf);
                free(sender);

                return -1;
            }

            //il file puo' essere inviato

            int pathlen = strlen(val.DirName) + strlen(message.data.buf) + 2; // byte aggiuntivo per lo slash l'altro per il terminatore

            //costruisco il path del file
            char *path = (char *)malloc(pathlen * sizeof(char));
            memset(path, 0, pathlen * sizeof(char));

            strncpy(path, val.DirName, strlen(val.DirName));
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
            free(path);

            if (!file_to_deliver){ //se fallisce l'apertura
                printf("errore nell'open del file in %s\n", path);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                printf("%s: richiesta di post di un file fallita perche fallimento apertura del file\n", sender);
                free(fileData.buf);
                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -1;
            }

            int x = fwrite(fileData.buf, sizeof(char), fileData.hdr.len, file_to_deliver);

            if (x != fileData.hdr.len) { //se c'e' un errore nella scrittura
                printf("errore nella scrittura\n");
                free(fileData.buf);
                free(message.data.buf);
                printf("%s: richiesta di post di un file fallita perche errore nella scrittura\n", sender);
                free(sender);

                return -1;
            }

            free(fileData.buf);

            if (fclose(file_to_deliver) != 0) {
                printf("errore nella chiusura del file da inviare\n");
                free(message.data.buf);
                printf("%s: richiesta di post di un file fallita perche errore nella close\n", sender);
                free(sender);

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

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 1, 0); //aumento il numero dei file non consegnati

                //aggiorno la history del destinatario e marco il messaggio come non letto
                if (-1 == add_to_history(dest, &msg_to_deliver, 0, 0)){

                    if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                        free(message.data.buf);
                        printf("%s: richiesta di post di un file fallita errore nel send header\n", sender);
                        free(sender);

                        return -1;
                    }
                    free(message.data.buf);
                    printf("%s: richiesta di post di un file fallita history del destinatario piena\n", sender);
                    free(sender);

                    return 1; //history del destinatario piena, il fd del mittente verra' rimesso in coda
                }

            } else { //se il destinatario e' online

                printf("il destinatario e' online, messaggio file inviato con successo\n");

                //aumento il numero dei file non consegnati (un file e' consegnato quando l'utente lo riceve a seguito di una GETFILE_OP)
                update_stats(&chattyStats, 0, 0, 0, 0, 0, 1, 0);

                //aggiorno la history del destinatario e marco il messaggio come letto
                if (-1 == add_to_history(dest, &msg_to_deliver, 1, 0)){

                    if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                        free(message.data.buf);
                        printf("%s: richiesta di post di un file fallita errore lettura header\n", sender);
                        free(sender);
                        return -1;
                    }
                    free(message.data.buf);
                    printf("%s: richiesta di post di un file fallita history del destinatario piena\n", sender);
                    free(sender);


                    return 1; //history del destinatario piena, il fd del mittente verra' rimesso in coda
                }
            }


            if(sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                free(message.data.buf);
                free(sender);

                return -1;
            }
            free(message.data.buf);
            printf("%s: post file riuscita, egli verra' ascoltato di nuovo\n", sender);
            free(sender);

        } break;


//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//


        case GETFILE_OP: {

            printf("%s: richiesta di get di un file\n", sender);

            //il client che tenta di ricevere un file non e' registrato
            if (NULL == icl_hash_find(hashtable, sender)){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }


            //il client che tenta di ricevere un file non e' connesso
            if (NULL == listFind(user_list, sender)){

                printf("il client %s non e' connesso\n", sender);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }


            int pathlen = strlen(val.DirName) + strlen(message.data.buf) + 2; // byte aggiuntivo per lo slash l'altro per il terminatore

            //costruisco il path del file
            char *path = (char *)malloc(pathlen * sizeof(char));
            memset(path, 0, pathlen * sizeof(char));

            strncpy(path, val.DirName, strlen(val.DirName));
            strcat(path, "/");

            char *slash  = strrchr(message.data.buf, '/'); //controllo se esiste uno slash in message.data.buf

            if (NULL == slash) {
                strncat(path, message.data.buf, message.data.hdr.len);
            } else {
                slash ++;
                int len = message.data.hdr.len - (slash - message.data.buf);
                strncat(path, slash, len);
            }


            //apro il file
            FILE *file_to_deliver = fopen(path, "r");
            free(path);

            if (!file_to_deliver){ //se fallisce l'apertura
                printf("errore nell'open del file in %s\n", path);
                setHeader(&reply_msg.hdr, OP_NO_SUCH_FILE, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -1;
            }


            //dimensione del file da inviare
            fseek(file_to_deliver, 0L, SEEK_END);
            int filesize = ftell(file_to_deliver);
            rewind(file_to_deliver);

            //alloco la memoria per il buffer che ospitera' il contenuto del file
            char *buffer = (char *)malloc((filesize+1) * sizeof(char));
            memset(buffer, 0, (filesize+1) * sizeof(char));

            int x = fread(buffer, sizeof(char), filesize, file_to_deliver);

            if (x != filesize) { //se c'e' un errore nella lettura
                printf("errore nella lettura\n");
                free(buffer);
                free(message.data.buf);
                free(sender);

                return -1;
            }

            if (fclose(file_to_deliver) != 0) {
                printf("errore nella chiusura del file da inviare\n");
                free(buffer);
                free(message.data.buf);
                free(sender);

                return -1;
            }

            //operazione ha avuto successo

            //aumento il numero dei file consegnati e diminuisco quello dei file non consegnati
            update_stats(&chattyStats, 0, 0, 0, 0, 1, -1, 0);

            //setta l'header del messaggio di risposta dal server per il mittente di GETFILE_OP
            setHeader(&reply_msg.hdr, OP_OK, "");

            if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                free(buffer);
                free(message.data.buf);
                free(sender);

                return -1;
            }

            setData(&reply_msg.data, sender, buffer, filesize + 1);

            if (sendData(fd_c, &reply_msg.data) < 0) {
                free(buffer);
                free(message.data.buf);
                free(sender);

                return -1;
            }

            free(buffer);
            free(message.data.buf);
            printf("%s: get file riuscita, egli verra' ascoltato di nuovo\n", sender);
            free(sender);

        } break;


//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//


        case GETPREVMSGS_OP: {

            printf("%s: richiesta di recupero dei messaggi nella history\n", sender);

            user_data_t *usrdt = (user_data_t *)icl_hash_find(hashtable, sender);
            //il client che tenta di ottenere i messaggi nella history non e' registrato
            if (NULL == usrdt){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }

            //il client che tenta di ottenere i messaggi nella history non e' connesso
            if (NULL == listFind(user_list, sender)){

                printf("il client %s non e' connesso\n", sender);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }

            //l'operazione ha successo

            //setto Header e Dati per la risposta del server al client
            setHeader(&reply_msg.hdr, OP_OK, "");

            if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                free(message.data.buf);
                free(sender);

                return -1;
            }

            size_t x = usrdt->size;

            setData(&reply_msg.data, "", (char *)&x, sizeof(size_t));

            if (sendData(fd_c, &reply_msg.data) < 0) {
                free(message.data.buf);
                free(sender);

                return -1;
            }

            int read_now = 0; //contatore dei messaggi consegnati per la prima volta in questa GETPREVMSGS_OP

            //invia all'utente ogni messaggio presente nella history
            for (int i = 0; i < usrdt->size; i++) {
                if (-1 == sendRequest(fd_c, retrieve_from_history(usrdt, i, &read_now))) {
                    printf("l'utente %s ha fallito il retrieve del messaggio %d della history\n", sender, i);
                }
            }

            printf("l'utente %s ha ricevuto lo storico messaggi con successo\n", sender);
            //aggiorno le statistiche: sono stati consegnati read_now messaggi di quelli non ancora letti
            update_stats(&chattyStats, 0, 0, read_now, (-read_now), 0, 0, 0);
            free(message.data.buf);
            printf("%s: get dello storico riuscita, egli verra' ascoltato di nuovo\n", sender);
            free(sender);

        } break;


//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//


        case USRLIST_OP: {

            printf("%s: richiesta della lista degli utenti\n", sender);

            //il client che tenta di ottenere la lista degli utenti non e' registrato
            if (NULL == icl_hash_find(hashtable, sender)){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }

            //il client che tenta di ottenere la lista degli utenti non e' connesso
            if (NULL == listFind(user_list, sender)){

                printf("il client %s non e' connesso\n", sender);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }

            //operazione ha avuto successo
            char * buffer = toBuf(user_list); //memorizza nel buffer da inviare la lista degli utenti connessi
            int length = user_list->numb_elems * (MAX_NAME_LENGTH + 1); //lunghezza del buffer

            //setto Header e Dati per la risposta del server al client
            setHeader(&reply_msg.hdr, OP_OK, "");

            if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                free(message.data.buf);
                free(buffer);
                free(sender);

                return -1;
            }

            setData(&reply_msg.data, "", buffer, length);

            if (sendData(fd_c, &reply_msg.data) < 0) {
                free(message.data.buf);
                free(buffer);
                free(sender);

                return -1;
            }
            free(message.data.buf);
            free(buffer);
            printf("%s: get lista utenti riuscita, egli verra' ascoltato di nuovo\n", sender);
            free(sender);

        } break;


//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//

        case UNREGISTER_OP: {

            printf("%s: richiesta di deregistrazione\n", sender);
            user_data_t *usrdt = (user_data_t *)icl_hash_find(hashtable, sender);

            //il client che tenta di deregistrarsi non e' registrato
            if (NULL == usrdt){

                printf("il client %s non e' registrato\n", sender);

                setHeader(&reply_msg.hdr, OP_NICK_UNKNOWN, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }


            //il client che tenta di deregistrarsi non e' connesso
            if (NULL == listFind(user_list, sender)){

                printf("il client %s non e' connesso\n", sender);

                setHeader(&reply_msg.hdr, OP_FAIL, "");
                sendHeader(fd_c, &reply_msg.hdr);

                update_stats(&chattyStats, 0, 0, 0, 0, 0, 0, 1); //aumento il numero degli errori
                free(message.data.buf);
                free(sender);

                return -2;
            }

            //l'operazione ha successo

            //l'utente viene disconnesso (e quindi eliminato dalla lista dei connessi)
            printf("numero degli el della lista: %d\n", user_list->numb_elems);
            user_list = deleteNameFromList(user_list, sender);
            printf("numero degli el della lista: %d\n", user_list->numb_elems);

            update_stats(&chattyStats, 0, -1, 0, 0, 0, 0, 0);//diminuisco il numero degli utenti connessi

            free_userdata(usrdt); //si libera la memoria allocata per lo storico messaggi

            //l'utente viene deregistrato
            if (0 != icl_hash_delete(hashtable, sender, NULL, NULL)){

                printf("utente %s deregistrazione fallita\n", sender);
                free(message.data.buf);
                free(sender);

                return -1;
            }

            update_stats(&chattyStats, -1, 0, 0, 0, 0, 0, 0);//diminuisco il numero degli utenti registrati


            setHeader(&reply_msg.hdr, OP_OK, "");
            if (sendHeader(fd_c, &reply_msg.hdr) <= 0) {
                free(message.data.buf);
                free(sender);

                return -1;
            }
            free(message.data.buf);
            free(sender);

            return -3;

        } break;


//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//
//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo//


        default: {

            printf("operazione non riconosciuta\n");
            setHeader(&reply_msg.hdr, OP_FAIL, "");
            sendHeader(fd_c, &reply_msg.hdr);
            free(message.data.buf);
            free(sender);

            return -1;
        } break;
    }
    return 0;
}
