#ifndef FDICT_H
#define FDICT_H

#include <inttypes.h>
#include "common.h"

/* Unused arguments generate annoying warnings... */
#define NOTUSED(v) ((void) v)

typedef struct dictNode{
    void * key;
    
    union{
        void * val;
        uint64_t u64;
        int64_t s64;
    }value;
    
    struct dictNode * next;
}dictNode;

typedef struct dictHeader{
    dictNode ** table;
    size_t size;
    size_t size_mask;
    size_t used;
}dictHeader;

typedef struct dictIter{
	dictNode * current;
	size_t no; //current node's rank
	size_t iter_index;
}dictIter;

typedef struct dictType{
    unsigned int (* HashFunc)(void * key);
    int    (* CmpKeys)(void * key1, void * key2);
    void * (* DupKey)(void * key);
    void * (* DupVal)(void * value);
    void   (* DesKey)(void * key);
    void   (* DesVal)(void * value);
}dictType;

typedef struct fdict{
    dictHeader header[2];
    int rehash_index;//if rehash needed
    int iter_num; //iterators' number
    dictType * type;
}fdict;
/****** error code *****/
#define FDICT_FAILD -1 //memory faild
#define FDICT_NONE 0  //no result
#define FDICT_OK 1
#define FDICT_EXIST 2 //already exist

/******************** constants **************************/
#define FDICT_HEADER_INITIAL_SIZE 4
/************************macros***************************/
/* if DupKey is not null,
** then means string should be duplicated,
** either the string just linked */
#define fdictSetKey(d, node, key)				\
    if((d)->type->DupKey){						\
        (node)->key = (d)->type->DupKey(key);	\
    }else{										\
        (node)->key = key;						\
    }
/* if Deskey is not null,
** means string had been duplicated,
** then memeroy should be freed,
** either set the pointer NULL */
#define fdictFreeKey(d, node)					\
    if((d)->type->DesKey){						\
        (d)->type->DesKey((node)->key);			\
    }else{										\
        (node)->key = NULL;						\
    }
/* value has three situations, follow one then is in condition that value is 
** string, its mechanism as same as the "fdictSetKey".  */
#define fdictSetVal(d, node, value)				\
    if((d)->type->DupVal){								\
        (node)->value.val = (d)->type->DupVal(value);	\
    }else{												\
        (node)->value.val = value;				\
    }
#define fdictSetIntVal(d, node, value) do{		\
    (node)->value.s64 = value; } while(0)
#define fdictSetUIntVal(d, node, value) do {	\
    (node)->value.u64 = value; } while(0)
#define fdictFreeVal(d, node)					\
    if((d)->type->DesVal){						\
        (d)->type->DesVal((node)->value.val);	\
    }else{										\
        (node)->value.val = NULL;				\
    }
    
#define fdictCmpKeys(d, key1, key2)				\
    ((d)->type->CmpKeys(key1, key2))
#define fdictHash(d, key)					\
    ((d)->type->HashFunc(key))
#define fdictIsRehash(d)						\
    ((d)->rehash_index != -1)

/******************** API ****************************/
void fdictSetHashSeed(const int seed);
int fdictGetHashSeed();
void fdictSetRandSeed(const int seed);
int fdictGetRandSeed();
fdict * fdictCreate();
void fdictEmpty(fdict *);
void fdictFree(fdict *);
int fdictAddRaw(fdict * dict, const unsigned int hash,
				void *, void *);
int fdictAdd(fdict * dict, void * key, void * value);
void fdictRemove(fdict * dict, void * key);
dictNode * fdictPop(fdict * dict, void * key);
int fdictSet(fdict * dict, void * key, void * value);
int fdictReplace(fdict * dict, void * key, void * value);
dictNode * fdictSearch(fdict * dict, void * value);
dictNode * fdictGetAt(fdict * dict, const size_t index);
dictNode * fdictGetRandom(fdict * dict);
dictIter * fdictIterCreate(fdict * dict, char * key);
dictIter * fdictIterNext(fdict * dict, dictIter * iter);
void fdictIterCancel(fdict * dict, dictIter * iter);
void fdictInfo(fdict * dict);

/******************* hash type ************************/
extern dictType dictTypeDupKeyVal;
extern dictType dictTypeDupKey;
extern dictType dictTypeLnkKeyVal;
extern dictType dictTypeCmdTable;
extern dictType dictTypeSvrTable;

#endif
