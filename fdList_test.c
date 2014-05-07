#include <stdio.h>
#include </home/fankux/db/fdList.h>


int main(void){
    int a = 231, b = 324, c = 432, d = 32, e = 32;
    char str[] = "fdafd_flfjdlajdl_d";
    char pattern[] = "fdafd";
	fdListNode * p;
	
    fdList * list = fdListCreate();
	
    printf("******Add*******\n");
    fdListAddHead(list, &a);
    /* fdListInfo(list); */
    fdListAddTail(list, &b);
    /* fdListInfo(list); */
    fdListAddTail(list, &c);
    fdListInfo(list);

	p = fdListGetIndex(list, -1);
	if(p) printf("p value:%d\n", *(int *)p->data);
	else printf("p is null\n");
    /* fdListRemoveValue(list, &c, NULL); */
	/* fdListInfo(list); */
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
