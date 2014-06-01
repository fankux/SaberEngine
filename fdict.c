#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>
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

/****************Type Functions *****************/
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
int _StrCaseCmp(void * str1, void * str2){
	return strcasecmp((char *)str1, (char *)str2);
}
void _IntSet(dictNode * node, void * i){
	node->value.s64 = *(int64_t *)i;
}
void _FloatSet(dictNode * node, void * f){
	node->value.f64 = *(double *)f;
}
void _StrSet(dictNode * node, void * str){
	int len = strlen((char*)str);
	if(!(node->value.val = malloc(sizeof(len) + 1))) return;
	cpystr((char*)node->value.val, (char*)str, len);
}
void * _StrDup(void * str){
    int len = strlen((char*)str);
    char * p = NULL;
    if(!(p = malloc(sizeof(len) + 1))) return NULL;
    cpystr((char*)p, (char*)str, len);
    
    return p;
}
void * _IntDup(void * str){
	return (void *)(*(int *)str);
}
void _StrFree(void * str){
    free(str);
}
dictType dictTypeDupKeyVal =
{_GenHash, _StrCmp, _StrDup, _StrSet, _StrFree, _StrFree};  
dictType dictTypeDupKey =
{_GenHash, _StrCmp, _StrDup, NULL, _StrFree, NULL};  
dictType dictTypeLnkKeyVal =
{_GenHash, _StrCmp, NULL, NULL,	NULL, NULL};
dictType dictTypeCmdTable =
{_GenCaseHash, _StrCaseCmp, NULL, NULL, NULL, NULL};
/* key is ip(unsigned int) cast to int, value is pointer */
dictType dictTypeSvrTable =
{_IntHash, _IntCmpKey, _IntDup, NULL, NULL, NULL};
/* key is string */
dictType dictTypeStabTable = 
{_GenHash, _StrCmp, NULL, NULL, NULL, NULL};

/*************** Hash *********************/
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
	unsigned int key = (int)k;
	
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
    
    while (*p) /* hash = hash * 33 + c */
	    hash = ((hash << 5) + hash) + *p++; 

	return hash;
}
/* case insensitive hash function (based on djb hash) */
unsigned int _GenCaseHash(void * buf) {
    unsigned int hash = (unsigned int)fdict_hash_seed;
	unsigned char * p = (unsigned char *)buf;
    
    while (*p) /* hash * 33 + c */
		hash = ((hash << 5) + hash) + (tolower(*p++)); 
	return hash;
}

/****************** API Implementations ********************/
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
    if(!(dict = malloc(sizeof(fdict)))) return NULL;
    
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

static void _RehashStep(fdict * dict, const unsigned int step){
	dictHeader * org = &dict->header[0];
    dictHeader * new = &dict->header[1];
    dictNode * p;
    size_t index;
    int i = 0;

	if(!fdictIsRehash(dict) || step == 0) return;
    
    while(i < step){
        p = org->table[dict->rehash_index];
        while(p && i < step){
            org->table[dict->rehash_index] = p->next;
            index = new->size_mask & fdictHash(dict, p->key);
            p->next = new->table[index];
            new->table[index] = p;
            p = org->table[dict->rehash_index];

			--org->used;
            ++new->used;
            ++i;
        }
        if(org->table[dict->rehash_index]) continue;
        
        do{
			/* rehash complete */
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
    if(!(new = malloc(len * sizeof(dictNode *)))) return 0;
    memset(new, 0, len * sizeof(dictNode *));

    h->table = new;
    h->size = len;
    h->size_mask = len - 1;
    h->used = 0;
    
    dict->rehash_index = 0;
    _RehashStep(dict, 1);
}

/* find the key, the "hash" stores the key hash value */
static dictNode * _Find(fdict * dict, void * key,
						unsigned int * hash){
    dictHeader * h = &dict->header[0];
	dictNode * p;
	unsigned int result;
	int flag, hflag = 1;
	
    /* fisrt memeroy allocation */
    if(h->size <= 0){
        if(!(h->table = malloc(FDICT_HEADER_INITIAL_SIZE *
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
		flag = 0;
		if(fdictIsRehash(dict) && hflag){
			h = &dict->header[1];
			flag = 1;
			hflag = 0;
		}
	}while(flag);
	
    return NULL;
}

/* Add a key-value node at index of 'hash to index'. 
** Be sure that key's hash must be equal to p->key's */
int fdictAddRaw(fdict * dict , const unsigned int hash,
				void * key, void * value){
    dictHeader * h;
    if(fdictIsRehash(dict))
		h = &dict->header[1];
    else h = &dict->header[0];
    
    /* fisrt memeroy allocation */
    if(fdictIsEmpty(dict)){
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
	/* key never be null if key is string */
	if(dict->type->DupKey == _StrDup && !new->key){
		free(new);
        return FDICT_FAILD;
    }
	
    fdictSetVal(dict, new, value);

    new->next = h->table[index];
    h->table[index] = new;

	if(fdictIsRehash(dict)) _RehashStep(dict, 1);

	
    if(++h->used >= h->size);
		/* _Expand(dict, h->size << 1); */
    
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

/* change value,if not exist, add new one */
/* !!!!!!!bug!!!!!!! */
int fdictSet(fdict * dict, void * key, void * value){
    unsigned int hash;
    dictNode * p = _Find(dict, key, &hash);
    
    //not exist,then add it to dict
    if(!p) return fdictAddRaw(dict, hash, key, value);
    fdictFreeVal(dict, p);
    fdictSetVal(dict, p, value);
    
    return FDICT_OK;
}

int fdictRemove(fdict * dict, void * key){
    dictNode * p;
    if(!(p = fdictPop(dict, key))) return FDICT_NONE;
    
    fdictFreeKey(dict, p);
    fdictFreeVal(dict, p);

    return FDICT_OK;
}

/* pop a node,
** the memory should be controlled by caller manually */
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

	iter = fdictIterCreate(dict, 0);
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

/**************** iterator implement ******************/
/* pos is index of node where starting iter */
dictIter * fdictIterCreate(fdict * dict, size_t pos){
	size_t i, level = 0;
	dictIter * iter;
	dictHeader * h = &dict->header[0];
	
	if(pos >= h->used){
		level = h->size;
		pos -= h->used;
		h = &dict->header[1];
		if(pos >= h->used)
			return NULL; /* out of range */
	}
	if(!(iter = malloc(sizeof(dictIter)))) return NULL;

	for(i = 0; i < h->size; ++i){
		if(!h->table[i]) continue;
		else break;
	}
	
	iter->no = 0;
	iter->iter_index = i + level;
	iter->current = h->table[i];
	
	/* for(i = 0; i < pos; ++i) */
	/* 	iter = fdictIterNext(dict, iter); */
	
	return iter;
}

dictIter * fdictIterNext(fdict * dict, dictIter * iter){
	dictHeader * h;
	int flag = 0, hflag = 1;
	size_t i, level = 0;

	if(!iter) return NULL;
	/* no more */
	if(iter->no >= dict->header[0].used + dict->header[1].used-1){
		free(iter);
		return NULL;
	}

	h = &dict->header[0];
	if(iter->iter_index >= h->size){
		level = h->size;
		h = &dict->header[1];
		hflag = 0;
	}

	for(i = iter->iter_index - level; i < h->size; ){
		if(!h->table[i]){
			++i;
			continue;
		}
		
		if(flag == 1){
			iter->current = h->table[i];
			iter->iter_index = i + level;
			++iter->no;
			return iter;
		}
		
		if(iter->current->next){
			iter->current = iter->current->next;
			iter->iter_index = i + level;
			++iter->no;
			return iter;
		}else{
			++i;
			flag = 1;
		}

		if(i == h->size && fdictIsRehash(dict) && hflag){
			h = &dict->header[1];
			hflag = 0;
			i = 0;
			level = dict->header[0].size;
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

/****************Test use ******************/
void fdictInfo(fdict * dict){
    printf("rehash_index:%d; iter_num:%d\n", dict->rehash_index,
		   dict->iter_num);
    dictHeader * h;
    for(int j = 0; j < 2; ++j){
        h = &dict->header[j];
        printf("h%d: size:%d; used:%d\n", j, h->size, h->used);
        
        for(int i = 0; i < h->size; ++i){
            dictNode * p = h->table[i];
            printf("%4d(p:%9d):", i, (int)p);
            while(p){
                printf("%s:%d->", (char *)p->key, (int)&p->value);
                p = p->next;
            }
            printf("\n");
        }
    }
}
//#define DEBUG_FDICT 1
#ifdef DEBUG_FDICT
int main(){    
    fdict * dict = fdictCreate();
    dict->type = &dictTypeDupKey;
    
    //srand(time(NULL));
    fdictAdd(dict, "count:1", "collapse");
    fdictAdd(dict, "count:2", "213213");
    fdictAdd(dict, "count:3", "futali");
	fdictAdd(dict, "count:4", "fdafdaf");
    fdictAdd(dict, "count:5", "jodandesu");
	fdictAdd(dict, "count:6", "fdafdaf");
	fdictAdd(dict, "count:7", "jodandesu");
//	fdictAdd(dict, "count:8", "jodandesu");
//	fdictAdd(dict, "count:8", "jodandesu");
	
	fdictInfo(dict);
    //_Expand(dict, 8);
    //_RehashStep(dict, 10);
    //printf("replace:%d\n",fdictReplace(dict, "22222222", "fdafda"));
    //printf("replace:%d\n",fdictReplace(dict, "33333", "333333"));
    //fdictInfo(dict);
    
    //fdictRemove(dict, "22222222");
    //fdictInfo(dict);
    
    //fdictInfo(dict);
	
	/* dictIter * iter = fdictIterCreate(dict, 0); */

	/* if(!iter){ */
	/* 	printf("out of range\n"); */
	/* 	return 0; */
	/* }	 */
	/* printf("%u:%s-%s\n", iter->iter_index, */
	/* 	   (char *)iter->current->key, */
	/* 	   (char *)iter->current->value.val); */
	/* while((iter = fdictIterNext(dict, iter))){ */
	/* 	printf("%u:%s-%s\n", iter->iter_index, */
	/* 		   (char *)iter->current->key, */
	/* 	   (char *)iter->current->value.val); */
	/* } */

	/* for(int i = 0; i < 4; ++i){ */
	/* 	fdictSetRandSeed(time(NULL)); */
	/* 	sleep(1); */
	/* 	dictNode * s = fdictGetRandom(dict); */
	/* 	if(s){ */
	/* 		printf("%s-%s\n", (char *)s->key, */
	/* 			   (char *)s->value.val); */
	/* 	} */
	/* } */
	
    return 0;
}
#endif
