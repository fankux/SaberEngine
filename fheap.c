#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fheap.h"
#include "common.h"

/* private functions */
static void _AdjustAdd(fheap * h);
static void _AdjustRemove(fheap * h);
static int _MaxHeapCmp(const ssize_t a, const ssize_t b);
static int _MinHeapCmp(const ssize_t a, const ssize_t b);

#define LCHILD(n) ((n << 1) + 1) /* n is 0 base */
#define RCHILD(n) ((n << 1) + 2)
#define PARENT(n) ((n - 1) >> 1)

/* initilized a heap ,
** whether max-heap or min-heap ensured by type,
** type can't be changed from when heap constructed
** min-heap is default */
fheap * fheapCreate(const uint8_t type){
	fheap * new;
	if(NULL == (new = (fheap *)malloc(sizeof(fheap))))
		return NULL;

	if(NULL == (new->start = (fheapNode *)malloc(
					FHEAP_INITIAL_SIZE * sizeof(fheapNode)))){
		free(new);
		return NULL;
	}
	new->type = FHEAP_MIN;
	new->keyCmpFunc = _MinHeapCmp;
	if(type == FHEAP_MAX){
		new->type = FHEAP_MAX;
		new->keyCmpFunc = _MaxHeapCmp;
	}
	new->valDupFunc = NULL;
	new->valFreeFunc = NULL;
	new->free = FHEAP_INITIAL_SIZE;
	new->size = 0;
	
	return new;
}

fheapNode * fheapGet(fheap * h, ssize_t key, size_t * pos){
	size_t i;

	for(i = 0; i < h->size; ++i){
		if(h->start[i].key == key){
			if(pos) *pos = i;
			return &h->start[i];
		}
	}
	
	return NULL;
}

int fheapExist(fheap * h, ssize_t key){
	return (fheapGet(h, key, NULL) != NULL);
}

int fheapAdd(fheap * h, ssize_t key, void * value){
	fheapNode * p, * parent;
	ssize_t tkey;
	void * tp;
	size_t ps;
	int re;
	
	if (h->free < 1){
		ps = (h->size + 1) << 1; /* now 'ps' means new_size */

		parent = h->start; /* 'parent' just temp use */
		if(NULL == (h->start = (fheapNode *)
					malloc(ps * sizeof(fheapNode))))
			return FHEAP_FAILD;
		memcpy(h->start, parent, h->size * sizeof(fheapNode)); 
		free(parent);
		h->free = h->size + 2;
	}

	/*find if already exist*/
	if(fheapExist(h, key))
		return FHEAP_EXIST;
	
	/* insert into last free space, then adjust the heap */
	p = &h->start[h->size];
	p->key = key;
	fheapSetVal(h, p, value);
		
	/* this time, last one is the new one,
	** and not at the right position */
	ps = h->size; /* now ps means position */
	
	while((ssize_t)ps > 0){
		ps = PARENT(ps);
		parent = &h->start[ps];
		re = fheapCmpKey(h, parent->key, p->key);
		if(1 == re){ /* complete! */
			break;
		}else if(-1 == re){ /* exchange, p->key < parent->key */
			tkey = p->key;
			tp = p->value;
			p->key = parent->key;
			p->value = parent->value;
			parent->key = tkey;
			parent->value = tp;

			p = parent;
		}
	}

	++h->size;
	--h->free;

	return FHEAP_OK;
}

/* real remove operation function, remove a node at 'pos' */
int fheapRemoveAt(fheap * h, size_t pos){
	fheapNode * t, * p;
	ssize_t tkey;
	void * tp;
	size_t new_pos;
	int re;
	
	p = &h->start[pos];
	
	/* exchange it with last node */
	tkey = h->start[h->size - 1].key;
	tp = h->start[h->size - 1].value;
	h->start[h->size - 1].key = p->key;
	h->start[h->size - 1].value = p->value;
	p->key = tkey;
	p->value = tp;

	--h->size;
	++h->free;

	new_pos = pos;
    /* then adjust from p */
	while(pos < h->size){
		/* LCHILD must smaller then p.key when min-heap
	    ** while LCHILD must larger then p.key when max-heap */
		if(LCHILD(pos) < h->size){
			new_pos = LCHILD(pos);
			t = &h->start[new_pos];
		}else{
			break;
		}
		if(RCHILD(pos) < h->size &&
		   fheapCmpKey(h, h->start[RCHILD(pos)].key, tkey) == 1){
			new_pos = RCHILD(pos);
			t = &h->start[new_pos];
		}
		pos = new_pos;
		
		tkey = t->key;
		tp = t->value;
		t->key = p->key;
		t->value = p->value;
		p->key = tkey;
		p->value = tp;
	}
	
	return FHEAP_OK;
}

int fheapRemove(fheap * h, ssize_t key){
	fheapNode * p;
	size_t pos;

	if(NULL == (p = fheapGet(h, key, &pos)))
		return FHEAP_NONE;

	return fheapRemoveAt(h, pos);
}

/* type function */
static int _MaxHeapCmp(const ssize_t a, const ssize_t b){
	if(a > b)
		return 1;
	else if(a == b)
		return 0;
	else
		return -1;
}

static int _MinHeapCmp(const ssize_t a, const ssize_t b){
	if(a < b)
		return 1;
	else if(a == b)
		return 0;
	else
		return -1;
}

/* debug */
static void fheapInfo(fheap * h){
	size_t i;

	for(i = 0; i < h->size; ++i){
		printf("%d ", h->start[i].key);
	}
	printf("\nsize:%u,free:%u\n", h->size, h->free);
}

/* int main(void){ */
/* 	fheap * h = fheapCreate(1); */
/* 	if(!h) */
/* 		exit(EXIT_FAILURE); */

/* 	int re = fheapAdd(h, 5, NULL); */
/* 	re = fheapAdd(h, 1, NULL); */
/* 	re = fheapAdd(h, 3, NULL); */
/* 	re = fheapAdd(h, 20, NULL); */
/* 	re = fheapAdd(h, 15, NULL); */
/* 	re = fheapAdd(h, 30, NULL); */

/* 	fheapInfo(h); */
/* 	fheapAdd(h, 1, NULL); */
/*     //fheapRemove(h, 5); */
/* 	fheapInfo(h); */
	
/* 	return 0; */
/* } */
