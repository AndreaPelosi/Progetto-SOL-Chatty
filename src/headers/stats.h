/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
 /**
 * @file stats.h
 * @brief contiene strutture e funzioni utili per la gestione delle statistiche di Chatty
 *
 * @author Andrea Pelosi 547596
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */
#if !defined(MEMBOX_STATS_)
#define MEMBOX_STATS_

#include <stdio.h>
#include <time.h>

/**
    @struct statistics
    @brief raccoglie le statistiche del server

    @param nusers
    @param nonline
    @param ndelivered
    @param nnotdelivered
    @param nfiledelivered
    @param nfilenotdelivered
    @param nerrors
*/
struct statistics {
    unsigned long nusers;                       // n. di utenti registrati
    unsigned long nonline;                      // n. di utenti connessi
    unsigned long ndelivered;                   // n. di messaggi testuali consegnati
    unsigned long nnotdelivered;                // n. di messaggi testuali non ancora consegnati
    unsigned long nfiledelivered;               // n. di file consegnati
    unsigned long nfilenotdelivered;            // n. di file non ancora consegnati
    unsigned long nerrors;                      // n. di messaggi di errore
};

/* aggiungere qui altre funzioni di utilita' per le statistiche */

typedef enum {

    NUSERS = 0,
    NONLINE = 1,
    NDELIVERED = 2,
    NNOTDELIVERED = 3,
    NFILEDELIVERED = 4,
    NFILENOTDELIVERED = 5,
    NERRORS = 6
} stats;

/**
 * @function printStats
 * @brief Stampa le statistiche nel file passato come argomento
 *
 * @param fout descrittore del file aperto in append.
 *
 * @return 0 in caso di successo, -1 in caso di fallimento
 */
static inline int printStats(FILE *fout) {
    extern struct statistics chattyStats;

    if (fprintf(fout, "%ld - %ld %ld %ld %ld %ld %ld %ld\n",
		(unsigned long)time(NULL),
		chattyStats.nusers,
		chattyStats.nonline,
		chattyStats.ndelivered,
		chattyStats.nnotdelivered,
		chattyStats.nfiledelivered,
		chattyStats.nfilenotdelivered,
		chattyStats.nerrors
		) < 0) return -1;
    fflush(fout);
    return 0;
}

#endif /* MEMBOX_STATS_ */
