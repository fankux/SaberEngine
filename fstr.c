#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "fstr.h"

fstr * fstrCreateLen(const char * str, const size_t len){
    fstr * p;
	if(!(p = (fstr *)malloc(sizeof(fstr) + len + 1)))
		return NULL;
	
	if(str){
		size_t str_len = strlen(str);
		p->len = str_len > len? len: str_len;
		p->free = len - p->len;

		memcpy(p->buf, str, p->len);
		p->buf[p->len] = '\0';
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
	if(!(p = (fstr *)malloc(sizeof(fstr) + sizeof(int64_t))))
		return NULL;

	memcpy(p->buf, &value, sizeof(value));

	return p;
}

void fstrEmpty(fstr * str){
	str->free += str->len;
	str->len = 0;
	if(str->buf)
		str->buf[0] = '\0';
}

void fstrFree(fstr * str){
	free(str);
}

fstr * fstrCatLen(fstr * a, const char * b, const size_t len){
    fstr * p = a;    
    int newfree = a->free - len;
	int flag = 0;
	
	/* space not enough,realloc double space of which needed */
	if(a->free < len){
        if(!(p = malloc(sizeof(fstr) + ((p->len + len) << 2) + 1)))
			return NULL;
        memcpy(p->buf, a->buf, a->len);
        newfree = a->len + len;
		flag = 1;
    }
    /* complete cat */
    memcpy(p->buf + a->len, b, len);
    p->buf[a->len + len] = '\0';
    
    p->len = a->len + len;
    p->free = newfree;

	if(flag) free(a);
	
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

/* replace chars which length len */
fstr * fstrSetLen(fstr * s, char * str, const size_t start, const size_t len){
	fstr * p = s;

	if(start > p->len)/* out of bound */
		return NULL;
	
	if(p->len + p->free < len + start){/* space need realloc */
		if(!(p = (fstr *)malloc(sizeof(fstr) +
								((len + start) << 1) + 1)))
			return NULL;

		memcpy(p->buf, s->buf, start);
		p->len = p->free = len + start;
		fstrFree(s);
	}else{
		p->free = p->free + p->len - len - start;
		p->len = len + start;
	}
	
	memcpy(p->buf + start, str, len);
	*(p->buf + start + len) = '\0';

	return p;
}

fstr * fstrSet(fstr * p, char * str, const size_t start){
	return fstrSetLen(p, str, start, strlen(str));
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

fstr * fstrDupLen(fstr * a, const size_t start, const size_t len){
    int i, j;
    i = min(start, start + len);
    j = max(start, start + len);
    if(i >= a->len || i < 0 || j < 0 || j > a->len)
		return NULL;
    
    fstr * p;
	if(!(p = malloc(sizeof(fstr) + len + 1)))
		return NULL;
    p->len = j - i;
    p->free = 0;
    
    memcpy(p->buf, a->buf + i, j - i);
    p->buf[p->len] = '\0';
	
    return p;
}

fstr * fstrDup(fstr * a){
    return fstrDupLen(a, 0, a->len);
}

/* copy 'b' to 'a' till 'c' appear in 'b',
** original content of 'a' will be covered and cleared */
fstr * fstrDupEndPoint(fstr * b, const char c){
	fstr * p;
	int i = 0;
	
	while(i < b->len && b->buf[i] != c )
		++i;
	if(i == b->len) return NULL; /* no 'c' appear */

	if(!(p = (fstr *)malloc(
			 sizeof(fstr) + ((i + 2) << 1 ) * sizeof(char))))
		return NULL;

	p->free = p->len = i + 1;
	
	memcpy(p->buf, b->buf, i + 1);
	p->buf[p->len] = '\0';
	
	return p;
}

fstr * fstrReplaceLen(fstr * a, char * b,
					  const size_t pos, const size_t len){
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

fstr * fstrInsertLen(fstr * a, char * b, const size_t pos,
					 const size_t len){
	size_t str_len;
	fstr * p;
	
	if(pos > a->len) return NULL;

	str_len = strlen(b);
	if(str_len > len) str_len = len;

	p = a;
	
	if(str_len > a->free){
		if(!(p = malloc(sizeof(fstr) +
						(a->len + str_len << 1) + 1)))
			return NULL;

		p->free = p->len = a->len + str_len;
		memcpy(p->buf, a->buf, pos);  /* original buf before pos */
		memcpy(p->buf + pos + str_len, a->buf + pos, a->len - pos);
		
		free(a);
	}else{
	    memmove(p->buf + pos + str_len, p->buf + pos, p->len); /* move */

		p->free -= str_len;
		p->len += str_len;
	}
	memcpy(p->buf + pos, b, str_len); /* string to be inserted */
	p->buf[p->len] = '\0';

	return p;
}

fstr * fstrInsert(fstr * a, char * b, const size_t pos){
	return fstrInsertLen(a, b, pos, strlen(b));
}

/* clear invisiblily characters in the string */
void fstrSqueeze(fstr * a){
	size_t i, s = 0, l = 0;
	int re, flag = 0;
	
	for(i = 0; i < a->len; ++i){
		re = isspace(a->buf[i]);
		if(re && 0 == flag){/* start count space characters */
			s = i;
			++l;

			flag = 1;
		}else if(re && 1 == flag){/* counting */
			++l;

			if(i == a->len - 1){/* end of string */
				a->len -= l;
				a->free += l;
				
				return;
			}
			
		}else if(!re && 1 == flag){
			/* counting complete */
			/* clear invisiblity characters by memcpy */
			memcpy(a->buf + s, a->buf + s + l, a->len - s - l);
			a->len -= l;
			a->free += l;
			i -= l;
			l = 0;

			flag = 0;
		}else{/* nothing */

		}
	}
	
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

void fstrInfo(fstr * p){
    if(p == NULL){
        printf("null\n");
        return;
    }
    for(int i = 0; i < p->len; i++){
        printf("%c",p->buf[i]);
    }
    printf(";length=%d,free=%d\n", p->len, p->free);
}

/* int main(){ */
/*     char str_key[] = "     qwert  yuiop[]|   "; */
/*     char str_value[] = "qqqqqwwwww"; */
/*     char str_value1[] = "qweuuurtyuuudu"; */
/*     char str_value2[] = "ssssss1"; */
    
/*     fstr * p = fstrCreate(str_value); */
/*     fstr * p1 = fstrCreate(str_value1); */
/*     fstr * p2 = fstrCreate(str_value2); */
/*     fstrInfo(p); */
/* 	p = fstrSetLen(p, "sssss", 10, 1); */
/* 	fstrInfo(p); */

/* 	/\* fstrSqueeze(p1); *\/ */
/* 	/\* fstrInfo(p1); *\/ */
	
/*     /\* printf("%d %d %d\n", fstrCompare(p, p), fstrCompare(p1, p2), *\/ */
/*     /\*                 fstrCompare(p1, p)); *\/ */
	
/* 	/\* p = fstrInsertLen(p, "bbbbbdafdaf", 10, 5); *\/ */
/* 	/\* fstrInfo(p); *\/ */
/*     /\* p = fstrInsertLen(p, "ccccc", 0, 3); *\/ */
/* 	/\* fstrInfo(p); *\/ */
/* 	/\* fstr * re = fstrDupEndPoint(p1, '6'); *\/ */
/* 	/\* fstrInfo(re); *\/ */
	
/* 	/\* re = fstrDupEndPoint(p1, 'd'); *\/ */
/* 	/\* fstrInfo(re); *\/ */

/* 	/\* re = fstrCopyEndPoint(p, p1, 'u'); *\/ */
/* 	/\* printf("copy re:%d\n", re); *\/ */
/* 	/\* fstrInfo(p); *\/ */

/* 	/\* re = fstrCopyEndPoint(p, p1, 'r'); *\/ */
/* 	/\* printf("copy re:%d\n", re); *\/ */
/* 	/\* fstrInfo(p); *\/ */
/* 	/\* fstrFree(p1); *\/ */
	
/* 	return 0; */
/* } */
