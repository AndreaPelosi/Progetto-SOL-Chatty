/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file conf_parsing.c
 * @brief Implementazione della funzione necessaria per il parsing del file di
            configurazione del server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conf_parsing.h>
#include <macrosctest.h>

/**
    @function conf_init
    @brief parsa uno dei file di configurazione e setta il server con i valori del file

    @param conffile     percorso del file di configurazione
    @param val        struttura dove vengono memorizzati i valori di configurazione

    @return 0 se ha successo, EXIT_FAILURE atrimenti
*/
int conf_init( char *conffile, struct conf_values *val) {
    FILE *f1;
    char *buf;
    char *token; //token per strtok
    char *endptr; //per strtol
    int tksize; //size di token
    int flag = 1; //flag che diventa 0 quando fgets su conffile raggiunge EOF
    val->UnixPath = NULL;
    val->MaxConnections = -1;
    val->ThreadsInPool = -1;
    val->MaxMsgSize = -1;
    val->MaxFileSize = -1;
    val->MaxHistMsgs = -1;
    val->DirName = NULL;
    val->StatFileName = NULL;

    ec_null(buf = (char *)malloc(MAX_CONF_FILE_LINE_SIZE*sizeof(char)), "conf malloc error");
    ec_null(f1 = fopen(conffile, "r"), "conf file open error");

    while (flag) {
        buf = fgets(buf, MAX_CONF_FILE_LINE_SIZE, f1);

        if (NULL == buf){
            flag = 0;
            if ((NULL == val->UnixPath) || (0 >= val->MaxConnections) || (0 >= val->ThreadsInPool) || (0 >= val->MaxMsgSize) \
                || (0 >= val->MaxFileSize) || (0 >= val->MaxHistMsgs) || (NULL == val->DirName) || (NULL == val->StatFileName)){
                    fprintf(stderr, "invalid conf file parsed\n");
                    exit(EXIT_FAILURE);
                }
        }
        // else if ('\n' == buf[0]) {
//
//        }
         else if (buf[0] != '#') { //se buf[0] e' #, significa che la riga del file di configurazione e' un commento e va ignorata
            token = strtok(buf, " \t\n");
        //    printf("token : %s\n", token);
            while (token) {
        //        printf("ciao3, %s\n", token);
                if (0 == strcmp(token, "UnixPath")){
        //            printf("ciao4\n");
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        exit(EXIT_FAILURE);
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            exit(EXIT_FAILURE);
                        } else {
                            tksize = strlen(token);
                            ec_null(val->UnixPath = (char *)malloc((tksize + 1)*sizeof(char)), "conf malloc error");
                            val->UnixPath = strcpy(val->UnixPath, token);
                            token = NULL;
                        }
                    }
                } else if (0 == strcmp(token, "DirName")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        exit(EXIT_FAILURE);
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            exit(EXIT_FAILURE);
                        } else {
                            tksize = strlen(token);
                            ec_null(val->DirName = (char *)malloc((tksize + 1)*sizeof(char)), "conf malloc error");
                            val->DirName = strcpy(val->DirName, token);
                            token = NULL;
                        }
                    }
                } else if (0 == strcmp(token, "StatFileName")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        exit(EXIT_FAILURE);
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            exit(EXIT_FAILURE);
                        } else {
                            tksize = strlen(token);
                            ec_null(val->StatFileName = (char *)malloc((tksize + 1)*sizeof(char)), "conf malloc error");
                            val->StatFileName = strcpy(val->StatFileName, token);
                            token = NULL;
                        }
                    }
                } else if (0 == strcmp(token, "MaxConnections")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        exit(EXIT_FAILURE);
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            exit(EXIT_FAILURE);
                        } else {
                            val->MaxConnections = strtol(token, &endptr, 10);
                            if (endptr == token){
                                fprintf(stderr, "invalid conf file parsed\n");
                                exit(EXIT_FAILURE);
                            }
                            if (*endptr != '\0' && *endptr != '\n'){
                                fprintf(stderr, "invalid conf file parsed\n");
                                exit(EXIT_FAILURE);
                            }
                            token = NULL;
                        }
                    }
                } else if (0 == strcmp(token, "ThreadsInPool")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        exit(EXIT_FAILURE);
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            exit(EXIT_FAILURE);
                        } else {
                            val->ThreadsInPool = strtol(token, &endptr, 10);
                            if (endptr == token){
                                fprintf(stderr, "invalid conf file parsed\n");
                                exit(EXIT_FAILURE);
                            }
                            if (*endptr != '\0' && *endptr != '\n'){
                                fprintf(stderr, "invalid conf file parsed\n");
                                exit(EXIT_FAILURE);
                            }
                            token = NULL;
                        }
                    }
                } else if (0 == strcmp(token, "MaxMsgSize")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        exit(EXIT_FAILURE);
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            exit(EXIT_FAILURE);
                        } else {
                            val->MaxMsgSize = strtol(token, &endptr, 10);
                            if (endptr == token){
                                fprintf(stderr, "invalid conf file parsed\n");
                                exit(EXIT_FAILURE);
                            }
                            if (*endptr != '\0' && *endptr != '\n'){
                                fprintf(stderr, "invalid conf file parsed\n");
                                exit(EXIT_FAILURE);
                            }
                            token = NULL;
                        }
                    }
                } else if (0 == strcmp(token, "MaxFileSize")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        exit(EXIT_FAILURE);
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            exit(EXIT_FAILURE);
                        } else {
                            val->MaxFileSize = strtol(token, &endptr, 10);
                            if (endptr == token){
                                fprintf(stderr, "invalid conf file parsed\n");
                                exit(EXIT_FAILURE);
                            }
                            if (*endptr != '\0' && *endptr != '\n'){
                                fprintf(stderr, "invalid conf file parsed\n");
                                exit(EXIT_FAILURE);
                            }
                            token = NULL;
                        }
                    }
                } else if (0 == strcmp(token, "MaxHistMsgs")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        exit(EXIT_FAILURE);
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            exit(EXIT_FAILURE);
                        } else {
                            val->MaxHistMsgs = strtol(token, &endptr, 10);
                            if (endptr == token){
                                fprintf(stderr, "invalid conf file parsed\n");
                                exit(EXIT_FAILURE);
                            }
                            if (*endptr != '\0' && *endptr != '\n'){
                                fprintf(stderr, "invalid conf file parsed\n");
                                exit(EXIT_FAILURE);
                            }
                            token = NULL;
                        }
                    }
                } else {
                    token = NULL;
                }
            }
        }
    }

    ec_meno1( fclose(f1), "conf file close error");

   printf(" val.UnixPath = %s; \n val.MaxConnections = %d; \n val.ThreadsInPool = %d;\n val.MaxMsgSize = %d;\n val.MaxFileSize = %d;\n \
val.MaxHistMsgs = %d; \n val.DirName = %s; \n val.StatFileName = %s; \n", val->UnixPath, val->MaxConnections, val->ThreadsInPool, val->MaxMsgSize, \
    val->MaxFileSize, val->MaxHistMsgs, val->DirName, val->StatFileName);

    return 0;
}
