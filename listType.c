#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "saber.h"

/* a, b both type of sobj */
int fdListCmpSobj(void * a, void * b){
	sobj * m = (sobj *)a;
	sobj * n = (sobj *)b;

	return sobjCmp(m, n);
}
/* a type of sobj(STRING), b type of (char *) */
int fdListCmpSobjStr(void * a, void * b){
	char * m = ((fstr *)((sobj *)a)->value)->buf;
	char * n = (char *)b;

	return strcmp(m, n);
}
void listObjSetCmpSobj(sobj * o){
	if(SABER_ENCODE_DLIST == o->encode)
		((fdList*)o->value)->CmpValFunc = fdListCmpSobj;
}
void listObjSetCmpSobjStr(sobj * o){
	if(SABER_ENCODE_DLIST == o->encode)
		((fdList*)o->value)->CmpValFunc = fdListCmpSobjStr;
}

int listObjPushHead(sobj * o, sobj * new){
	if(o->encode == SABER_ENCODE_DLIST){
		return fdListAddHead((fdList *)o->value, (void *)new);
	}else{
		return SABER_WRONGTYPE;
	}
}

int listObjPushTail(sobj * o, sobj * new){
	if(o->encode == SABER_ENCODE_DLIST){
		return fdListAddTail((fdList *)o->value, (void *)new);
	}else{
		return SABER_WRONGTYPE;
	}
}

sobj * listObjPopHead(sobj * o){
	if(o->encode == SABER_ENCODE_DLIST){
		fdListNode * p;
		if((p = fdListPopHead((fdList *)o->value)))
			return (sobj *)p->data;
		return NULL;
	}
	return NULL;
}

sobj * listObjPopTail(sobj * o){
	if(o->encode == SABER_ENCODE_DLIST){
		fdListNode * p;
		if((p = fdListPopTail((fdList *)o->value)))
			return (sobj *)p->data;
		return NULL;
	}
	return NULL;
}

int listObjInsert(sobj * o, sobj * value, sobj * pivot,
				  const uint8_t aorb){
	int re;
	if(SABER_ENCODE_DLIST == o->encode){
		re = fdListInsert((fdList *)o->value, value,
						  (void *)pivot, aorb);
		if(FDLIST_FAILD == re) return SABER_FAILD;
		else if(FDLIST_NONE == re) return SABER_NONE;
		return SABER_OK;
	}
	return SABER_WRONGTYPE;
}

int listObjSet(sobj * o, const size_t index, sobj * value){
	int re;
	if(o->encode == SABER_ENCODE_DLIST){
		re = fdListSet((fdList *)o->value, index, (void *)value);
		if(FDLIST_OUTRANGE == re) return SABER_FAILD;
		else if(FDLIST_OK == re) return SABER_OK;
	}else{
		return SABER_WRONGTYPE;
	}
}

sobj * listObjGetIndex(sobj * o, const size_t index){
	if(o->encode == SABER_ENCODE_DLIST){
		fdListNode * p = fdListGetIndex((fdList *)o->value, index);
		if(p) return (sobj *)p->data;
		return NULL;
	}else{
		return NULL;
	}
}

sobj * listObjGetRandom(sobj * o){
	sobj * new;

	if(o->encode == SABER_ENCODE_DLIST){
		if(!(new = sobjCreateList()))
			return NULL;

		new->value = (void *)fdListGetRandom((fdList *)o->value, 0);

		return new;
	}
	return NULL;
}

int listObjIndexOf(sobj * o, sobj * value){
	if(o->encode == SABER_ENCODE_DLIST){
		return fdListIndexOf((void *)value, NULL);
	}else{
		return SABER_WRONGTYPE;
	}
}

/* remove number of absolute value of 'count' node from 'pos',
** it's value equal to 'value'
** count > 0, search from left to right,
** count < 0, search from right to left,
** count = 0, remove all, 'pos' direction depend on value sign */
int listObjRemove(sobj * o, sobj * value,
				  const size_t pos, const size_t count){
	int re;
	if(o->encode == SABER_ENCODE_DLIST){
		re = fdListRemove((fdList *)o->value, (void *)value,
						  pos, count);
		if(re == FDLIST_NONE) return SABER_NONE;
		return re;
	}
	
	return SABER_WRONGTYPE;
}

int listObjRemoveAt(sobj * o, const size_t pos, sobj ** out_val){
	if(o->encode == SABER_ENCODE_DLIST){
		return fdListRemoveAt((fdList *)o->value, pos,
							  (void **)out_val);
	}else{
		return SABER_NONE;
	}
}

int listObjRemoveObj(sobj * o, sobj * value, sobj ** out_val){
	if(o->encode == SABER_ENCODE_DLIST){
		return fdListRemoveValue((fdList *)o->value, (void *)value,
								 (void **)out_val);
	}else{
		return SABER_NONE;
	}
}

listObjIter * listObjIterCreate(sobj * o, const uint8_t direct,
								const size_t start){
	listObjIter * iter;
	if(SABER_ENCODE_DLIST == o->encode){

		fdList * list = (fdList *)o->value;
		fdListNode * p;
	    size_t i = 0;
		
		if(start >= fdListLen(list)) return NULL;
		
		if(!(iter = malloc(sizeof(listObjIter))))
			return NULL;

		if(2 == direct) p = list->tail;
		else p = list->head;
		if(!p){
			listObjIterCancel(iter);
			return NULL;
		}
		
		
		iter->direct = direct;
		iter->rank = -1;
		iter->next = (sobj *)p->data;
		iter->inner = (void *)p;

		for(;i < start; ++i)/* be sured in bound */
			listObjIterNext(o, iter);
		
		return iter;
	}else
		return NULL;
}

sobj * listObjIterNext(sobj * o, listObjIter * iter){
	sobj * next;
	if(SABER_ENCODE_DLIST == o->encode){
		/* no more node,return null */
		if(!iter->next){
			listObjIterCancel(iter);
			return NULL;
		}
		next = iter->next;
		
		++iter->rank;
		if(2 == iter->direct)
			iter->inner = ((fdListNode *)iter->inner)->prev;
		else iter->inner = ((fdListNode *)iter->inner)->next;
		
		if(iter->inner)
			iter->next = ((fdListNode*)iter->inner)->data;
		else iter->next = NULL;
				
		return 	next;
	}
	return NULL;
}

void listObjIterCancel(listObjIter * iter){
	if(!iter) return;
	free(iter);
}

void listObjIterRewind(sobj * o, listObjIter * iter){
	if(SABER_ENCODE_DLIST == o->encode){
			iter->rank = -1;
		if(2 == iter->direct)
			iter->next = (sobj*)((fdList*)o->value)->tail->data;
		else iter->next = (sobj*)((fdList*)o->value)->head->data;
	}
}

/***************************** command **********************************/
/* internal function to push a value, direct 1 means left,
** 2 means right  */
static int PushCmd(sclnt * client, int direct){
	/* numbers of args unmatched */
	AssertSResultReturn(client, client->argc, 2, err_arg);
	
    /* parse the key */
	fstr * key = NULL;
	sobj * obj = NULL, * value = NULL;
	char * buf, * start, * next;
	size_t sec_len;
	int re, counter = 0;
	
	buf = client->argv[0];
	key = fstrCreate(buf);
	AssertResultReturn(client, key, NULL, err_mem);

	/* make sure this key in table */
	if(!(obj = stabGet(client->table, (void *)key->buf))){
		obj = sobjCreateList();
		AssertResultReturn(client, obj, NULL, err_mem);
		listObjSetCmpSobjStr(obj);
		re = stabAdd(client->table, key->buf, (void *)obj);
		AssertResultGoto(client, re, SABER_FAILD, faild, err_key);
	}else fstrFree(key);
    
	/* parse the value */
	buf = client->argv[1];
	while((re = ValueSplit(buf, &sec_len, &start, &next)) > 0){
		value = sobjCreateStringLen(start, sec_len);
		if(!value) continue;

		if(2 == direct) re = listObjPushTail(obj, value);
		else re = listObjPushHead(obj, value);
		AssertResultGoto(client, re, SABER_WRONGTYPE,
						 err_type, err_val);
		if(FDLIST_OK != re){
			sobjFree(value);
			continue;
		}
		buf = next;
		++counter;
	}
	SetResultStr(client, server.share->ok);
	AddResultInt(client, counter);
	
	return 1;

err_key:
	sobjFree(obj);
	fstrFree(key);
	return 0;
	
err_val:
	sobjFree(value);
	return 0;
}

/* LPUSH key value1 "value2" ... */
int LPushCmd(sclnt * client){
	return PushCmd(client, 1);
}
/* RPUSH key value1 "value2" ... */
int RPushCmd(sclnt * client){
	return PushCmd(client, 2);
}
static int PopCmd(sclnt * client, int direct){
	sobj * obj, * re;
	size_t n;
	/* numbers of args unmatched */
	AssertUResultReturn(client, client->argc,
						client->cmd->arity, err_arg);

	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->type, SABER_LIST, err_type);
	
	if(2 == direct) re = listObjPopTail(obj);
	else re = listObjPopHead(obj);

	if(!re) n = 0;
	else n = 1;

	SetResultStr(client, server.share->ok);
	AddResultInt(client, n);
	if(n > 0){
		AddResultStr(client, ((fstr *)re->value)->buf);
		AddResultStr(client, server.share->crlf);
	}

	return 1;
}
/* syntax: LSET key index new_value
** index must be number that indicates list key */
int LSetCmd(sclnt * client){
	AssertSResultReturn(client, client->argc, 2, err_arg);

	sobj * obj, * value;
	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->type, SABER_LIST, err_type);

	int re;
	size_t sec_len, index;
	char val_buf[128] = "\0";
	char * start, * next, * buf = client->argv[1];
	/* parse index, index must be number */
	re = ValueSplit(buf, &sec_len, &start, &next);
	AssertUResultReturn(client, re, 2, err_arg);
	cpystr(val_buf, start, sec_len);
	index = atoi(val_buf);

	buf = next;
	/* parse value to be set */
	re = ValueSplit(buf, &sec_len, &start, NULL);
	AssertSEResultReturn(client, re, 0, err_arg);
	value = sobjCreateStringLen(start, sec_len);
	AssertResultReturn(client, value, NULL, err_mem);

	re = listObjSet(obj, index, value);
	AssertResultReturn(client, re, SABER_OK, ok);
	AssertResultReturn(client, re, SABER_OUTRANGE, err_out_range);

	sobjFree(value);

	return 1;
}
/* LPOP key */
int LPopCmd(sclnt * client){
	return PopCmd(client, 1);
}
/* RPOP key */
int RPopCmd(sclnt * client){
	return PopCmd(client, 2);
}
/* LLEN key */
int LLenCmd(sclnt * client){
	sobj * obj;
	int re;
	/* numbers of args unmatched */
	AssertUResultReturn(client, client->argc,
						client->cmd->arity, err_arg);
	
	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->type, SABER_LIST, err_type);
	
	re = listObjLen(obj);
	AssertResultReturn(client, re, SABER_WRONGTYPE, err_type);

	SetResultStr(client, server.share->ok);
	AddResultInt(client, re);
}

/* syntax: LINS key value BRFORE|AFTER pivot
** insert a value before or after value pivot */
int LInsCmd(sclnt * client){
	sobj * obj, * value = NULL, * pivot = NULL;

	/* numbers of args unmatched */
	AssertSResultReturn(client, client->argc, 2, err_arg);
		
	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->type, SABER_LIST, err_type);

	int re, flag;
	size_t sec_len;
	char val_buf[128] = "\0";
	char * start, * next, * buf = client->argv[1];
	
	/* parse value */
	re = ValueSplit(buf, &sec_len, &start, &next);
	AssertSEResultReturn(client, re, 0, err_arg);
	value = sobjCreateStringLen(start, sec_len);
	AssertResultReturn(client, value, NULL, err_mem);
	buf = next;
	
    /* parse position flag, BEFORE or AFTER, case insensetive */
	re = ValueSplit(buf, &sec_len, &start, &next);
	AssertSEResultGoto(client, re, 0, err_arg, err);
	cpystr(val_buf, start, sec_len);
	if(!strcasecmp(val_buf, "BEFORE")) flag = 1;
	else if(!strcasecmp(val_buf, "AFTER")) flag = 2;
	else{
		SetResultStr(client, server.share->err_arg);
		goto err;
	}
	buf = next;
	
    /* parse pivot */
	re = ValueSplit(buf, &sec_len, &start, NULL);
	AssertSEResultGoto(client, re, 0, err_arg, err);
	cpystr(val_buf, start, sec_len);
	
	re = listObjInsert(obj, value, (void *)val_buf, flag);
	AssertResultGoto(client, re, SABER_FAILD, faild, err);
	AssertResultGoto(client ,re, SABER_NONE, none, err);

	SetResultStr(client, server.share->ok);
		
	return 1;

err:
	sobjFree(value);
	return 0;
}

/* syntax: LREM key pos count value
** remove n node from 'pos' which value equal to 'value' 
** pos: the pos start search, must be number
** count: the remove count, must be number
** value: value to be search */
int LRemCmd(sclnt * client){
	sobj * obj;
    /* numbers of args unmatched */
	AssertSResultReturn(client, client->argc, 2, err_arg);

	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->type, SABER_LIST, err_type);
	
	int re;
	size_t sec_len, index, count;
	char val_buf[128] = "\0";
	char * start, * next, * buf = client->argv[1];
	/* parse index, must be number */
	re = ValueSplit(buf, &sec_len, &start, &next);
	AssertUResultReturn(client, re, 2, err_arg);
	cpystr(val_buf, start, sec_len);
	index = atoi(val_buf);
	buf = next;
	/* parse count, must be number */
	re = ValueSplit(buf, &sec_len, &start, &next);
	AssertUResultReturn(client, re, 2, err_arg);
	cpystr(val_buf, start, sec_len);
	count = atoi(val_buf);
	buf = next;
	/* parse value */
	re = ValueSplit(buf, &sec_len, &start, NULL);
	AssertSEResultReturn(client, re, 0, err_arg);
	cpystr(val_buf, start, sec_len);

	re = listObjRemove(obj, (void *)val_buf, index, count);
	
	SetResultStr(client, server.share->ok);
	AddResultInt(client, re);

	return 1;
}

/* get the value at the postion of index
** syntax: LIDX key index
** index must be number */
int LIdxCmd(sclnt * client){
	sobj * obj;
	/* numbers of args unmatched */
	AssertSResultReturn(client, client->argc, 2, err_arg);

	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->type, SABER_LIST, err_type);
	
	int re;
	size_t sec_len, index;
	char val_buf[128] = "\0";
	char * start;
	/* parse index, index must be number */
	re = ValueSplit(client->argv[1], &sec_len, &start, NULL);
	AssertUResultReturn(client, re, 2, err_arg);
	cpystr(val_buf, start, sec_len);
	index = atoi(val_buf);

	obj = listObjGetIndex(obj, index);
	AssertResultReturn(client, obj, NULL, none);
	
	SetResultStr(client, server.share->ok);
	AddResultStr(client, ((fstr *)obj->value)->buf);
	AddResultStr(client, server.share->crlf);

	return 1;
}

/* syntax: LRNG key start stop direct
** get a bulk of values from start to stop
** start: index where begin scanning, must be number,
** stop: index where stop scanning, must be number,
** direct is optional, must be number,
** the actual number of values may be smaller
** than (stop - start +1), even 0 */
int LRngCmd(sclnt * client){
	sobj * obj;
	/* numbers of args unmatched */
	AssertSResultReturn(client, client->argc, 2, err_arg);

	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->type, SABER_LIST, err_type);
	
	int re, l, h, direct = 1;
	size_t sec_len;
	char val_buf[128] = "\0";
	char * start, *next, * buf = client->argv[1];
	/* parse start, must be number */
	re = ValueSplit(buf, &sec_len, &start, &next);
	AssertUResultReturn(client, re, 2, err_arg);
	cpystr(val_buf, start, sec_len);
	l = atoi(val_buf);
	buf = next;
	/* parse stop index must be number */
	re = ValueSplit(buf, &sec_len, &start, &next);
	AssertUResultReturn(client, re, 2, err_arg);
	cpystr(val_buf, start, sec_len);
	h = atoi(val_buf);
	buf = next;
	/* parse direct must be number, optional */
	re = ValueSplit(buf, &sec_len, &start, NULL);
	if(2 == re){
		cpystr(val_buf, start, sec_len);
		direct = atoi(val_buf);
	}

	/* scanning value */
	int count, i = 0;
	sobj * node;
	listObjIter * iter;
	if(l > h){
		SetResultStr(client, server.share->err_arg);
		AddResultStr(client, "start must smaller than stop\r\n");
		return 0;
	}
	count = h - l + 1;
	printf("count:%d; l:%d; direct: %d\n", count, l, direct);
	iter = listObjIterCreate(obj, direct, l);
	EmptyResult(client);
	while(iter && i < count &&
		  (node = listObjIterNext(obj, iter))){

		AddResultStr(client, ((fstr *)node->value)->buf);
		AddResultStr(client, server.share->crlf);
		
		++i;
	}
	if(node)
		listObjIterCancel(iter);
	
	if(!i) SetResultInt(client, 0);
	else InsResultInt(client, i, 0);

	return 1;
}
