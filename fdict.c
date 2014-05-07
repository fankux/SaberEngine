#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>
#include "fdict.h"

static int fdict_hash_seed = 5381;
static int fdict_rand_seed = 5731;

//private function
static void _ResetHeader(dictHeader * );
static void _ReleaseHeader(dictHeader *);
static dictNode * _Find(fdict *, void *, unsigned int *);
static int _Expand(fdict * ,const size_t);
static void _RehashStep(fdict *, const unsigned int);
unsigned int _IntHash(void * key);
unsigned int _GenHash(void *buf);
unsigned int _GenCaseHash(void *buf);
int _StrCmp(void * str1, void * str2);
int _IntCmp(void * a, void * b);
int _IntCmpKey(void * a, void * b);
void * _StrDup(void * str);
void * _IntDup(void * str);
void _StrFree(void * str);

dictType dictTypeDupKeyVal =
{_GenHash, _StrCmp, _StrDup, _StrDup, _StrFree, _StrFree};  
dictType dictTypeDupKey =
{_GenHash, _StrCmp, _StrDup, NULL, _StrFree, NULL};  
dictType dictTypeLnkKeyVal =
{_GenHash, _StrCmp, NULL, NULL,	NULL, NULL};
dictType dictTypeCmdTable =
{_GenCaseHash, _StrCmp, NULL, NULL, NULL, NULL};
dictType dictTypeSvrTable =
{_IntHash, _IntCmpKey, _IntDup, NULL, NULL, NULL};

static void _ResetHeader(dictHeader * header){
    header->table = NULL;
    header->size = 0;
    header->size_mask = 0;
    header->used = 0;
}
static void _ReleaseHeader(dictHeader * header){
    free(header->table);
    _ResetHeader(header);
}
/*create a fdict */
fdict * fdictCreate(){
    fdict * dict;
    if(!(dict = (fdict *)malloc(sizeof(fdict)))) return NULL;
    
    _ResetHeader(&dict->header[1]);
    _ResetHeader(&dict->header[0]);
    dict->rehash_index = -1;
    dict->iter_num = 0;

	dict->type = &dictTypeLnkKeyVal;
}

void fdictFree(fdict * dict){
	size_t i, j;
	dictNode * p, * q;
	dictHeader * h;
	for(j = 0; j < 2; ++j){
		h = &dict->header[j];
		for(i = 0; i < h->size; ++i){
			p = h->table[i];
			while(p){
				q = p;
				p = p->next;
				fdictFreeKey(dict, q);
				fdictFreeVal(dict, q);
			}
		}
		free(h->table);
	}
	free(dict);
}

/***************Hash***********************************************************/
void fdictSetHashSeed(const int seed){
    fdict_hash_seed = seed;
}
int fdictGetHashSeed(void){
    return fdict_hash_seed;
}
void fdictSetRandSeed(const int seed){
	fdict_rand_seed = seed;
}
int fdictGetRandSeed(void){
	return fdict_rand_seed;
}

/* Thomas Wang's 32 bit Mix Function */
unsigned int _IntHash(void * k)
{
	unsigned int key = *(int *)k;
	
    key += ~(key << 15);
    key ^=  (key >> 10);
    key +=  (key << 3);
    key ^=  (key >> 6);
    key += ~(key << 11);
    key ^=  (key >> 16);
    return key;
}

/* djb hash */
unsigned int _GenHash(void * buf) {
    unsigned int hash = (unsigned int)fdict_hash_seed;
	unsigned char * p = (unsigned char *)buf;
    
    while (*p)
	    hash = ((hash << 5) + hash) + *p++; /* hash = hash * 33 + c */

	return hash;
}
/* case insensitive hash function (based on djb hash) */
unsigned int _GenCaseHash(void * buf) {
    unsigned int hash = (unsigned int)fdict_hash_seed;
	unsigned char * p = (unsigned char *)buf;
    
    while (*p)
		hash = ((hash << 5) + hash) + (tolower(*p++)); /* hash * 33 + c */
	return hash;
}

/************* API Implementations ********************************************/
static void _RehashStep(fdict * dict, const unsigned int step){
    if(!fdictIsRehash(dict) || step <= 0) return;
    
    dictHeader * org = &dict->header[0];
    dictHeader * new = &dict->header[1];
    dictNode * p;
    size_t index;
    int i = 0; 
    while(i < step){
        p = org->table[dict->rehash_index];
        while(p && i < step){
            org->table[dict->rehash_index] = p->next;
            index = new->size_mask & fdictHash(dict, p->key);
            p->next = new->table[index];
            new->table[index] = p;
            p = org->table[dict->rehash_index];
            
            ++new->used;
            ++i;
        }//rehash complete
        if(org->table[dict->rehash_index]) continue;
        
        do{
            if(++dict->rehash_index >= org->size){
                dict->rehash_index = -1;
                
                _ReleaseHeader(&dict->header[0]);
                dict->header[0] = dict->header[1];
                _ResetHeader(&dict->header[1]);
                //h0 <= h1, release h1
                
                return;
            }  
        }while(!org->table[dict->rehash_index]);
    }
} 
static int _Expand(fdict * dict, const size_t len){
    //is rehash, expand action denied
    if(fdictIsRehash(dict)) return 2;
    
    dictHeader * h = &dict->header[1];
    dictNode ** new = NULL;
    if(!(new = (dictNode **)malloc(len * sizeof(dictNode *)))) return 0;
    memset(new, 0, len * sizeof(dictNode *));
    h->table = new;
    h->size = len;
    h->size_mask = len - 1;
    h->used = 0;
    
    dict->rehash_index = 0;
    //_RehashStep(dict);
}

/* find the key, if not exist, then the "index" stores the key index */
static dictNode * _Find(fdict * dict, void * key, unsigned int * hash){
    dictHeader * h = &dict->header[0];
	dictNode * p;
	unsigned int result;
	
    /* fisrt memeroy allocation */
    if(h->size <= 0){
        if(!(h->table = (dictNode **)malloc(FDICT_HEADER_INITIAL_SIZE *
                        sizeof(dictNode *))))
            return 0;
        memset(h->table, 0,
			   FDICT_HEADER_INITIAL_SIZE * sizeof(dictNode *));
        h->size = FDICT_HEADER_INITIAL_SIZE;
        h->size_mask = h->size - 1;
    }
	
    result = fdictHash(dict, key);
    if(hash) *hash = result;

	do {
		p = h->table[(unsigned int)(h->size_mask & result)];
		while(p){
			if(0 == fdictCmpKeys(dict, p->key, key))
				return p;
			p = p->next;
		}
		if(fdictIsRehash(dict)){
			h = &dict->header[1];
			continue;
		}
	}while(0);
	
    return NULL;
}

/* Add a key-value node at index of 'hash to index'. 
** Be sure that key's hash must be equal to p->key's */
int fdictAddRaw(fdict * dict , const unsigned int hash,
				void * key, void * value){
    dictHeader * h;
    if(fdictIsRehash(dict)){
        h = &dict->header[1];
        //_RehashStep(dict);
    }else h = &dict->header[0];
    
    /* fisrt memeroy allocation */
    if(h->size <= 0){
        if(!(h->table = malloc(FDICT_HEADER_INITIAL_SIZE *
							   sizeof(dictNode *))))
            return FDICT_FAILD;
		memset(h->table, 0,
			   FDICT_HEADER_INITIAL_SIZE * sizeof(dictNode *));
        h->size = FDICT_HEADER_INITIAL_SIZE;
        h->size_mask = h->size - 1;
    }
    
    unsigned int index = hash & h->size_mask;
    dictNode * new;
    if(!(new = malloc(sizeof(dictNode))))
		return FDICT_FAILD;/* allocate one */
	
	fdictSetKey(dict, new, key);
	if(dict->type->DupKey != _IntDup
	   && !new->key){/* key never be null if key is string */
        free(new);
        return FDICT_FAILD;
    }
	
    fdictSetVal(dict, new, value);
    new->next = NULL;

    new->next = h->table[index];
    h->table[index] = new;
    
    ++h->used;
    
    return FDICT_OK;
}
int fdictAdd(fdict * dict, void * key, void * value){  
    unsigned int hash;
    dictNode * p = _Find(dict, key, &hash);
	if(p) return FDICT_EXIST;//this key already exist, add faild

    return fdictAddRaw(dict, hash, key, value);
}
inline dictNode * fdictSearch(fdict * dict, void * key){
	return _Find(dict, key, NULL);
}
int fdictReplace(fdict * dict, void * key, void * value){
    dictNode * p = NULL;
    p = _Find(dict, key, NULL);
    
    if(!p) return 0;
    
    fdictFreeVal(dict, p);
    fdictSetVal(dict, p, value);
    
    return 1;
}

int fdictSet(fdict * dict, void * key, void * value){
    unsigned int hash;
    dictNode * p = _Find(dict, key, &hash);
    
    //not exist,then add it to dict
    if(!p) return fdictAddRaw(dict, hash, key, value);
    fdictFreeVal(dict, p);
    fdictSetVal(dict, p, value);
    
    return FDICT_OK;
}

void fdictRemove(fdict * dict, void * key){
    dictNode * p;
    if(!(p = fdictPop(dict, key))) return;
    
    fdictFreeKey(dict, p);
    fdictFreeVal(dict, p);

    return;
}

/* pop a node, the memory should be controlled  by caller manually */
dictNode * fdictPop(fdict * dict, void * key){
    dictHeader * h = NULL;
    if(fdictIsRehash(dict)) h = &dict->header[1];
    else h = &dict->header[0];
    
    unsigned int index = h->size_mask & fdictHash(dict, key);
    dictNode * p = h->table[index];
    dictNode * q = p;
    while(p){
        if(!fdictCmpKeys(dict, p->key, key)) break;
        
        q = p;
        p = p->next;
    }
    
    //not exist
    if(!p) return NULL;
    
    if(q != p) q->next = p->next;
    else h->table[index] = p->next;
    
    --h->used;
      
    return p;
}

/* get node at the position of "index(0 base)" */
dictNode * fdictGetAt(fdict * dict, const size_t index){
	dictIter * iter;
	size_t real_size = dict->header[0].used;
	size_t i;

	if(fdictIsRehash(dict)) real_size += dict->header[1].used;
	
	if(index >= real_size) return NULL;

	iter = fdictIterCreate(dict, NULL);
	for(i = 0; i < index; ++i)
		iter = fdictIterNext(dict, iter);

	return iter->current;
}

dictNode * fdictGetRandom(fdict * dict){
	srand(fdict_rand_seed);
	size_t real_size = dict->header[0].used;
	if(fdictIsRehash(dict)) real_size += dict->header[1].used;

	return fdictGetAt(dict, rand() % real_size);
}

/***** iterator implement ******/
dictIter * fdictIterCreate(fdict * dict, char * key){
	size_t i;
	dictIter * iter;
	dictHeader * h = &dict->header[0];

	if(h->used <= 0) return NULL;
	if(NULL == (iter = (dictIter *)malloc(sizeof(dictIter)))) return NULL;
	
	iter->no = 0;
	for(i = 0; i < h->size; ){
		if(!h->table[i]){
			++i;
		}else{
			iter->current = h->table[i];
			if(key){
				while(iter->current){
					if(dict->type->CmpKeys((void *)key,
										   (void *)iter->current->key) == 0){
						__sync_add_and_fetch(&dict->iter_num, 1);
						iter->iter_index = i;
						return iter;
					}
					iter->current = iter->current->next;
					++iter->no;
				}
				++i;
			}else{
				__sync_add_and_fetch(&dict->iter_num, 1);
				iter->iter_index = i;
				return iter;
			}
		}
	}
	return NULL;
}

dictIter * fdictIterNext(fdict * dict, dictIter * iter){
	size_t i;
	dictHeader * h = &dict->header[0];
	int flag = 0;
	if(!iter) return NULL;
	
	for(i = iter->iter_index; i < h->size; ){
		if(!h->table[i]){
			++i;
		}else{
			if(flag == 1){
				iter->current = h->table[i];
				iter->iter_index = i;
				++iter->no;
				return iter;
			}
			if(iter->current->next){
				iter->current = iter->current->next;
				++iter->no;
				return iter;
			}else{
				++i;
				flag = 1;
			}
		}
	}
	/* iteration complete */
	fdictIterCancel(dict, iter);
	return NULL;
}

void fdictIterCancel(fdict * dict, dictIter * iter){
	__sync_sub_and_fetch(&dict->iter_num, 1);
	free(iter);
}

/****************Type Functions ***********************************************/
int _IntCmpKey(void * key, void * value){
	int a = (int)key;
	int b = *(int *)value;
	return a > b? 1: (a < b? -1: 0);
}
int _IntCmp(void * av, void * bv){
	int a = *(int *)av;
	int b = *(int *)bv;
	return a > b? 1: (a < b? -1: 0);
}
int _StrCmp(void * str1, void * str2){
    return strcmp((char *)str1, (char *)str2);
}

void * _IntDup(void * i){
	return (void *)(*(int *)i);
}
void * _StrDup(void * str){
    int len = strlen(str);
    char * p = NULL;
    if(!(p = (char *)malloc(sizeof(len) + 1))) return NULL;
    memcpy(p, str, len);
    p[len] = '\0';

    return p;
}
void _StrFree(void * str){
    free(str);
}

/****************Test use *****************************************************/
void fdictInfo(fdict * dict){
    printf("rehash_index:%d; iter_num:%d\n", dict->rehash_index,
		   dict->iter_num);
    dictHeader * h;
    for(int j = 0; j < 2; ++j){
        h = &dict->header[j];
        printf("h%d: size:%d; used:%d\n", j, h->size, h->used);
        
        for(int i = 0; i < h->size; ++i){
            dictNode * p = h->table[i];
            printf("%4d(p:%10d):", i, (int)p);
            while(p){
                printf("%s:%d->", (char *)p->key, (int)&p->value);
                p = p->next;
            }
            printf("\n");
        }
    }
}

#ifndef ALL_DEBUG
#define FDICT_DEBUG
/* void * thread_func(void * arg); */
/* int main(){ */    
/*     fdict * dict = fdictCreate(); */
/*     dict->type = &typeLnkKeyVal; */
    
/*     //srand(time(NULL)); */
/*     fdictSetHashSeed(rand()); */
/*     fdictAdd(dict, "fdf", "collapse"); */
/*     fdictAdd(dict, "fda78", "213213"); */
/*     fdictAdd(dict, "fdha72", "futali"); */
/*     fdictSet(dict, "I*#", "fdafdaf"); */
/*     fdictAdd(dict, "4", "jodandesu"); */
/*     fdictInfo(dict); */
/*     //_Expand(dict, 8); */
/*     //_RehashStep(dict, 10); */
/*     //printf("replace:%d\n",fdictReplace(dict, "22222222", "fdafda")); */
/*     //printf("replace:%d\n",fdictReplace(dict, "33333", "333333")); */
/*     //fdictInfo(dict); */
    
/*     //fdictRemove(dict, "22222222"); */
/*     //fdictInfo(dict); */
    
/*     //fdictInfo(dict); */
	
/* 	pthread_t tid; */
	
/* 	//if((pthread_create(&tid, NULL, thread_func, (void *)dict))){ */
/* 	//	printf("thread error\n"); */
/* 	//} */

/* 	dictIter * iter = fdictIterCreate(dict, NULL); */

	
/* 	printf("%ul:%s-%s\n", iter->no, (char *)iter->current->key, */
/* 		   (char *)iter->current->value.val); */
/* 	while((iter = fdictIterNext(dict, iter))){ */
/* 		printf("%ul:%s-%s\n", iter->no, (char *)iter->current->key, */
/* 		   (char *)iter->current->value.val); */
/* 		//break; */
/* 	} */

/* 	for(int i = 0; i < 4; ++i){ */
/* 		fdictSetRandSeed(time(NULL)); */
/* 		sleep(1); */
/* 		dictNode * s = fdictGetRandom(dict); */
/* 		if(s){ */
/* 			printf("%s-%s\n", (char *)s->key, (char *)s->value.val); */
/* 		} */
/* 	} */
	
/*     return 0; */
/* } */
/* void * thread_func(void * arg){ */
/* 	fdict * dict = (fdict *)arg; */
/* 	dictIter * iter = fdictIterCreate(dict, NULL); */

	
/* 	printf("%ul:%s-%s\n", iter->no, (char *)iter->current->key, */
/* 		   (char *)iter->current->value.val); */
/* 	while((iter = fdictIterNext(dict, iter))){ */
/* 		printf("%ul:%s-%s\n", iter->no, (char *)iter->current->key, */
/* 		   (char *)iter->current->value.val); */
/* 		//break; */
/* 	} */

	
	
/* 	return (void *)0; */
/* } */
#endif
