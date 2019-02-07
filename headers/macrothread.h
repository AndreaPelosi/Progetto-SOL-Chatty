/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file macrothread.h
 * @brief contiene macro per il controllo sistematico delle SC
 *
 * @author Andrea Pelosi 547596
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
int err;

#define THREAD(c,m)\
if ((err = (c)) != 0){errno = err;perror(m);exit(EXIT_FAILURE);}
