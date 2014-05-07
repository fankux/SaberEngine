#ifndef FBINTREE_H
#define FBINTREE_H

#include <sys/types.h>
#include <inttypes.h>

/* error code */
#define FBINTREE_ERROR -2 //struct destroyed,can not be recoveryed
#define FBINTREE_FAILD -1 //memory faild
#define FBINTREE_NONE 0 //no result
#define FBINTREE_OK 1
#define FBINTREE_EXIST 2 //inserting action, key already exist 

typedef struct treeNode{
    int key;
    void * value;
    int bf; //balance factor
    
    struct treeNode * left;
    struct treeNode * right;
}treeNode;

typedef struct fbintree{
    treeNode * root;
    size_t num; 
}fbintree;

/* API */
fbintree * fbintreeCreate(void);
void fbintreeFree(fbintree * tree);
int fbintreeInsert(fbintree * tree, const ssize_t key, void * value);

treeNode * fbintreeSearch(fbintree * tree, const ssize_t key);


#endif /* fbintree.h */
