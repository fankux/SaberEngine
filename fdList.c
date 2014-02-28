#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "fdList.h"

fdList * fdListCreate(void){
	fdList * list;
    if(NULL == (list = (fdList *)malloc(sizeof(fdList))))
        return NULL;
    
    list->len = 0;
    list->head = NULL;
    list->tail = NULL;
    
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

//add node at header
int fdListAddHead(fdList * p, void * data){
    fdListNode * new;
	if(NULL == (new = (fdListNode *)malloc(sizeof(fdListNode))))
		return FDLIST_FAILD;
	
    new->data = data;
    if(fdListEmpty(p)){//list empty，head,tail pointing new point
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

//add node at tail
int fdListAddTail(fdList * p, void * data){
    fdListNode * new;
	if(NULL == (new = (fdListNode *)malloc(sizeof(fdListNode))))
        return FDLIST_FAILD;
    
    new->data = data;
    if(fdListEmpty(p)){
        new->prev = new->next = NULL;
        p->head = new;
    }else{
        new->next = NULL;
        new->prev = p->tail;
        p->tail = p->tail->next = new;
    }
    ++p->len;
    
    return FDLIST_OK;
}

//delete node at header
fdListNode * fdListPopHead(fdList * list){
    fdListNode * p = NULL;
    if(fdListEmpty(list))
        return NULL;
    
    else if(list->len == 1){ //last one node
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

//delete node at tail
fdListNode * fdListPopTail(fdList * list){
    fdListNode * p = NULL;
    if(fdListEmpty(list))
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

static void fdListInfo(const fdList * p){
    if(p == NULL){
        printf("null\n");
        return;
    }
    fdListNode * iter = p->head;
    printf("next:");
    while(iter){
        printf("%d->", *((int *)(iter->data)));
        iter = iter->next;
    }
    printf("null;\nprev:");
    iter = p->tail;
    while(iter){
        printf("%d->", *((int *)(iter->data)));
        iter = iter->prev;
    }
    printf("null;length:%u\n", p->len);
}

//test
/*int main(void){
    int a = 231, b = 324, c = 432, d = 32, e = 32; 
    char str[] = "fdafd_flfjdlajdl_d";
    char pattern[] = "fdafd";
    
    fdList * list = fdListCreate();
        
    
    printf("******Add*******\n");
    fdListAddHead(list, &a);
    fdListInfo(list);
    fdListAddTail(list, &b);
    fdListInfo(list);
    fdListAddTail(list, &c);
    fdListInfo(list);
    
    
    printf("******Pop*******\n");
    fdListPopHead(list);
    fdListInfo(list);
    fdListPopTail(list);
    fdListInfo(list);
    
	fdListFree(list);
	
    return 0;
}*/






















