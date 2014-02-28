#ifndef SABER_H
#define SABER_H

#include <inttypes.h>
#include "fstr.h"
#include "fdList.h"
#include "fdict.h"
#include "fintset.h"
#include "fbintree.h"

/* data type */
#define SABER_STRING 0
#define SABER_LIST 1
#define SABER_SORT 2
#define SABER_HASH 3

/* real data struct */
#define SABER_ENCODE_STRING 0
#define SABER_ENCODE_INTEGER 1
#define SABER_ENCODE_DLIST 2
#define SABER_ENCODE_BINTREE 3
#define SABER_ENCODE_INTSET 4
#define SABER_ENCODE_HASH 5

/* error code */
#define TYPE_ERROR -2
#define FAILD -1
#define NONE 0
#define SUCCESS 1
#define OK 1
#define EXIST 2


typedef struct saberObject{
	uint8_t type;
	uint8_t encode;
	void * value;
}sobj;

typedef struct saberTable{
	int id;
	fdict * table;
	fdict * expire;
	fdict * watch;
}stab;

/****************************** API **************************************/
sobj * createObj(uint8_t type, void * value);
sobj * createStringObj(char * str, size_t len);
sobj * createStringObjInt(const int64_t value);
sobj * createListObj(void);
sobj * createSortObj(void);
sobj * createSortObjBinTree(void);
sobj * createHashObj(void);
void freeObj(sobj * o);
/* list type object */
int listObjAdd(sobj * o, sobj * new);
/* sort type object */
int sortObjAdd(sobj * o, sobj * new);
/* hash  type object */
int hashObjAdd(sobj * o, sobj * new);
int hashObjRemove(sobj * o, const char * key);
int hashObjGet(sobj * o, const char * key, void * value);
int hashObjSet(sobj * o, const char * key, const void * vlaue);
int hashObjExist(sobj * o, const char * key);
/* saber table(table.c) */
int stabAdd(stab * t, sobj * key, sobj * value);
int stabRemove(stab * t, sobj * key);
int stabSet(stab * t, sobj * key);
int stabGet(stab * t, sobj * key);
int stabReplace(stab * t, sobj * key);
int stabExist(stab * t, sobj * key);
/* saber client */

/* saber engine */

#endif /* saber.h */
