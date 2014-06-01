#include <stdio.h>
#include </home/fankux/db/fdList.h>


int main(void){
    int a = 1, b = 2, c = 3, d = 4, e = 5;
	int re;
    char str[] = "fdafd_flfjdlajdl_d";
    char pattern[] = "fdafd";
	fdListNode * p;
	
    fdList * list = fdListCreate();
	list->CmpValFunc = &fdListCmpInt;
	
    printf("******Add*******\n");
    fdListAddHead(list, &a);
    /* fdListInfo(list); */
    fdListAddTail(list, &a);
    /* fdListInfo(list); */
    fdListAddTail(list, &a);
    fdListInfo(list);

	fdListRemove(list, &a, 0, 0);
	fdListInfo(list);

	
	/* re = fdListInsert(list, &d, &a , 2); */
	/* printf("insert re:%d\n", re); */
	/* if(FDLIST_OK == re) */
	/* 	fdListInfo(list); */
		
	/* p = fdListGetIndex(list, -1); */
	/* if(p) printf("p value:%d\n", *(int *)p->data); */
	/* else printf("p is null\n"); */
    /* printf("******Pop*******\n"); */
    /* fdListPopHead(list); */
    /* fdListInfo(list); */
    /* fdListPopTail(list); */
    /* fdListInfo(list); */

	/* fdListIter * iter = fdListIterCreate(list, FDLIST_START_TAIL, 3); */
	/* fdListNode * p ; */
	 
	/* if(iter == NULL) return 0; */
	/* while((p = fdListIterNext(iter))){ */
	/* 	printf("%ul:%d->", iter->rank, *(int *)p->data); */
	/* 	//fdListIterCancel(iter); */
	/* } */

	/* p = fdListGetRandom(list, time(0)); */
	/* if(p){ */
	/*  	printf("\n%d\n", *(int *)p->data); */
	/* } */
	 
	/* fdListFree(list); */
	
    return 0;
}
