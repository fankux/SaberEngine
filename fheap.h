#ifndef FHEAP_H
#define FHEAP_H

#include <inttypes.h>
#include <sys/types.h>

/* error codes */
#define FHEAP_FAILD -1
#define FHEAP_NONE 0
#define FHEAP_OK 1
#define FHEAP_SUCCESS 1
#define FHEAP_EXIST 2

/**************** heap type ****************/
#define FHEAP_INITIAL_SIZE 4
#define FHEAP_MIN 0
#define FHEAP_MAX 1

typedef struct fheapNode{
	ssize_t key;
	void * value;	
}fheapNode;

typedef struct fheap{
	size_t size; /* heap size */
	size_t free; /* remain space */
	fheapNode * start; /* data array */
    /* due to the differences of compartion between
	** max heap and min heap  */
	int (* keyCmpFunc)(const ssize_t a, const ssize_t b);
    /* if value copying needed,this necessary */
	void * ( * valDupFunc)(void *  value);
	void ( * valFreeFunc)(void * value);
	
	uint8_t type;
}fheap;

/**************** macros ******************/
#define fheapSetVal(h, p, value) \
	(h->valDupFunc != NULL? (p->value = h->valDupFunc(value)):\
	 (p->value = value))
#define fheapCmpKey(h, a, b) (h->keyCmpFunc(a, b))


fheap * fheapCreate(const uint8_t type);
fheapNode * fheapGet(fheap * h, ssize_t key, size_t * pos);
int fheapExist(fheap * h, ssize_t key);
int fheapSet(fheap * h, ssize_t key);
int fheapReplace(fheap * h, ssize_t key);
int fheapAdd(fheap * h, ssize_t key, void * value);
int fheapRemoveAt(fheap * h, size_t pos);
int fheapRemove(fheap * h, ssize_t key);
int fheapSort(fheap * h);

#endif
