#ifndef FQUEUE_H
#define FQUEUE_H


#include <inttypes.h>

typdef struct fqueue{
	size_t len;
	size_t head;
	size_t tail;
	char * buf;
}fqueue;



#endif
