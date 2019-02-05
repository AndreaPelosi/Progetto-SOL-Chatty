/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file config.h
 * @brief File contenente alcune define con valori massimi utilizzabili
 *
 * @author Andrea Pelosi 547596
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 *
 */

#if !defined(CONFIG_H_)
#define CONFIG_H_

#define MAX_NAME_LENGTH                  32 //lunghezza massima del nome di un client
#if !defined(UNIX_PATH_MAX)
#define UNIX_PATH_MAX                    108
#endif
/* aggiungere altre define qui */



// to avoid warnings like "ISO C forbids an empty translation unit"
typedef int make_iso_compilers_happy;

#endif /* CONFIG_H_ */
