/************
**balance tree
**basically use and implenty
*************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "common.h"
#include "fbintree.h"

/* private functions */
static void _LRotate(treeNode ** p);
static void _RRotate(treeNode ** p);
static void _LRRotate(treeNode ** p);
static void _RLRotate(treeNode ** p);
static int _Balance(treeNode ** p);
static int _Insert(treeNode ** p,const ssize_t key,void * value);
static int _Delete(treeNode ** p, const ssize_t key, void * r);

fbintree * fbintreeCreate(void){
	fbintree * tree;
	if(NULL == (tree = (fbintree *)malloc(sizeof(fbintree)))) return NULL;

	tree->root = NULL;
	tree->num  = 0;
	
	return tree;
}

void fbintreeFree(fbintree * tree){
	
}

static void _LRotate(treeNode ** p){
    treeNode * root, * pivot;
    root = *p;
    pivot = root->right;
    
    root->right = pivot->left;
    pivot->left = root;
    
    *p = pivot;
}

static void _RRotate(treeNode ** p){
    treeNode * root, * pivot;
    root = *p;
    pivot = root->left;
    
    root->left = pivot->right;
    pivot->right = root;
    
    *p = pivot;
}

static void _LRRotate(treeNode ** p){
    _LRotate(&(*p)->left);
    _RRotate(p);
}

static void _RLRotate(treeNode ** p){
    _RRotate(&(*p)->right);
    _LRotate(p);
}

/* chosing the balance method
** and adjusting balance factors */
static int _Balance(treeNode ** p){    
    int c_bf;
    switch((*p)->bf){
    case 2:/* choising rotate type */
        switch((*p)->left->bf){
        case 0:/* R */
            (*p)->bf = 1;
            (*p)->left->bf = -1;
            _RRotate(p);
            return 1;
        case 1://R
            (*p)->bf = (*p)->left->bf = 0;
            _RRotate(p);
            return 1;
        case -1://LR
            c_bf = (*p)->left->right->bf;
            if(c_bf == 0) (*p)->bf = (*p)->left->bf = 0;
            else if(c_bf == 1){
                (*p)->bf = -1;
                (*p)->left->bf = 0;
            }else if(c_bf == -1){
                (*p)->bf = 0;
                (*p)->right->bf = 1;
            }
            _LRRotate(p);
            return 1;
        default://AVL destroyed
            return 0;
        }
        break;
    case -2:
        switch((*p)->right->bf){
        case 0://L
            (*p)->bf = -1;
            (*p)->right->bf = 1;
            _LRotate(p);
            return 1;
        case -1://L
            (*p)->bf = (*p)->right->bf = 0;
            _LRotate(p);
            return 1;
        case 1://RL
            c_bf = (*p)->right->left->bf;
            if(c_bf == 0) (*p)->bf = (*p)->right->bf = 0;
            else if(c_bf == 1){
                (*p)->bf = 0;
                (*p)->right->bf = -1;
            }else if(c_bf == -1){
                (*p)->bf = 1;
                (*p)->right->bf = 0;
            }
            _RLRotate(p);
            return 1;
        default://AVL destroyed
            return 0;
        }
        break;
		/* AVL tree has been destroyed
		** something needed to rebuild struct */
    default:
        return 0;
    }
}
/* real insert action */
static int _Insert(treeNode ** p,const ssize_t key,void * value){
    if(*p == NULL){//right position achieved
        treeNode * new = (treeNode *)malloc(sizeof(treeNode));
        if(!new) return -1;//mem faild
        
        new->left = new->right = NULL;
        new->key = key;
        new->value = value;
        new->bf = 0;
        
        *p = new;
        return 1;
    }
    int c_bf; /* child->bf */
    /* find the right position before the insertion point */
    if(key < (*p)->key){/* left */
        switch(_Insert(&(*p)->left, key, value)){
        case 1: /* inserted as left leaf */
            ++(*p)->bf;
            return 2;
        case 2: /* have inserted in it's children,someone */ 
            c_bf = (*p)->left->bf;
            if(c_bf == 0) return 0;/* p->left's depth didn't change */
            else if(c_bf == 1 || c_bf == -1){/* p->left's depth increased */
                if(++(*p)->bf == 2){/* balance action, then work done */
                    _Balance(p);
                    return 0;
                }
                return 2;
            }else return -1;
        case 0: return 0; /* inserted,already balanced */
        case -2: return -2;/* key already exist, no action done */ 
        default: return -1; /* -1 returned, mem failed, no action done */   
        }
    }else if(key > (*p)->key){/* right */
        switch(_Insert(&(*p)->right, key, value)){
        case 1: /* inserted as right leaf */
            --(*p)->bf;
            return 2;
        case 2:
            c_bf = (*p)->right->bf;
            if(c_bf == 0) return 0;
            else if(c_bf == 1 || c_bf == -1){
                if(--(*p)->bf == -2){
                    _Balance(p);
                    return 0;
                }
                return 2;
            }else return -1;
         case 0: return 0;
         case -2: return -2;
         default: return -1;
        }
    }else{//this key already exist
        return -2;
    }
}

/* insert interface
   if success, return 1,
   if key already exist, return 2,
   if faild, return 0 */
int fbintreeInsert(fbintree * tree, const ssize_t key, void * value){
    int result = _Insert(&tree->root, key, value);
    if(result == -1) return FBINTREE_NONE;
    else if(result == -2) return FBINTREE_EXIST;
    else return FBINTREE_OK;
}

static int _Delete(treeNode ** p, const ssize_t key, void * r){
    int c_bf;
    if((*p) == NULL){/* search faild */
        return -1;
    }
    if(key < (*p)->key){
        switch(_Delete(&(*p)->left, key, r)){
        case  0: return 0;/* already balance */
        case  1:/* just delete a leaf node */
            --(*p)->bf;            
            return 2;
        case  2:
            c_bf = (*p)->left->bf;
            if(c_bf == 0){/* depth decresed */
                if(--(*p)->bf == -2){
                    _Balance(p);
                    return 0;
                }
                return 2;
            }else if(c_bf == 1 || c_bf == -1)/* depth didn't change */
				return 0;
            else return -1;
		case -1: return -1;//faild
        }
    }else if(key > (*p)->key){
        switch(_Delete(&(*p)->right, key, r)){
        case  0: return 0;
        case  1:
            ++(*p)->bf;
            return 2;
        case 2:
            c_bf = (*p)->right->bf;
            if(c_bf == 0){
                if(++(*p)->bf == 2){
                    _Balance(p);
                    return 0;
                }
                return 2;
            }else if(c_bf == 1 || c_bf == -1) return 0;
            else return -1;
		case -1: return -1;
        }
    }else{/* got it */
        treeNode t;
        treeNode * m, * q;
        r = (*p)->value;/* get the value */
		
        if((*p)->bf == 1){
			/* find the p->left's maximum childnode
		    ** link p->right to it,then adjust the balanced
			** then replace p with p->left, free old p's memory */
			m = (*p)->left;
			while(m->right) m = m->right;
			m->right = (*p)->right;

			t.key = (*p)->key;
			t.value = (*p)->value;
			(*p)->key = m->key;
			(*p)->value = m->value;
			m->key = t.key;
			m->value = t.value;
			
			return _Delete(p, key, r);
		}else if((*p)->bf == 0){
            if((*p)->left){/* same action as (*p)->bf == 1 */
				m = (*p)->right;
				while(m->left) m = m->right;
				m->left = (*p)->left;
				q = *p;
				m = (*p)->right;
				if((*p)->left)
					if(++m->bf == 2) _Balance(&m);
				*p = m;
				free(q);
				return 2;			
			}else{/* left node */
				q = *p;
				*p = NULL;
				free(q);
				return 1;
			}
        }else if((*p)->bf == -1){
			m = (*p)->right;
			while(m->left) m = m->left;
			m->left = (*p)->left;

			t.key = (*p)->key;
			t.value = (*p)->value;
			(*p)->key = m->key;
			(*p)->value = m->value;
			m->key = t.key;
			m->value = t.value;
			
			return _Delete(p, key, r);
        }else return -1;    
    }
}

treeNode * fbintreeSearch(fbintree * tree, const ssize_t key){
    treeNode * p = tree->root;
    
    while(p){
        if(key < p->key) p = p->left;
        else if(key > p->key) p = p->right;
        else return p; //got it;
    }
    
    return NULL; //nothing, search faild;
}

/*
** out put inorder tree,for debug using
*/
static void InorderTraverse(treeNode * root){
    if(root == NULL){
        return;
    }
    printf("%d.%d(",root->key,root->bf);
    if(root->left){
        InorderTraverse(root->left);
    }
    printf(",");
    if(root->right){
        InorderTraverse(root->right);
    }
    printf(")");
}

/* int main(){ */
/*     int data[] = {8,5,1}; */
/*     fbintree tree; */
/*     treeNode root,a,b,c; */
/*     root.left = root.right = NULL; */
/*     a.left = a.right = NULL; */
/*     b.left = b.right = NULL; */
/*     c.left = c.right = NULL; */
/*     root.bf = a.bf = b.bf = c.bf = 0; */
    
/*     tree.root = NULL; */

/*     printf("\n"); */
/*     int f = -1; */
/*     for(int i = 0; i <= 10; ++i){ */
/*         int n = rand()%100; */
/*         int result = fbintreeInsert(&tree, n, (void *)data); */
/*         /\* if(result == 1){ *\/ */
/*         /\*     printf("%d success\n", n); *\/ */
/*         /\* }else if(result == 2){ *\/ */
/*         /\*     printf("%d already exist\n", n); *\/ */
/*         /\* }else{ *\/ */
/*         /\*     printf("%d faild,mem error\n", n); *\/ */
/*         /\* } *\/ */
/*     } */
/*     InorderTraverse(tree.root); */
/*     void * r; */
    
/*     printf("\n%d", _Delete(&tree.root, 15, r)); */
/*     InorderTraverse(tree.root);    */
    
/*     printf("\n"); */
    
/*     return 0; */
/* } */




