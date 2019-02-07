/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file macrosctest.h
 * @brief contiene macro per il controllo sistematico delle SC
 *
 * @author Andrea Pelosi 547596
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */

/* controlla -1; stampa errore e termina */
#define ec_meno1(s,m) \
if ( (s) == -1 ) {perror(m); return -1;}

/* controlla NULL; stampa errore e termina la funzione  */
#define ec_null(s,m) \
if((s) == NULL) {perror(m); return -1;}

/* controlla -1; stampa errore ed esegue c */
#define ec_meno1_c(s,m,c) \
if((s) == -1) {perror(m); c;}
