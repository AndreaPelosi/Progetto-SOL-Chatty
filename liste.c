#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <config.h>
#include <liste.h>


list_t *createList(){

    list_t *hd = (list_t *)malloc(sizeof(list_t));
    hd->numb_elems = 0;
    hd->list = NULL;

    return hd;
}



list_t *insertListHead(list_t *L, const char *str, int fd){

    node_t *curr = (node_t *)malloc(sizeof(node_t));
    curr->word = (char *)malloc((MAX_NAME_LENGTH + 1) * sizeof(char));
    int len = strlen(str);

    strncpy(curr->word, str, len);
    for (size_t i = len; i < MAX_NAME_LENGTH + 1; i++) {
        (curr->word)[i] = ' ';
    }

    curr->fd = fd;

    curr->next = L->list;
    L->list = curr;
    L->numb_elems ++;

    return L;
}


#if 0
list_t *insertListHead(list_t *L, const char *str, int fd){

    node_t *curr = (node_t *)malloc(sizeof(node_t));
    int x = strlen(str);

    curr->word = (char *)malloc(x * sizeof(char));
    strncpy(curr->word, str, x);
    curr->fd = fd;

    curr->next = L->list;
    L->list = curr;
    L->numb_elems ++;

    return L;
}
#endif

node_t *listFind(list_t *L, const char *str){

    node_t *curr;
    char *cpy; //variabile che prende ogni utente della lista senza considerare gli spazi
    int len = strlen(str);

    for (curr = L->list; curr != NULL; curr = curr->next) {

        cpy = (char *)malloc(len * sizeof(char));
        strncpy(cpy, curr->word, len);
        if (0 == strcmp(cpy, str)){
            return curr;
        }
        free(cpy);
    }
    return NULL;
}


list_t *deleteNameFromList(list_t *L, const char *str){

    int trovato = 0;
    node_t *curr, *prec;
    curr = L->list;

    //se la stringa da cancellare e' nel primo nodo della lista
    if (strcmp(curr->word, str) == 0){
        L->numb_elems--;
        L->list = curr->next;
        return L;
    }

    while ((NULL != curr) && (0 == trovato)) {
        prec = curr;
        curr = curr->next;
        if (NULL == curr){
            return L;
        }
        if (strcmp(curr->word, str) == 0){
            prec->next = curr->next;
            free(curr->word);
            free(curr);
            curr = NULL;
            L->numb_elems--;
        }
    }

    return L;
}



list_t *deleteFdFromList(list_t *L, int fd){

    int trovato = 0;
    node_t *curr, *prec;
    curr = L->list;

    //se il fd da cancellare e' nel primo nodo della lista
    if (curr->fd == fd){
        L->numb_elems--;
        L->list = curr->next;
        return L;
    }

    while ((NULL != curr) && (0 == trovato)) {
        prec = curr;
        curr = curr->next;
        if (NULL == curr){
            return L;
        }
        if (curr->fd == fd){
            prec->next = curr->next;
            free(curr->word);
            free(curr);
            curr = NULL;
            L->numb_elems--;
        }
    }

    return L;

}



void destroyList(list_t *L){

    node_t *curr, *prec;

    curr = L->list;

    while (NULL != curr) {
        prec = curr;
        curr = curr->next;
        free(prec->word);
        free(prec);
    }

    free(L);
}



char *toBuf(list_t *L) {
    char *buffer = calloc((L->numb_elems * (MAX_NAME_LENGTH + 1)), sizeof(char));
	char *aux = buffer;
    node_t *curr = L->list;

	for (size_t i = 0; i < L->numb_elems; i++) {
			strcat(aux, curr->word);
			aux += MAX_NAME_LENGTH+1;
            curr = curr->next;
	}

    return buffer;
}



#if 0

char *toBuf(list_t *L){

    int len;

//    char *buffer = (char *)malloc((L->numb_elems) * (MAX_NAME_LENGTH + 1) * sizeof(char));
    char *buffer = calloc((L->numb_elems * (MAX_NAME_LENGTH + 1)), sizeof(char));
    node_t *curr = L->list;

//    memset(buffer, '\0', strlen(buffer));

    for (int i=0,p=0;i< L->numb_elems; ++i, p+=(MAX_NAME_LENGTH+1)) {

        len = strlen(curr->word);

        char *space_string = (char *)malloc((MAX_NAME_LENGTH + 1 - len) * sizeof(char));

        for (size_t j = 0; j < (MAX_NAME_LENGTH - len); j++) {
            space_string[j] = ' ';
        }

        space_string[MAX_NAME_LENGTH - len] = '\n';

        int len1 = strlen(space_string);

        strncat(&buffer[p], curr->word, len);
        strncat(&buffer[p], space_string, len1 + 1);
        curr = curr->next;
	}

    return buffer;
}
#endif
