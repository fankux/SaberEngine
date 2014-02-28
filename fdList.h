#ifndef FDLIST_H
#define FDLIST_H

/* error code */
#define FDLIST_FAILD -1 //memory faild
#define FDLIST_NONE 0
#define FDLIST_OK 1

typedef struct fdListNode{
    struct fdListNode * prev;
    struct fdListNode * next;
    void * data;
}fdListNode;

typedef struct fdList{
    size_t len;
    struct fdListNode * head;
    struct fdListNode * tail;
}fdList;

/* macros */
#define fdListEmpty(list) (list->len <= 0)

/* API */
fdList * fdListCreate(void);
void fdListFree(fdList * list);
int fdListAddHead(fdList * p, void * value);
int fdListAddTail(fdList * p, void * value);
fdListNode * fdListPopHead(fdList * list);
fdListNode * fdListPopTail(fdList * list);

#endif /* fdList.h */
