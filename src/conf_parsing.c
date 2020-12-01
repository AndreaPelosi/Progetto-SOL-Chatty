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
 *           configurazione del server
 *
 * @author Andrea Pelosi 547596
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conf_parsing.h>
#include <macrosctest.h>


int conf_init( char *conffile, struct conf_values *val) {
    FILE *f1;
    char *buf;
    char *token; //token per strtok
    char *endptr; //per strtol
    int tksize; //size di token

    val->UnixPath = NULL;
    val->MaxConnections = -1;
    val->ThreadsInPool = -1;
    val->MaxMsgSize = -1;
    val->MaxFileSize = -1;
    val->MaxHistMsgs = -1;
    val->DirName = NULL;
    val->StatFileName = NULL;

    buf = (char *)malloc(MAX_CONF_FILE_LINE_SIZE*sizeof(char));
    if (!buf) {
        return -1;
    }
    memset(buf, 0, MAX_CONF_FILE_LINE_SIZE*sizeof(char));
    f1 = fopen(conffile, "r");
    if (!f1) {
        free(buf);
        return -1;
    }

    while (fgets(buf, MAX_CONF_FILE_LINE_SIZE, f1)) {

         if (buf[0] != '#') { //se buf[0] e' #, significa che la riga del file di configurazione e' un commento e va ignorata
            token = strtok(buf, " \t\n");

            while (token) {

                if (0 == strcmp(token, "UnixPath")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        continue;
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            continue;
                        } else {
                            tksize = strlen(token);
                            val->UnixPath = (char *)malloc((tksize + 1)*sizeof(char));
                            if(!val->UnixPath){
                                continue;
                            }
                            val->UnixPath = strcpy(val->UnixPath, token);
                            token = NULL;
                        }
                    }
                } else if (0 == strcmp(token, "DirName")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        continue;
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            continue;
                        } else {
                            tksize = strlen(token);
                            val->DirName = (char *)malloc((tksize + 1)*sizeof(char));
                            if(!val->DirName){
                                continue;
                            }
                            val->DirName = strcpy(val->DirName, token);
                            token = NULL;
                        }
                    }
                } else if (0 == strcmp(token, "StatFileName")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        continue;
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            continue;
                        } else {
                            tksize = strlen(token);
                            val->StatFileName = (char *)malloc((tksize + 1)*sizeof(char));
                            if(!val->StatFileName){
                                continue;
                            }
                            val->StatFileName = strcpy(val->StatFileName, token);
                            token = NULL;
                        }
                    }
                } else if (0 == strcmp(token, "MaxConnections")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        continue;
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            continue;
                        } else {
                            val->MaxConnections = strtol(token, &endptr, 10);
                            if (endptr == token){
                                fprintf(stderr, "invalid conf file parsed\n");
                                continue;
                            }
                            if (*endptr != '\0' && *endptr != '\n'){
                                fprintf(stderr, "invalid conf file parsed\n");
                                continue;
                            }
                            token = NULL;
                        }
                    }
                } else if (0 == strcmp(token, "ThreadsInPool")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        continue;
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            continue;
                        } else {
                            val->ThreadsInPool = strtol(token, &endptr, 10);
                            if (endptr == token){
                                fprintf(stderr, "invalid conf file parsed\n");
                                continue;
                            }
                            if (*endptr != '\0' && *endptr != '\n'){
                                fprintf(stderr, "invalid conf file parsed\n");
                                continue;
                            }
                            token = NULL;
                        }
                    }
                } else if (0 == strcmp(token, "MaxMsgSize")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        continue;
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            continue;
                        } else {
                            val->MaxMsgSize = strtol(token, &endptr, 10);
                            if (endptr == token){
                                fprintf(stderr, "invalid conf file parsed\n");
                                continue;
                            }
                            if (*endptr != '\0' && *endptr != '\n'){
                                fprintf(stderr, "invalid conf file parsed\n");
                                continue;
                                }
                            token = NULL;
                        }
                    }
                } else if (0 == strcmp(token, "MaxFileSize")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        continue;
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            continue;
                        } else {
                            val->MaxFileSize = strtol(token, &endptr, 10);
                            if (endptr == token){
                                fprintf(stderr, "invalid conf file parsed\n");
                                continue;
                            }
                            if (*endptr != '\0' && *endptr != '\n'){
                                fprintf(stderr, "invalid conf file parsed\n");
                                continue;
                            }
                            token = NULL;
                        }
                    }
                } else if (0 == strcmp(token, "MaxHistMsgs")){
                    token = strtok(NULL, " \t\n"); //parsing della stringa "="
                    if (NULL == token){
                        fprintf(stderr, "invalid conf file parsed\n");
                        continue;
                    } else {
                        token = strtok(NULL, " \t\n");
                        if (NULL == token){
                            fprintf(stderr, "invalid conf file parsed\n");
                            continue;
                        } else {
                            val->MaxHistMsgs = strtol(token, &endptr, 10);
                            if (endptr == token){
                                fprintf(stderr, "invalid conf file parsed\n");
                                continue;
                            }
                            if (*endptr != '\0' && *endptr != '\n'){
                                fprintf(stderr, "invalid conf file parsed\n");
                                continue;
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

    free(buf);
    buf = NULL;

    ec_meno1( fclose(f1), "conf file close error");

    if ((NULL == val->UnixPath) || (0 >= val->MaxConnections) || (0 >= val->ThreadsInPool) || (0 >= val->MaxMsgSize) \
        || (0 >= val->MaxFileSize) || (0 >= val->MaxHistMsgs) || (NULL == val->DirName) || (NULL == val->StatFileName)){
            fprintf(stderr, "invalid conf file parsed\n");
            return -1;
    }

   printf(" val.UnixPath = %s; \n val.MaxConnections = %d; \n val.ThreadsInPool = %d;\n val.MaxMsgSize = %d;\n val.MaxFileSize = %d;\n \
val.MaxHistMsgs = %d; \n val.DirName = %s; \n val.StatFileName = %s; \n", val->UnixPath, val->MaxConnections, val->ThreadsInPool, val->MaxMsgSize, \
    val->MaxFileSize, val->MaxHistMsgs, val->DirName, val->StatFileName);

    return 0;
}
