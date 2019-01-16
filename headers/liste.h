#include<stdio.h>
#include<string.h>
#include<stdlib.h>


typedef struct node{
    int fd;
    char *word;
    struct node *next;
} node_t;

typedef struct list{
    node_t *list;
    int numb_elems;
} list_t;


list_t *createList();                         // crea una lista vuota

void destroyList(list_t *L);               // libera tutta la memoria allocata dalla lista

list_t *insertListHead(list_t *L, const char *str, int fd);  // inserisce una stringa nella lista in testa

node_t *listFind(list_t *L, const char *str);    //cerca la stringa str nella lista (ritorna str se presente, NULL altrimenti)

list_t *deleteNameFromList(list_t *L, const char *str); //cancella la stringa dalla lista

list_t *deleteFdFromList(list_t *L, int fd); //cancella fd dalla stringa

char *toBuf(list_t *L);       //concatena le stringhe della lista in un'unica stringa dalla stringa in testa a quella in coda
