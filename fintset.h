#ifndef __FINTSET_H
#define __FINTSET_H

#include <stdint.h>

/* error code */
#define FINTSET_ERROR -2 //unrecoverbal error
#define FINTSET_FAILD -1 //action faild, usually cause by memory allocation faild
#define FINTSET_NONE 0   //no result when search
#define FINTSET_SUCCESS 1
#define FINTSET_EXIST 2  //add or insert action, the node already existed


typedef struct fintset{
    unsigned int encode;
    size_t len;
    int8_t sets[];
}fintset;


fintset * fintsetCreate(void);
void fintsetFree(fintset * is);
int fintsetAdd(fintset *, const int64_t );
uint8_t fintsetSearch(fintset *, const int64_t, size_t *);
uint8_t fintsetRemove(fintset *, const int64_t);
uint8_t fintsetGet(fintset *, const size_t pos, int64_t *);
#endif
