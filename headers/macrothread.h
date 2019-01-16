#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
int err;

#define THREAD(c,m)\
if ((err = (c)) != 0){errno = err;perror(m);exit(EXIT_FAILURE);}
