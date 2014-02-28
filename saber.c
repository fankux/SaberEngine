#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "saber.h"

/* private functions */


/* API Implements */
sobj * createObj(uint8_t type, void * value){
	sobj * o;
	if(NULL == (o = (sobj *)malloc(sizeof(sobj)))) return NULL;

	o->type = type;
	o->encode = SABER_ENCODE_STRING;
	o->value = value;

	return o;
}

sobj * createStringObj(char * str, size_t len){
	return createObj(SABER_STRING, (void *)fstrCreateLen(str, len));
}

sobj * createStringObjInt(const int64_t value){
	sobj * o;
	if(NULL == (o = (sobj *)malloc(sizeof(sobj)))) return NULL;
	
	if(value >= INT32_MIN && value <= INT32_MAX){
		o->type = SABER_STRING;
		o->encode = SABER_ENCODE_INTEGER;

		//if value less then 4 bytes, just cast to a pointer value
		o->value = (void *)((int32_t)value);
	}else{
		o = createObj(SABER_STRING, (void *)fstrCreateInt(value));
	}

	return o;
}

sobj * createListObj(void){
	sobj * o = createObj(SABER_LIST, (void *)fdListCreate());
	o->encode = SABER_ENCODE_DLIST;

	return o;
}

sobj * createSortObj(void){
	sobj * o = createObj(SABER_SORT, (void *)fintsetCreate());
	/* use intset first,
	   if it's size increment to a size
	   which large than 512,
	   change it to bintree */
	o->encode = SABER_ENCODE_INTSET;

	return o;
}

sobj * createSortObjBinTree(void){
	sobj * o = createObj(SABER_SORT, (void *)fbintreeCreate());
	o->encode = SABER_ENCODE_BINTREE;

	return o;
}

void freeObj(sobj * o){
	switch(o->encode){
	case SABER_ENCODE_STRING :
		fstrFree((fstr *)o->value);
		free(o);
		break;
	case SABER_ENCODE_INTEGER :
		free(o);
		break;
	case SABER_ENCODE_DLIST :
		fdListFree((fdList *)o->value);
		free(o);
		break;
	case SABER_ENCODE_BINTREE :
		fbintreeFree((fbintree *)o->value);
		free(o);
		break;
	case SABER_ENCODE_INTSET :
		fintsetFree((fintset *)o->value);
		free(o);
		break;
	case SABER_ENCODE_HASH :
		fdictFree((fdict *)o->value);
		free(o);
		break;
	default:break;
	}
}

sobj * createHashObj(void){
	sobj * o = createObj(SABER_HASH, (void *)fdictCreate());
	o->encode = SABER_ENCODE_HASH;

	return o;
}

/* hash type object */
int hashObjAdd(sobj * o, sobj * new){
	if(o->encode != SABER_HASH ||
	   o->encode != SABER_HASH) return TYPE_ERROR;
	
	return fdictAdd((fdict *)o->value, ((fdict *)new->value)->key,
			 &((fdict *)new->value)->value);
}



static void sobjInfo(sobj * o){
	printf("\ntype:%d;encode:%d\n", o->type, o->encode);
}

int main(void){

	return 0;
}
