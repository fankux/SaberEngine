#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "fdList.h"

fdList * fdListCreate(void){
	fdList * list;
    if(!(list = (fdList *)malloc(sizeof(fdList))))
        return NULL;

	memset(list, 0, sizeof(fdList));
    
    return list;
}

void fdListFree(fdList * list){
	fdListNode * p = list->head;
	fdListNode * q = p;
	for(size_t i = list->len; i > 0; --i){
		free(q);
		p = p->next;
		q = p;
	}
	free(list);
}

/* get a node by 'value', if not exist, return FDLIST_NONE
** else set p to it */
int fdListGet(fdList * list, void * value, fdListNode ** p){
	fdListNode * head = list->head;
	
	while(head){
		if(fdListCmpVal(list, head->data, value) == 0){
			if(p) *p = head;
			return FDLIST_OK;
		}
		head = head->next;
	}

	return FDLIST_NONE;
}

/* add node at header */
int fdListAddHead(fdList * p, void * data){
    fdListNode * new;
	if(NULL == (new = (fdListNode *)malloc(sizeof(fdListNode))))
		return FDLIST_FAILD;
	
    new->data = fdListDupVal(p, data);
	
    if(fdListIsEmpty(p)){//list empty, head, tail pointing new point
        new->prev = new->next = NULL;
        p->head = p->tail = new;
    }else{
        new->prev = NULL;
        new->next = p->head;
		p->head = p->head->prev = new;
    }
    ++p->len;
    
    return FDLIST_OK;
}

/* add node at tail */
int fdListAddTail(fdList * p, void * data){
    fdListNode * new;
	if(!(new = (fdListNode *)malloc(sizeof(fdListNode))))
        return FDLIST_FAILD;
    
    new->data = fdListDupVal(p, data);
	
    if(fdListIsEmpty(p)){
        new->prev = new->next = NULL;
        p->head = p->tail = new;
    }else{
        new->next = NULL;
        new->prev = p->tail;
        p->tail = p->tail->next = new;
    }
    ++p->len;
    
    return FDLIST_OK;
}

/* delete node at header */
fdListNode * fdListPopHead(fdList * list){
    fdListNode * p = NULL;
    if(fdListIsEmpty(list))
        return NULL;
    
    else if(list->len == 1){ /* last one node */
        p = list->head;
        list->head = list->tail = NULL;
    }else{
        p = list->head;
        list->head = list->head->next;
        list->head->prev = NULL;
        
    }
    --list->len;
    return p;
}

/* delete node at tail */
fdListNode * fdListPopTail(fdList * list){
    fdListNode * p = NULL;
    if(fdListIsEmpty(list))
        return NULL;
    
    else if(list->len == 1){
        p = list->tail;
        list->head = list->tail = NULL;
    }else{
        p = list->tail;
        list->tail = list->tail->prev;
        list->tail->next = NULL;
    }
    --list->len;
    return p;
}

int fdListSet(fdList * list, const size_t index, void * value){
	if(index >= fdListLen(list))
		return FDLIST_OUTRANGE;

	fdListNode * p = list->head;
	int i = 0;

	while(i < index){
		p = p->next;
		++i;
	}
	
	fdListFreeVal(list, p);
	p->data = fdListDupVal(list, value);

	return FDLIST_OK;
}

/* get the 'left to right' rank of first node which value equal 'value',
** if no result , result -1 */
int fdListIndexOf(fdList * list, void * value){
	fdListNode * p = list->head;

	int  i = 0;
	while(p){
		if(p->data && fdListCmpVal(list, p->data, value) == 0)
			return i;
		else
			++i;
	}
	
	return -1;
}

/* unlink the p from list */
#define riditof(list, p)						\
	if(list->len == 1){							\
		list->head = list->tail = NULL;			\
	}else{										\
		if((p)->prev == NULL){					\
			list->head = (p)->next;				\
			(p)->next->prev = NULL;				\
		}else if((p)->next == NULL){			\
			list->tail = (p)->prev;				\
			(p)->prev->next = NULL;				\
		}else{									\
			(p)->next->prev = (p)->prev;		\
			(p)->prev->next = (p)->next;		\
		}										\
	}

/* remove number of absolute value of 'count' node from 'pos',
** it's value equal to 'value'
** count > 0, search from left to right,
** count < 0, search from right to left,
** count = 0, remove all, 'pos' direction depend on value sign,
** return value is the actually number of deleted nodes */
int fdListRemove(fdList * list, void * value,
				 const size_t pos, int count){
	fdListNode * p, * q;
	size_t  i = 0;
	int n = 0;
	
	if(pos >= fdListLen(list))
		return FDLIST_NONE;
	
	if(count < 0){/* start scan from tail */
		count = 0 - count;
		p = list->tail;
		while(i++ < pos){/* go forward to 'pos' */
			p = p->prev;
		}
		while(p && n < count){
			/* printf("n:%d, count:%d \n", n, count); */
			q = p;
			p = p->prev;
			if(0 == fdListCmpVal(list, q->data, value)){
				riditof(list, q);
				fdListFreeVal(list, q);
				free(q);
				++n;
			}
		}
	}else{/* count >= 0 */
		p = list->head;
		while(i++ < pos){/* go forward to 'pos' */
			p = p->next;
		}
		while(p && (n < count || count == 0)){
			q = p;
			p = p->next;
			if(0 == fdListCmpVal(list, q->data, value)){
				riditof(list, q);
				fdListFreeVal(list, q);
				free(q);
				++n;
			}
		}
	}

	return n;
}
/* remove a node at position of 'pos', pos is 0 base,
** if 'out_val' is NULL, the value at 'pos' would be freed,
** else value at 'pos' would be linked to 'out_val' */
int fdListRemoveAt(fdList * list, const size_t pos, void ** out_val){
	fdListNode * p, * q;
	size_t i;
	
	if(pos >= fdListLen(list))/* pos type of unsigned, never minus */
		return FDLIST_OUTRANGE;
	
	p = list->head;
	i = -1;
	while(++i < pos){
		p = p->next;
	}
	q = p;

	/* free or store out the p->value */
	if(out_val == NULL){
		fdListFreeVal(list, p);
	}else{
		*out_val = p->data;
	}

	riditof(list, p);
	
	free(q);
	--list->len;

	return FDLIST_OK;
}

/* remove first node which value equals to 'value',
** 'null' can't equal anything
** 'value' just send to CmpFunc, different implement of CmpFunc
** may define different value type */
int fdListRemoveValue(fdList * list, void * value, void ** out_val){
	fdListNode * p, * q;
	size_t i;
	
	p = list->head;

	while(p){
		q = p;
		if(p->data && 0 == fdListCmpVal(list, p->data, value))
			break;
		p = p->next;
	}

	/* no result */
	if(p == NULL){
		if(out_val)
			*out_val = NULL;
		return FDLIST_NONE;
	}
	
	/* free or store out the p->value */
	if(out_val == NULL){
		fdListFreeVal(list, p);
	}else{
		*out_val = p->data;
	}

	riditof(list, p);
	
	free(q);
	--list->len;

	return FDLIST_OK;
}

/* iterator implements, pos is 0 base */
fdListIter * fdListIterCreate(fdList * list, const uint8_t direct,
							  const size_t pos){
	fdListIter * iter;
	size_t i;
	
	if(fdListIsEmpty(list) || pos >= fdListLen(list))
		return NULL;
	
	if(NULL == (iter = (fdListIter *)malloc(sizeof(fdListIter))))
		return NULL;

	iter->direct = direct;
	iter->rank = -1; /* note that iter->rank is size_t */
	if(direct == FDLIST_START_HEAD)
		iter->next = list->head;
	else if(direct == FDLIST_START_TAIL)
		iter->next = list->tail;

	/* there wouldn't be in condition that 'next' is NULL
	** in case of that 'pos' be ensured being in bound */
	for(i = 0; i < pos; ++i){ 
		fdListIterNext(iter);
	}
	
	return iter;
}

fdListNode * fdListIterNext(fdListIter * iter){
	fdListNode * p;
	
	if(NULL == iter->next){
		fdListIterCancel(iter);
		return NULL;
	}
	
	p = iter->next;
	++iter->rank;

	if(iter->direct == FDLIST_START_HEAD)
		iter->next = p->next;
	else if(iter->direct == FDLIST_START_TAIL)
		iter->next = p->prev;
	
	return p;
}

/* rewind the iterator as it's direction */
void fdListIterRewind(fdList * list, fdListIter * iter){
	if(fdListIsEmpty(list)){
		fdListIterCancel(iter);
		return;
	}
	
	iter->rank = -1;
	/* if(iter->direct == FDLIST_START_HEAD) */
	iter->next = list->head;/* head first is default */
	if(iter->direct == FDLIST_START_TAIL)
		iter->next = list->tail;

	return;
}

void fdListIterCancel(fdListIter * iter){
	free(iter);
}
fdListNode * fdListGetIndex(fdList * list, const size_t index){
	if(index >= fdListLen(list)) return NULL;

	fdListIter * iter;
	fdListNode * p;
	if(!(iter = fdListIterCreate(list, FDLIST_START_HEAD,
								 index)))
		return NULL;
	p = fdListIterNext(iter);
	fdListIterCancel(iter);
	
	return p;

}
fdListNode * fdListGetRandom(fdList * list, const int seed){
	srand(seed);
	size_t pos = rand() % fdListLen(list);
	fdListIter * iter;
	fdListNode * p;
	if(!(iter = fdListIterCreate(list, FDLIST_START_HEAD, pos)))
		return NULL;
	p = fdListIterNext(iter);
	fdListIterCancel(iter);
	
	return p;
}

void fdListInfo(fdList * p){
    if(p == NULL){
        printf("null\n");
        return;
    }
    fdListNode * iter = p->head;
    printf("next:");
    while(iter){
        printf("%d->", (int)iter->data);
        iter = iter->next;
    }
	printf("null\n");
    /* printf("null;\nprev:"); */
    /* iter = p->tail; */
    /* while(iter){ */
    /*     printf("%d->", *((int *)(iter->data))); */
    /*     iter = iter->prev; */
    /* } */
    /* printf("null;length:%u\n", p->len); */
	printf("length:%u\n", p->len);
}
