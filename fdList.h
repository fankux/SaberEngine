#ifndef FDLIST_H
#define FDLIST_H

#include <inttypes.h>

/* error code */
#define FDLIST_OUTRANGE -2 /* pos overflow */
#define FDLIST_FAILD -1 /* memory faild */
#define FDLIST_NONE 0
#define FDLIST_OK 1

/* fdList max size is SIZE_MAX - 2,
** because of -1(SIZE_MAX) meaning faild,
** -2(SIZE_MAX - 1) meaning out of range */
typedef struct fdListNode{
    struct fdListNode * prev;
    struct fdListNode * next;
    void * data;
}fdListNode;

typedef struct fdList{
    size_t len;
    struct fdListNode * head;
    struct fdListNode * tail;

	void * (* DupValFunc)(void * value);
	int (* CmpValFunc)(void * a, void * b);
	void (* FreeValFunc)(void * a);
}fdList;

/* iter direction */
#define FDLIST_START_HEAD 0
#define FDLIST_START_TAIL 1

typedef struct fdListIter{
	int direct;
	size_t rank;
	fdListNode * next;
}fdListIter;

/* macros */
#define fdListIsEmpty(list) (list->len == 0)
#define fdListIsFull(list) (list->len == (SIZE_MAX - 2))
#define fdListLen(list) (list->len)
#define fdListFreeVal(list, n) do { \
	if(list->FreeValFunc) list->FreeValFunc(n->data); \
	else n->data = NULL; }while(0)
#define fdListCmpVal(list, val1, val2) \
	(list->CmpValFunc != NULL? list->CmpValFunc(val1, val2): \
    ((val1) > (val2)? 1: ((val2) > (val1)? -1: 0)))
#define fdListDupVal(list, value) \
	(list->DupValFunc? list->DupValFunc(value): value)

/* API */
fdList * fdListCreate(void);
void fdListFree(fdList * list);
int fdListSet(fdList * list, const size_t index, void * value);
int fdListInsert(fdList * list, void * value, void * pivot, 
				 const uint8_t aorb);
int fdListGet(fdList * list, void * value, fdListNode ** p);
int fdListAddHead(fdList * list, void * value);
int fdListAddTail(fdList * list, void * value); 
fdListNode * fdListPopHead(fdList * list);
fdListNode * fdListPopTail(fdList * list);
int fdListRemove(fdList * list, void * value,
				 const size_t pos, int count);
int fdListRemoveValue(fdList * list, void * value,
					  void ** out_val);
int fdListRemoveAt(fdList * list, const size_t pos,
				   void ** out_val);
fdListNode * fdListGetIndex(fdList *list , const size_t index);
fdListNode * fdListGetRandom(fdList * list, const int seed);
int fdListIndexOf(fdList * list, void * value);
fdListIter * fdListIterCreate(fdList * p, const uint8_t direct,
							  const size_t start_pos);
fdListNode * fdListIterNext(fdListIter * iter);
void fdListIterCancel(fdListIter * iter);
void fdListIterRewind(fdList * list, fdListIter * iter);
void fdListInfo(fdList * list);

/**** function type ****/
extern int fdListCmpInt(void * a, void * b);
extern int fdListCmpCaseStr(void * a, void * b);
extern int fdListCmpStr(void * a, void * b);


#endif /* fdList.h */
