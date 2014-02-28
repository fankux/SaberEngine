#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "common.h"
#include "fstr.h"

fstr * fstrCreateLen(const char * str, const size_t len){
    fstr * p;
	if(NULL == (p = (fstr *)malloc(sizeof(fstr) + len + 1))) return NULL;
	
	if(str != NULL){
		size_t str_len = strlen(str);
		p->len = str_len > len? len: str_len;
		p->free = len - p->len;

		memcpy(p->buf, str, p->len);
		p->buf[str_len] = '\0';
	}else{
		p->len = 0;
		p->free = len;
    }
	
    return p;
}

fstr * fstrCreate(const char * str){
	return fstrCreateLen(str, (str == NULL)? 0: strlen(str));
}

fstr * fstrCreateInt(const int64_t value){
	fstr * p;
	if(NULL == (p = (fstr *)malloc(sizeof(fstr) + sizeof(int64_t))))
		return NULL;

	memcpy(p->buf, &value, sizeof(value));

	return p;
}

void fstrFree(fstr * str){
	free(str->buf);
	free(str);
}

fstr * fstrCatLen(fstr * a, const char * b, const size_t len){
    fstr * p = a;    
    int newfree = a->free - len;
    if(a->free < len){   //space not enough,realloc double space of which needed
        if(NULL == (p = (fstr *)malloc(sizeof(fstr) + (p->len + len) * 2 + 1)))
			return NULL;
        memcpy(p->buf, a->buf, a->len);
        newfree = a->len + len;
    }
    //complete cat
    memcpy(p->buf + a->len, b, len);
    p->buf[a->len + len] = '\0';
    
    p->len = a->len + len;
    p->free = newfree;

	free(a);
	a = p;
	
    return p;
}

fstr * fstrCat(fstr * a, const char * b){
	return fstrCatLen(a, b, strlen(b));
}

#define FSTR_TRIMBOTH 0
#define FSTR_TRIMLEFT 1
#define FSTR_TRIMRIGHT 2

fstr * fstrTrim(fstr * p, const int FLAG){
    int i = 0, j = 0, front_blank_num = 0, tail_blank_num = 0;
    
    while(i < p->len){
        int f = 0;
        if(p->buf[i] == ' ' &&
		   (FLAG == 0 || FLAG == FSTR_TRIMLEFT)){
            ++front_blank_num;
            f = 1;
        }
        if(p->buf[p->len - i - 1] == ' ' &&
		   (FLAG == 0 || FLAG == FSTR_TRIMRIGHT)){
            ++tail_blank_num;
            f = 1;
        }
        if(f == 0){
            break;
        }
        ++i;
    }
    for(i = front_blank_num; i < p->len - tail_blank_num && i != j;
		++i, ++j){
       p->buf[j] = p->buf[i]; 
    }
    p->len -= front_blank_num + tail_blank_num;
    p->buf[p->len] = '\0';
    
    return p;
}

fstr * fstrRemoveLen(fstr * p, const size_t start, const size_t len){
    int i, j;
    i = min(start, start + len);
    j = max(start, start + len);
    if(i >= p->len || i < 0 || j < 0 || j > p->len) return NULL;
	for(;j < p->len; ++i, ++j){
        p->buf[i] = p->buf[j];
    }
    p->len -= j - i;
    
    return p;
}

fstr * fstrCopyLen(fstr * a, const size_t start, const size_t len){
    int i, j;
    i = min(start, start + len);
    j = max(start, start + len);
    if(i >= a->len || i < 0 || j < 0 || j > a->len) return NULL;
    
    fstr * p;
	if(NULL == (p = (fstr *)malloc(sizeof(fstr) + len + 1))) return NULL;
    p->len = j - i;
    p->free = 0;
    
    memcpy(p->buf, a->buf + i, j - i);
    p->buf[p->len] = '\0';
	
    return p;
}

fstr * fstrCopy(fstr * a){
    return fstrCopyLen(a, 0, strlen(a->buf));
}

fstr * fstrReplaceLen(fstr * a, char * b, const size_t pos, const size_t len){
	if(pos >= a->len) return NULL;

	size_t str_len = strlen(b);
	if(str_len > len) str_len = len;
	if(pos + str_len >= a->len) return NULL;

	memcpy(a->buf + pos, b, str_len);

	return a;
}

fstr * fstrReplace(fstr * a, char * b, const size_t pos){
	return fstrReplaceLen(a, b, pos, strlen(b));
}

fstr * fstrInsertLen(fstr * a, char * b, const size_t pos, const size_t len){
	if(pos > a->len) return NULL;

	size_t str_len = strlen(b);
	if(str_len > len) str_len = len;
	fstr * p = a;
	
	if(str_len > a->free){
		if(NULL == (p = (fstr *)malloc(sizeof(fstr) +
									   2 * (a->len + str_len) + 1)))
			return NULL;

		p->free = p->len = a->len + str_len;
		memcpy(p->buf, a->buf, pos);  //original buf before pos
		memcpy(p->buf + pos + str_len, a->buf + pos, a->len - pos);
		
		free(a);
		a = p;
	}else{
		p->free -= str_len;
		p->len += str_len;
		memcpy(a->buf + pos + str_len,
			   a->buf + pos, a->len - pos);//move
	}
	memcpy(p->buf + pos, b, str_len); //string to be inserted
	p->buf[p->len] = '\0';

	return p;
}

fstr * fstrInsert(fstr * a, char * b, const size_t pos){
	return fstrInsertLen(a, b, pos, strlen(b));
}

int fstrCompare(fstr * a, fstr * b){
	int i = 0;
	if(a->len != b->len){
        return a->len < b->len ? -1 : (a->len == b->len ? 0 : 1);
    }
    while(i < a->len){
        if(a->buf[i] != b->buf[i]){
            return a->buf[i] < b->buf[i] ? -1 :
                    (a->buf[i] == b->buf[i] ? 0 : 1);
        }
        ++i;
    }
    return 0;
}

static void fstrInfo(fstr * p){
    if(p == NULL){
        printf("null\n");
        return;
    }
    for(int i = 0; i < p->len; i++){
        printf("%c",p->buf[i]);
    }
    printf(";length=%d,free=%d\n", p->len, p->free);
}

/*int main(){
    char str_key[] = "     qwert  yuiop[]|   ";
    char str_value[] = "qqqqqwwwwweeeeerrrrrttttt";
    char str_value1[] = "sssssss";
    char str_value2[] = "ssssss1";
    
    fstr * p = fstrCreate(str_value);
    fstr * p1 = fstrCreate(str_value1);
    fstr * p2 = fstrCreate(str_value2);
    fstrInfo(p);
    
    //printf("%d %d %d\n", fstrCompare(p, p), fstrCompare(p1, p2),
    //                fstrCompare(p1, p));
	
	p = fstrInsertLen(p, "bbbbbdafdaf", 25, 5);
	fstrInfo(p);
    p = fstrInsertLen(p, "ccccc", 0, 3);
	fstrInfo(p);
    return 0;
	}*/