#ifndef FSTACK_H
#define FSTACK_H

#include <stdio.h>
#include <stdlib.h>

#define FSTACKT_SIZE 512
typedef struct fstack{
    size_t len;
    int top;
    void * stack[];
}fstack;

fstack * fstackCreate(size_t size){
    fstack * p = (fstack *)malloc(sizeof(fstack) + sizeof(void *) * size); 
    p->len = size;
    p->addr_start = (int)(&p->top) + sizeof(void *);
    p->top = (void *)p->addr_start;
    
    return p;
}
void fstackPush(fstack * stack, void * p){
    void * addr = &stack->top + 2;
    printf("%d\n",(int)addr);
}

void fstackInfo(fstack * p){
    printf("length:%d;start addr:%d;end addr:%d;Top addr:%d\n", 
            p->len, (int)(p->addr_start),
            (int)(p->addr_start + p->len * sizeof(void *)),
            (int)(p->top));
}   

int main(){
    fstack * stack = fstackCreate(10);
    fstackInfo(stack);
    fstackPush(stack, NULL);
}
#endif
