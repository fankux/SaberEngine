#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "saber.h"


/* a,b both sobj */
static int fdictCmpSobj(void * a, void * b){
	return sobjCmp((sobj*)a, (sobj*)b);
}
/* a is sobj(ENCODE_STRING), b is (char *) */
static int fdictCmpSobjStr(void * a, void * b){
	char * m = ((fstr*)((sobj*)a)->value)->buf;
	return strcmp(m, (char *)b);
}
/* a is sobj(ENCODE_INTGER), b is int32 */
static int fdictCmpSobjInt(void * a, void * b){
	int32_t m = (int32_t)((sobj*)a)->value;
	int32_t n = (int32_t)b;

	return m < n? -1:(m > n? 1: 0);
}
static unsigned int fdictHashSobjInt(void * o){
	if(((sobj*)o)->encode == SABER_ENCODE_INTEGER)
		return _IntHash(((sobj*)o)->value);
	else
		return _GenHash(((fstr*)((sobj*)o)->value)->buf);
}
static unsigned int fdictHashSobjStr(void * o){
	return _GenHash(((fstr*)((sobj*)o)->value)->buf);
}

/* key is string, and value will be copied */
#define hashObjSetCmpSobjStr(o){				\
	if(SABER_ENCODE_DICT == (o)->encode){		\
		fdict * dict = (fdict*)(o)->value;		\
		dict->type->CmpKeys = fdictCmpSobjStr;	\
	}											\
}
#define hashObjSetCmpSobjInt(o){				\
	if(SABER_ENCODE_DICT == (o)->encode){		\
		fdict * dict = (fdict*)(o)->value;		\
		dict->type->CmpKeys = fdictCmpSobjInt;	\
	}											\
}
#define hashObjSetCmpSobj(o){					\
	if(SABER_ENCODE_DICT == (o)->encode){		\
		fdict * dict = (fdict*)(o)->value;		\
		dict->type->CmpKeys = fdictCmpSobj;		\
	}											\
}
#define hashObjSetKeyStr(o){					\
	if(SABER_ENCODE_DICT == (o)->encode){		\
		fdict * dict = (fdict*)(o)->value;		\
		dict->type->HashFunc = fdictHashSobjStr;\
		dict->type->DupKey = _StrDup;			\
		dict->type->DesKey = free;				\
	}											\
}
/* key is int */
#define hashObjSetKeyInt(o){					\
	if(SABER_ENCODE_DICT == (o)->encode){		\
		fdict * dict = (fdict*)(o)->value;		\
		dict->type->HashFunc = fdictHashSobjInt;\
		dict->type->DupKey = NULL;				\
		dict->type->DesKey = NULL;				\
	}											\
}
/* key is pinter */
#define hashObjSetKeyLnk(o){					\
	if(SABER_ENCODE_DICT == (o)->encode){		\
		fdict * dict = (fdict*)(o)->value;		\
		dict->type->HashFunc = fdictHashSobjStr;\
		dict->type->DupKey = NULL;				\
		dict->type->DesKey = NULL;				\
	}											\
}
#define hashObjSetValStr(o){					\
	if(SABER_ENCODE_DICT == (o)->encode){		\
		fdict * dict = (fdict*)(o)->value;		\
		dict->type->SetVal = _StrSet;			\
		dict->type->DesVal = free;				\
	}											\
}
#define hashObjSetValSobj(o){					\
	if(SABER_ENCODE_DICT == (o)->encode){		\
		fdict * dict = (fdict*)(o)->value;		\
		dict->type->SetVal = NULL;				\
		dict->type->DesVal = NULL;				\
	}											\
}

int hashObjAdd(sobj * o, sobj * key, sobj * value){
	int re;
	if(SABER_ENCODE_DICT == o->encode){
		re = fdictAdd((fdict*)o->value, (void*)key, (void*)value);
		if(FDICT_EXIST == re) return SABER_EXIST;
		else if(FDICT_FAILD == re) return SABER_FAILD;
		return SABER_OK;
	}else
		return SABER_WRONGTYPE;
}

/* if there is no "key", would add a new one,
** else old node will be released, and the key,
** which type of sobj would be freed */
int hashObjSet(sobj * o, sobj * key, sobj * value){
	if(SABER_ENCODE_DICT == o->encode){
		fdict * dict = (fdict*)o->value;
		dictNode * p = fdictSearch(dict, (void*)key);
		int re;
		if(p){/* release old value, then add new */
			sobjFree((sobj*)p->value.val);
			p->value.val = value;

			sobjFree(key);  /* free field value */
			return SABER_OK;
		}else{/* add a new one */
			re = fdictAdd(dict, key, value);
			if(FDICT_FAILD == re) return SABER_FAILD;
			else if(FDICT_EXIST == re) return SABER_EXIST;
			else return SABER_OK;
		}
	}
	return SABER_WRONGTYPE;
}

int hashObjSetXst(sobj * o, sobj * key, sobj * value){
	if(SABER_ENCODE_DICT == o->encode){
		fdict * dict = (fdict*)o->value;
		dictNode * p = fdictSearch(dict, (void*)key);
		if(p){/* release old value, then add new */
			sobjFree((sobj*)p->value.val);
			p->value.val = value;

			return SABER_OK;
		}else/* not exist */
			return SABER_NONE;
	}
	return SABER_WRONGTYPE;
}

sobj * hashObjGet(sobj * o, sobj * key){
	if(SABER_ENCODE_DICT == o->encode){
		dictNode * p = fdictSearch((fdict*)o->value, (void*)key);
		if(p) return p->value.val;
		return NULL;
	}else
		return NULL;
}
int hashObjRemove(sobj * o, sobj * key){
	int re;
	if(SABER_ENCODE_DICT == o->encode){
		re = fdictRemove((fdict*)o->value, (void *)key);
		if(FDICT_NONE == re) return SABER_NONE;
		else return SABER_OK;
	}else
		return SABER_WRONGTYPE;
}
int hashObjExist(sobj * o, void * key){
	int re;
	if(SABER_ENCODE_DICT == o->encode){
		return
			NULL != fdictSearch((fdict*)o->value, (void*)key);
	}else
		return SABER_WRONGTYPE;
}
int hashObjLenRegEx(sobj * o, char * reg){
	if(SABER_ENCODE_DICT == o->encode){
		fdict * dict = (fdict*)o->value;
		dictIter * iter = fdictIterCreate(dict, 0);
		sobj * key;
		char * key_str, *s = reg;
		int flag, n = 0;

		if(!iter) return -1;
		do{
			key = (sobj*)iter->current->key;
			key_str = ((fstr*)key->value)->buf;
			if(key->encode == SABER_ENCODE_STRING){
				flag = 1;
				s = reg;
				while(*s && *key_str){
					if(*s++ != *key_str++){
						flag = 0;
						break;
					}
				}
				if(flag && *key_str == ':') ++n;
			}
			iter = fdictIterNext(dict, iter);
		}while(iter);
		
		return n;
	}else
		return -1;
}
/*********** hash object commands implementions ************/
/* syntax: HSET key field1 value1 field2 value2 ...
** if no field exist, it will add new one,
** even if errors occur,
** there still may some "field-value" pairs successfully added
** before the pointe where errors occured */
static int SetCmd(sclnt * client, const int type){
    /* numbers of args unmatched */
	AssertSResultReturn(client, client->argc, 2, err_arg);
	
    /* parse the key */
	fstr * key = NULL;
	sobj * obj = NULL;
	int re;
	
	/* key */
	key = fstrCreate(client->argv[0]);
	AssertResultReturn(client, key, NULL, err_mem);
	/* make sure this key in table */
	if(!(obj = stabGet(client->table, (void *)key->buf))){
		obj = sobjCreateHash();
		hashObjSetCmpSobj(obj);
		AssertResultReturn(client, obj, NULL, err_mem);
		re = stabAdd(client->table, key->buf, (void *)obj);
		AssertResultGoto(client, re, SABER_FAILD, faild, err_key);
	}else fstrFree(key);
	/* judge type */
	AssertUResultReturn(client, obj->type, SABER_HASH, err_type);

	sobj * field, * value;
	char val_buf[SABER_RECVBUF_MAX] = "\0";
	char * buf, * start, * next;
	size_t sec_len;
	int counter = 0;

	buf = client->argv[1];
	
	while(1){
		field = value = NULL;
		
		re = ValueSplit(buf, &sec_len, &start, &next);
		buf = next;
		if(re <= 0) break;
		else if(1 == re){/* string as field */
			field = sobjCreateStringLen(start, sec_len);
			hashObjSetKeyLnk(obj);
		}else if(2 == re){/* number as field */
			cpystr(val_buf, start, sec_len);
			field = sobjCreateStringInt(atoll(val_buf));
			hashObjSetKeyInt(obj);
		}
		if(!field) break;
		
		/* parse the value */
		re = ValueSplit(buf, &sec_len, &start, &next);
		buf = next;
		
		if(re <= 0){
			sobjFree(field);/* field and value umatched */
			break;
		}else if(1 == re){
			value = sobjCreateStringLen(start, sec_len);
		}else if(2 == re){
			cpystr(val_buf, start, sec_len);
			value = sobjCreateStringInt(atoll(val_buf));
		}
		if(!value){
			sobjFree(field);
			break;
		}

		if(2 == type) re = hashObjSetXst(obj, field, value);
		else re = hashObjSet(obj, field, value);
		
		if(re != SABER_OK){
			sobjFree(field);
			sobjFree(value);
			continue;
		}

		++counter;
	}
	SetResultInt(client, counter);
	
	return 1;	
	
err_key:
	sobjFree(obj);
	return 0;
}
/* syntax: HGET key field1 field2 ...field(n) */
inline int HSetCmd(sclnt * client){
	return SetCmd(client, 1);
}
inline int HSetXstCmd(sclnt * client){
	return SetCmd(client, 2);
}
int HGetCmd(sclnt * client){
	sobj * obj;
	
	AssertSResultReturn(client, client->argc, 2, err_arg);
	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->type, SABER_HASH, err_type);

	size_t sec_len;
	char * start, * next, * buf = client->argv[1];
	char field_buf[128] = "\0";
	sobj * value, * field;
	int re, count = 0;
	
	/* parse field */
	EmptyResult(client);
	while(1){
		field = NULL;
		re = ValueSplit(buf, &sec_len, &start, &next);
		buf = next;
		AssertSResultReturn(client, re, 0, err_arg);
		if(0 == re) break;
		
		if(1 == re){/* string as field */
			field = sobjCreateStringLen(start, sec_len);
			hashObjSetKeyLnk(obj);
		}else if(2 == re){/* number as field */
			cpystr(field_buf, start, sec_len);
			field = sobjCreateStringInt(atoi(field_buf));
			hashObjSetKeyInt(obj);
		}
		if(!field){
			AddResultStr(client, server.share->err_mem);
			continue;
		}

		value = hashObjGet(obj, field);
		
		if(!value) AddResultStr(client, server.share->none);
		else{
			AddResultSobj(client, value);
			++count;
		}
		
		sobjFree(field);

		
	}
	InsResultInt(client, count, 0);
	
	return 1;
}
/* syntax: HGETM key regEx value
** get all key-value pairs that key satisfield with
** regEx and value equal to value
** value is optional */
int HGetMCmd(sclnt * client){
	sobj * obj;

	AssertSResultReturn(client, client->argc, 2, err_arg);
	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->encode, SABER_ENCODE_DICT, err_type);

	
	size_t sec_len;
	char * start, * next, * buf = client->argv[1];
	char regex[256] = "\0";
	char num_val[30] = "\0";
	sobj * value = NULL;
	int re, vflag = 1;
	
	/* regEx */
	re = ValueSplit(buf, &sec_len, &start, &next);
	buf = next; 
	AssertSResultReturn(client, re, 0, err_arg);
	cpystr(regex, start, sec_len);
	/* value */
	re = ValueSplit(buf, &sec_len, &start, NULL);
	if(re <= 0)
		vflag = 0;
	else if(1 == re){
		value = sobjCreateStringLen(start, sec_len);
		AssertResultReturn(client, value, NULL, err_mem);
	}else if(2 == re){
		cpystr(num_val, start, sec_len);
		value = sobjCreateStringInt(atoll(num_val));
		AssertResultReturn(client, value, NULL, err_mem);
	}
	
	fdict * dict = (fdict*)obj->value;
	dictIter * iter = fdictIterCreate(dict, 0);
	sobj * iter_key, * iter_val;
	char * key_str, * s = regex;
	int flag, n = 0;

	AssertResultReturn(client, iter, NULL, err_mem);
	EmptyResult(client);
	do{
		iter_val = (sobj*)iter->current->value.val;
		iter_key = (sobj*)iter->current->key;
		key_str = ((fstr*)iter_key->value)->buf;
		if(iter_key->encode == SABER_ENCODE_STRING){
			flag = 1;
			s = regex;
			while(*s && *key_str){
				if(*s++ != *key_str++){
					flag = 0;
					break;
				}
			}
			if(vflag)
				flag = (0 == sobjCmp(iter_val, value));
			if(flag && *key_str == ':'){
				++n;
				AddResultSobj(client, iter_key);
				AddResultSobj(client, iter_val);
			}
		}
		iter = fdictIterNext(dict, iter);
	}while(iter);

	InsResultInt(client, n, 0);

	if(vflag)
		sobjFree(value);
	
	return 1;
}
/* syntax:HGETA key num pos
** get all key-value pairs
** num must be number, if num large than
** actually number of nodes, the real value would be,
** num is optional, pos is optinal */
int HGetACmd(sclnt * client){
	sobj * obj;
	
	/* AssertSResultReturn(client, client->argc, 2, err_arg); */
	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->type, SABER_HASH, err_type);

	/****** temp code *******/
	AssertUResultReturn(client, obj->encode, SABER_ENCODE_DICT,
						err_type);
    /****** temp code *******/
	
	size_t sec_len, i = 0, count;
	char * start, * buf;
	char num_buf[128] = "\0";
	sobj * value, * field;
	fdict * dict = (fdict*)obj->value;
	dictIter * iter;
	int re;
	
	/* parse num */
	count = fdictLen(dict);
	if(client->argc >= 2){
		buf = client->argv[1];
		re = ValueSplit(buf, &sec_len, &start, NULL);
		if(2 == re){
			cpystr(num_buf, start, sec_len);
			count = atoll(num_buf);
	
			if(fdictLen(dict) < count)
				count = fdictLen(dict);
    	}
	}
	
	iter = fdictIterCreate(dict, 0);
	AssertResultReturn(client, iter, NULL, err_mem);

	EmptyResult(client);
	do{
		AddResultSobj(client, (sobj*)iter->current->key);
		AddResultSobj(client, (sobj*)iter->current->value.val);
		++i;
		if(!(iter = fdictIterNext(dict, iter)))
			break;
	}while(i < count);

	InsResultInt(client, i, 0);
	
	return 1;
}

/* syntax:HDEL key field1 field2....
** delete all value through field2 */
int HDelCmd(sclnt * client){
	sobj * obj;
	
	AssertSResultReturn(client, client->argc, 2, err_arg);
	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->type, SABER_HASH, err_type);

	size_t sec_len;
	char * start, *next, * buf = client->argv[1];
	char field_buf[128] = "\0";
	sobj * value, * field = NULL;
	int re, count = 0;
    	
	EmptyResult(client);
	while(1){
		re = ValueSplit(buf, &sec_len, &start, &next);
		buf = next;
		AssertSResultReturn(client, re, 0, err_arg);
		if(!re) break;
		
		if(1 == re){/* string as field */
			field = sobjCreateStringLen(start, sec_len);
			hashObjSetKeyLnk(obj);
		}else if(2 == re){/* number as field */
			cpystr(field_buf, start, sec_len);
			field = sobjCreateStringInt(atoi(field_buf));
			hashObjSetKeyInt(obj);
		}
		if(!field){
			AddResultStr(client, server.share->err_mem);
			continue;
		}
		
		re = hashObjRemove(obj, field);
		if(SABER_OK == re) ++count;
		
		sobjFree(field);
	}
	InsResultInt(client, count, 0);
	
	return 1;
}

/* syntax: HLEN key regEx
** if there be regEx - regular expression,
** return the length of set which
** keys are satisfield with regEx */
int HLenCmd(sclnt * client){
	sobj * obj;

	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->type, SABER_HASH, err_type);

	int re = hashObjLen(obj);
	SetResultInt(client, re);

	if(client->argc <= 1) return 1;
	
	size_t sec_len;
	char * start;
	char buf[256] = "\0";
	re = ValueSplit(client->argv[1], &sec_len, &start, NULL);
	if(re > 0){
		cpystr(buf, start, sec_len);
		re = hashObjLenRegEx(obj, buf);

		SetResultInt(client, re);
	}
	
	return 1;
}
/* syntax: HXST key field */
int HXstCmd(sclnt * client){
	sobj * obj;
	
	AssertSResultReturn(client, client->argc, 2, err_arg);
	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->type, SABER_HASH, err_type);

	size_t sec_len;
	char * start;
	char field_buf[128] = "\0";
	sobj * value, * field;
	int re;

	re = ValueSplit(client->argv[1], &sec_len, &start, NULL);
	AssertSEResultReturn(client, re, 0, err_arg);
	
	if(1 == re){/* string as field */
		field = sobjCreateStringLen(start, sec_len);
		hashObjSetKeyLnk(obj);
	}else if(2 == re){/* number as field */
		cpystr(field_buf, start, sec_len);
		field = sobjCreateStringInt(atoi(field_buf));
		hashObjSetKeyInt(obj);
	}
	AssertResultReturn(client, field, NULL, err_mem);

	re = hashObjExist(obj, field);
	if(re) SetResultStr(client, server.share->yes);
	else SetResultStr(client, server.share->none);
	
	sobjFree(field);
	
	return 1;
}
int HKeysCmd(sclnt * client){
	
	return 1;
}
int HValsCmd(sclnt * client){
	return 1;
}
/* syntax: HINCR key field num
** plus num(must be number, could be negetive)
** to value which could be parse to number
** float not support yet, but would be added later */
int HIncrCmd(sclnt * client){
	sobj * obj;
	
	AssertSResultReturn(client, client->argc, 2, err_arg);
	obj = stabGet(client->table, client->argv[0]);
	AssertResultReturn(client, obj, NULL, no_key);
	AssertUResultReturn(client, obj->type, SABER_HASH, err_type);

	size_t sec_len;
	char * start, * next, * buf = client->argv[1];
	char nbuf[128] = "\0";
	sobj * value, * field = NULL;
	int64_t num;
	int re;

	/* field */
	re = ValueSplit(buf, &sec_len, &start, &next);
	buf = next;
	AssertSEResultReturn(client, re, 0, err_arg);
	if(1 == re){
		field = sobjCreateStringLen(start, sec_len);
		hashObjSetKeyLnk(obj);
	}else if(2 == re){/* number as field */
		cpystr(nbuf, start, sec_len);
		field = sobjCreateStringInt(atoll(nbuf));
		hashObjSetKeyInt(obj);
	}
	AssertResultReturn(client, field, NULL, err_mem);

	value = hashObjGet(obj, field);
	AssertResultGoto(client, value, NULL, none, err);

	re = sobjToInt(value, &num);
	AssertResultGoto(client, re, 0, err_type, err);

	/* number */
	re = ValueSplit(buf, &sec_len, &start, NULL);
	AssertUResultGoto(client, re, 2, err_type, err);
	cpystr(nbuf, start, sec_len);
	if(SABER_ENCODE_INTEGER == value->encode){
		value = sobjCreateStringInt(num + atoll(nbuf));
	}else{
		sprintf(nbuf, "%"PRId64, num + atoll(nbuf));
		value = sobjCreateString(nbuf);
	}
	AssertResultGoto(client, value, NULL, err_mem, err);

	re = hashObjSetXst(obj, field, value);
	if(re != SABER_OK){
		sobjFree(field);
		sobjFree(value);
		SetResultStr(client, server.share->faild);
		return 0;
	}
	SetResultStr(client, server.share->ok);
	
	return 1;
	
err:

	sobjFree(field);
	return 0;
}
