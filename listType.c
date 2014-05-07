#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "saber.h"

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

int listObjSet(sobj * o, const size_t index, void * value){
	int re;
	if(o->encode == SABER_ENCODE_DLIST){
		re = fdListSet((fdList *)o->value, index, value);
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

int listObjIndexOf(sobj * o, void * value){
	if(o->encode == SABER_ENCODE_DLIST){
		return fdListIndexOf((fdList *)o->value, NULL);
	}else{
		return SABER_WRONGTYPE;
	}
}

/* remove number of absolute value of 'count' node from 'pos',
** it's value equal to 'value'
** count > 0, search from left to right,
** count < 0, search from right to left,
** count = 0, remove all, 'pos' direction depend on value sign */
int listObjRemove(sobj * o, void * value,
				  const size_t pos, const size_t count){
	int re;
	if(o->encode == SABER_ENCODE_DLIST){
		re = fdListRemove((fdList *)o->value, value, pos, count);
		if(re == FDLIST_NONE) return SABER_NONE;
		return re;
	}

	return SABER_WRONGTYPE;
}

int listObjRemoveAt(sobj * o, const size_t pos, void ** out_val){
	if(o->encode == SABER_ENCODE_DLIST){
		return fdListRemoveAt((fdList *)o->value, pos, out_val);
	}else{
		return SABER_NONE;
	}
}

int listObjRemoveObj(sobj * o, void * value, void ** out_val){
	if(o->encode == SABER_ENCODE_DLIST){
		return fdListRemoveValue((fdList *)o->value, value, out_val);
	}else{
		return SABER_NONE;
	}
}

listObjIter * listObjIterCreate(sobj * o, const uint8_t direct,
								const size_t start_pos){

}

sobj * listObjIterNext(listObjIter * iter){
	
}

void listObjIterCancel(listObjIter * iter){
	
}

void listObjIterRewind(sobj * o, listObjIter * iter){
	
}

/***************************** command **********************************/
/* internal function to push a value, direct 1 means left,
** 2 means right  */
static void PushCmd(sclnt * client, int direct){
	/* numbers of args unmatched */
	if(client->argc < 2){
		SetResultStr(client, server.share->err_arg);
		return;
	}
	
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
		AssertResultGoto(client, re, SABER_WRONGTYPE, err_type, err_val);
		if(FDLIST_OK != re){
			sobjFree(value);
			continue;
		}
		buf = next;
		++counter;

		printf("pushed value:%s\n", ((fstr *)value->value)->buf);
	}
	SetResultStr(client, server.share->ok);
	AddResultInt(client, counter);
	
	return;

err_key:
	sobjFree(obj);
	fstrFree(key);
	return;
	
err_val:
	sobjFree(value);
	return;
}

/* LPUSH key value1 "value2" ... */
void LPushCmd(sclnt * client){
	PushCmd(client, 1);
}
/* RPUSH key value1 "value2" ... */
void RPushCmd(sclnt * client){
	PushCmd(client, 2);
}
static void PopCmd(sclnt * client, int direct){
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
}
/* syntax: LSET key index new_value
** index must be number that indicates list key */
void LSetCmd(sclnt * client){
	AssertSResultReturn(client, client->argc, 2, err_arg);

	sobj * obj;
	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->type, SABER_LIST, err_type);

	int re;
	size_t sec_len, index;
	char val_buf[128] = "\0";
	char * start, * next, * buf = client->argv[1];
	fstr * value;
	/* parse index, index must be number */
	re = ValueSplit(buf, &sec_len, &start, &next);
	AssertUResultReturn(client, re, 2, err_arg);
	cpystr(val_buf, start, sec_len);
	index = atoi(val_buf);

	buf = next;
	/* parse value to be set */
	re = ValueSplit(buf, &sec_len, &start, NULL);
	AssertSEResultReturn(client, re, 0, err_arg);
	value = fstrCreateLen(start, sec_len);
	AssertResultReturn(client, value, NULL, err_mem);

	printf("index:%u, newly value is:%s\n", index, value->buf);
	
	re = listObjSet(obj, index, (void *)value->buf);
	AssertResultReturn(client, re, SABER_OK, ok);
	AssertResultReturn(client, re, SABER_OUTRANGE, err_out_range);

	fstrFree(value);
}
/* LPOP key */
void LPopCmd(sclnt * client){
	PopCmd(client, 1);
}
/* RPOP key */
void RPopCmd(sclnt * client){
	PopCmd(client, 2);
}
/* LLEN key */
void LLenCmd(sclnt * client){
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
void LInsCmd(sclnt * client){}
void LRemCmd(sclnt * client){}

/* get the value at the postion of index
** syntax: LIDX key index
** index must be number */
void LIdxCmd(sclnt * client){
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
}
void LRngCmd(sclnt * client){}
