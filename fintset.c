#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fintset.h"

/* constant */
#define FINTSET_INT16 (sizeof(int16_t))
#define FINTSET_INT32 (sizeof(int32_t))
#define FINTSET_INT64 (sizeof(int64_t))

/* private function */
static uint8_t _Find(fintset *, const int64_t, size_t *);
static inline int _IntegerSize(const int64_t);

/* API Implement */
fintset * fintsetCreate(void){
    fintset * new = (fintset *)malloc(sizeof(fintset));
    new->encode = FINTSET_INT16;
    new->len = 0;
    
    return new;
}

void fintsetFree(fintset * is){
	free(is->sets);
	free(is);
}

static inline int _IntegerSize(const int64_t value){
    if(value <= INT16_MAX) return FINTSET_INT16;
    else if(value > INT16_MAX && value <= INT32_MAX) return FINTSET_INT32;
    else return FINTSET_INT64;
}

/* binary search
** if find, return 1, and set it's position to 'pos'
** neither return 0,and the 'pos' is the position where 'value' would be; */
static uint8_t _Find(fintset * is, const int64_t value, size_t * pos){
    if(is->len <= 0) return 0;
    int l = 0;
    int h = is->len;
    int m = 0;
    int64_t num = 0;
    while(l <= h && l < is->len){
        m = (l + h) / 2;
        memcpy(&num, is->sets + m * is->encode, is->encode);
        if(num == value){
            *pos = m;
            return 1;
        }else if(num < value){
            l = m + 1;
			++m;   //adjust 'pos'
        }else{
            h = m - 1;
        }
    }
    *pos = m;
    return 0;
}
uint8_t fintsetSearch(fintset * is, const int64_t value, size_t * pos){
	if(_Find(is, value, pos))
		return FINTSET_SUCCESS;
	else
		return FINTSET_NONE;
}
int fintsetAdd(fintset * is, const int64_t value){
    size_t pos = 0;
	if(_Find(is, value, &pos) == 1) return FINTSET_EXIST;
    
    uint8_t size = _IntegerSize(value);
    if(!(is = (fintset *)realloc(is, sizeof(fintset) + 
                (is->len + 1) * size))) return FINTSET_FAILD;
    
	//printf("pos:%d;size:%d\n", (int)pos, (int)size);
    //upgrade:16bit=>32bit or 16bit=>64bit or 32bit=>64bit
    if(size > is->encode && is->len > 0){
		int64_t addr_last = (is->len - 1) * is->encode;
		int64_t addr_last_new = (is->len - 1) * size;
		
		//new node is the last one
		memcpy(is->sets + addr_last_new + size, &value, size);
        //upgrade
		for(; addr_last >= 0; addr_last_new -= size, addr_last -= is->encode){
			//printf("addr_last:%d;addr_last_new:%d\n",
			//	   (int)addr_last, (int)addr_last_new);	
			memcpy(is->sets + addr_last_new, 
				   is->sets + addr_last, is->encode);
			memset(is->sets + addr_last_new + is->encode,
				   0, size - is->encode);
		}
        is->encode = size;
		++is->len;

        return FINTSET_SUCCESS;
    }
    
    //need not upgrade, just move and add new node
    pos = (size_t)(is->sets + pos * size);
    size_t i = (size_t)(is->sets + is->len * size);
    for(; i > pos; i -= size){
		memcpy((void *)i, (void *)(i - size), size);
    }
    
    //add new node
    is->encode = size;
    memcpy((void *)pos, &value, size);
    ++is->len;

    return FINTSET_SUCCESS;
}
uint8_t fintsetRemove(fintset * is, const int64_t value){
	size_t pos;
	if(!_Find(is, value, &pos) || is->len <= 0) return FINTSET_NONE;

	//printf("pos:%u", pos);
	pos = (size_t)(is->sets + pos * is->encode);
	size_t i = (size_t)(is->sets + (is->len - 1) * is->encode);
	
	//printf("pos:%u;i:%u\n", pos, i);
	for(; pos < i; pos += is->encode){
		memcpy((void *)pos, (void *)(pos + is->encode), is->encode);
	}
	
	is = (fintset *)realloc(is, sizeof(fintset) +
								  (is->len - 1) * is->encode);
	
	--is->len;

	return FINTSET_SUCCESS;
}
uint8_t fintsetGet(fintset * is, const size_t pos, int64_t * value){
	if(pos >= is->len) return FINTSET_NONE;
	
	*value = 0;
	memcpy((void *)value, (void *)(is->sets + pos * is->encode), is->encode);
		
	return FINTSET_SUCCESS;
}
static void fintsetInfo(fintset * is){
    printf("\n");
    int64_t value = 0;
    size_t addr_last = is->len * is->encode;
    for(size_t  i = 0; i < addr_last; i += is->encode){
        memcpy(&value, is->sets + i, is->encode);    
        printf("%lld ", value);
    }
    printf("\nlen:%d;encode:%d;start addr:%d\n",
            is->len, is->encode, (int)is->sets);
}

/*int main(void){
    fintset * is = fintsetCreate();
	//printf("Add result:%d",fintsetAdd(is, INT16_MAX));
	for(int i = 0; i < 5; ++i){
		int16_t n = rand() % INT16_MAX;
		//printf("%hd ", n);
		fintsetAdd(is, n);
	}
	//fintsetInfo(is);
	for(int i = 0; i < 5; ++i){
		int64_t n = INT64_MAX / 2 + rand() % INT32_MAX;
		//printf("%lld ", n);
		fintsetAdd(is, n);
	}
	fintsetInfo(is);
	size_t pos;
	uint16_t result = _Find(is, 7295, &pos);
	if(result){
		printf("serching%lld,result:%u, position:%hu\n",
			   *(int64_t *)(is->sets + pos * is->encode), result, pos);
	}else{
		printf("not exsited\n");
		}
	printf("remove:1123,result:%hu\n", fintsetRemove(is, 1123));
	printf("remove:2237,result:%hu\n", fintsetRemove(is, 2237));
	printf("remove:7295,result:%hu\n", fintsetRemove(is, 7295));
	printf("remove:4611686020077148395,result:%hu\n", fintsetRemove(is, 4611686020077148395));
    fintsetInfo(is);
	int64_t value;
	printf("get:-1, result:%hu\n", fintsetGet(is, -1, &value));
	printf("get:20, result:%hu\n", fintsetGet(is, 20, &value));
	printf("get:5,result:%hu;", fintsetGet(is, 5, &value));
	printf("value:%lld\n", value);
	
	return 0;
}*/










