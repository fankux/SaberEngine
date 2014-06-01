#ifndef SABER_H
#define SABER_H

#define DEBUG_ALL 1
#define DEBUG_INFO 1

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <linux/limits.h>

#include "fstr.h"
#include "fdList.h"
#include "fdict.h"
#include "fintset.h"
#include "fbintree.h"
#include "fheap.h"
#include "common.h"

/* data type */
#define SABER_STRING 0
#define SABER_LIST 1
#define SABER_SORT 2
#define SABER_HASH 3

/* real data struct */
#define SABER_ENCODE_STRING 0
#define SABER_ENCODE_INTEGER 1
#define SABER_ENCODE_DLIST 2
#define SABER_ENCODE_QUEUE 3
#define SABER_ENCODE_BINTREE 4
#define SABER_ENCODE_INTSET 5
#define SABER_ENCODE_DICT 6
#define SABER_ENCODE_NULL 7

/* error code */
#define SABER_WRONGTYPE -2 /* wrong type */
#define SABER_FAILD -1 /* action faild,usual casued by memory error or index out of range */
#define SABER_MEMERR -1 /* memory full */
#define SABER_OUTRANGE -1 /* index out of range */
#define SABER_NONE 0 /* search action,no result */
#define SABER_SUCCESS 1 
#define SABER_OK 1
#define SABER_EXIST 2 /* insert action, value already exist */

/* constant */
/* maxium size of data which a client send once */
#define SABER_RECVBUF_MAX 4096
/* data size of read event each time */
#define SABER_RECVBUF_BLOCK 1024
#define SABER_EVENT_SIZE 64

#define SABER_SERVER_CLIENT_MAX 256
#define SABER_SERVER_TABLE_NUM 8
#define SABER_SERVER_IP "127.0.0.1"
#define SABER_SERVER_PORT 7213

#define SABER_CLIENT_DEFAULT 0
#define SABER_CLIENT_KEEPALIVE 1

#define SABER_PERSIST_MSGKEY 1024
#define SABER_PERSIST_MSGBUF_MAX 512

typedef struct saberObject{
	uint8_t type;
	uint8_t encode;
	void * value;
}sobj;

typedef struct saberTable{
	fdict * table;
	fdict * expire;
	int id;
}stab;

/************************ client ************************/
typedef struct saberClient{
	stab * table;
	sobj * name;
	int fd; /* connect id */
	int flags;
	int argc;
	int argc_max; /* max argv memory size */
	char ** argv;
	fstr * result; /* result list */

	time_t create_time;
	time_t last_action_time;
	
	struct saberCommand * cmd;
	struct saberCommand * last_cmd;

	int authed;
	unsigned long ip;
	
}sclnt;

typedef struct saberShared{
	char * crlf, * ok, * success, *yes, *none,* faild,
		* err_arg, * no_key,* err_mem, * err_svr,
		* err_cmd, *err_type,
		*err_cmd_syx, *err_out_range ;
}sshare;

typedef struct saberServer{
	char config_file[PATH_MAX]; /* configuration file's path */
	char log_file[PATH_MAX]; /* log file's path */

	fdList * client_list;	
	/* fdict * table; */ /* table pointer list */
	stab * table;
	fdict * command;
	sshare * share;

	/* event */
	unsigned long ip;
	unsigned short port;
	int event_flag;
	int listen_fd;
	int epoll_fd;
	struct epoll_event * event_list;
	int event_list_len;

	/* persistence */
	char persist_file[PATH_MAX];
	pid_t persist_pid;
	int persist_flag;
	FILE * fp;
	int msgid;

}ssvr;

typedef struct saberCommand{
	char * name;
	uint8_t type;
	size_t arity;

	int ( * func)(struct saberClient * c);

}scmd;

	
/* extern */
extern ssvr server;
extern sshare shared;
extern scmd command_list[];
/************************* API *********************************/
/* saber object */
sobj * sobjCreate(uint8_t type, void * value);
sobj * sobjCreateString(char * str);
sobj * sobjCreateStringLen(char * str, size_t len);
sobj * sobjCreateStringInt(const int64_t value);
sobj * sobjCreateStringFloat(const double value);
sobj * sobjCreateList(void);
sobj * sobjCreateSort(void);
sobj * sobjCreateSortBinTree(void);
sobj * sobjCreateHash(void);
void sobjFree(sobj * o);
sobj * sobjDup(sobj * o);
sobj * sobjGet(sobj * o);
size_t sobjGetValLen(sobj * o);
int sobjCmp(sobj * a, sobj * b);
int sobjToInt(sobj * a, int64_t * value);

/************** list type object(listType.c) *************/
#define listObjLen(o) (o->encode == SABER_ENCODE_DLIST?	  \
					   (fdListLen(((fdList *)o->value))): \
					   SABER_WRONGTYPE)
int listObjPushHead(sobj * o, sobj * new);
int listObjPushTail(sobj * o, sobj * new);
sobj * listObjPopHead(sobj * o);
sobj * listObjPopTail(sobj * o);
int listObjInsert(sobj * o, sobj * value, sobj * pivot,
				  const uint8_t aorb);
int listObjSet(sobj * o, const size_t index, sobj * value);
sobj * listObjGetIndex(sobj * o, const size_t index);
sobj * listObjGetRandom(sobj * o);
int listObjIndexOf(sobj * o, sobj * value);
int listObjRemove(sobj * o, sobj * value, const size_t pos,
				  const size_t count);
int listObjRemoveAt(sobj * o, const size_t pos, sobj ** out_val);
int listObjRemoveObj(sobj * o, sobj * value, sobj ** out_val);
int listObjRange(sobj * o, const size_t start, const size_t stop);
void listObjSetCmpSobj(sobj * o);
void listObjSetCmpSobjStr(sobj * o);
/**** list iterator ****/
#define SABER_LIST_HEADFIRST 0
#define SABER_LIST_TAILFIRST 1
typedef struct listObjIter{
	int direct;
	size_t rank;
	sobj * next;
	void * inner;
}listObjIter;
listObjIter * listObjIterCreate(sobj * o, const uint8_t direct,
								const size_t start_pos);
sobj * listObjIterNext(sobj * o, listObjIter * iter);
void listObjIterCancel(listObjIter * iter);
void listObjIterRewind(sobj * o, listObjIter * iter);
/* list comparation function */
extern int listCmpFuncIp(void * a, void * b);
extern int listCmpFuncFdForce(void * a, void * b);
extern int listCmpFuncFdAlive(void * a, void * b);
/**** command ****/
int LPushCmd(sclnt * client);
int RPushCmd(sclnt * client);
int LPopCmd(sclnt * client);
int RPopCmd(sclnt * client);
int LLenCmd(sclnt * client);
int LSetCmd(sclnt * client);
int LInsCmd(sclnt * client);
int LRemCmd(sclnt * client);
int LIdxCmd(sclnt * client);
int LRngCmd(sclnt * client);
/**** type function ****/
extern int fdListCmpSobj(void * a, void *b);
extern int fdListCmpSobjStr(void * a, void * b);
/************* sort type object ****************/
int sortObjAdd(sobj * o, sobj * new);

/************* hash type object ****************/
#define hashObjLen(o) (o->encode == SABER_ENCODE_DICT?	  \
					   (fdictLen(((fdict *)o->value))):	  \
					   SABER_WRONGTYPE)

int hashObjAdd(sobj * o, sobj * key, sobj * value);
int hashObjRemove(sobj * o, sobj * key);
int hashObjSet(sobj * o, sobj * key, sobj * value);
sobj * hashObjGet(sobj * o, sobj * key);
int hashObjExist(sobj * o, void * key);

/* void * hashObjSetKeyCmpSobjStr(sobj * o),
** void * hashObjSetKeyCmpSobjInt(sobj * o),
** void * hashObjSetKeyCmpSobj(sobj * o),
** void * hashObjSetValInt(sobj * o),
** void * hashObjSetValStr(sobj * o),
** void * hashObjSetValLnk(sobj * o) */
/**** command ****/
int HSetCmd(sclnt * client);
int HSetXstCmd(sclnt * client);
int HGetCmd(sclnt * client);
int HGetMCmd(sclnt * client);
int HGetACmd(sclnt * client);
int HDelCmd(sclnt * client);
int HLenCmd(sclnt * client);
int HXstCmd(sclnt * client);
int HKeysCmd(sclnt * client);
int HValsCmd(sclnt * client);
int HIncrCmd(sclnt * client);
/********** table(table.c) ***********************/
stab * stabCreate();
void stabFree(stab * t);
int stabAdd(stab * t, char * key, sobj * value);
int stabRemove(stab * t, char * key);
int stabSet(stab * t, char * key, sobj * value);
sobj * stabGet(stab * t, char * key);
int stabReplace(stab * t, char * key, sobj * value);
int stabExist(stab * t, char * key);

/************************ saber client ***************************/
sclnt * sclntCreate(int fd, unsigned int ip);
void sclntFree(sclnt * c);
/********************** saber engine *****************************/
void SaberEngineStart();
void SaberEngineExit();
/* request event(event.c) */
int EventInit();
/* all request and events birth place */
int EventLoopStart();
/* persistence */
void PersistStart();
int PersistSend(char * str);
int PersistRecovery();
/****************** command(command.c) ***************************/
int CommandIsMem(scmd * cmd);
void CommandCat();
int CommandDo(unsigned long fd, char * buf);
void EmptyResult(sclnt * client);
void AddResultStr(sclnt * client, char * str);
void AddResultInt(sclnt * client, size_t value);
void AddResultSobj(sclnt * client, sobj * o);
void SetResultStr(sclnt * client, char * str);
void SetResultInt(sclnt * client, size_t value);
void SetResultSobj(sclnt * client, sobj * o);
void InsResultStr(sclnt * client, char * str, const size_t pos);
void InsResultInt(sclnt * client, size_t value, const size_t pos);
#define AssertResultReturn(client, obj, value, err_name)	\
	if(obj == value){										\
		SetResultStr(client, server.share->err_name);		\
		return 0;											\
	}
#define AssertResultGoto(client, obj, value, err_name, dest)	\
	if(obj == value){											\
		SetResultStr(client, server.share->err_name);			\
		goto dest;												\
	}
#define AssertUResultReturn(client, obj, value, err_name)	\
	if(obj != value){										\
		SetResultStr(client, server.share->err_name);		\
		return 0;											\
	}
#define AssertUResultGoto(client, obj, value, err_name, dest)	\
	if(obj != value){											\
		SetResultStr(client, server.share->err_name);			\
		goto dest;												\
	}
#define AssertSEResultReturn(client, obj, value, err_name) \
	if(obj <= value){									   \
		SetResultStr(client, server.share->err_name);	   \
		return 0;										   \
	}
#define AssertSEResultGoto(client, obj, value, err_name,dest)	\
	if(obj <= value){											\
		SetResultStr(client, server.share->err_name);			\
		goto dest;												\
	}
#define AssertSResultReturn(client, obj, value, err_name)  \
	if(obj < value){									   \
		SetResultStr(client, server.share->err_name);	   \
		return 0;										   \
	}
#define AssertSResultGoto(client, obj, value, err_name, dest)   \
	if(obj < value){											\
		SetResultStr(client, server.share->err_name);			\
		goto dest;												\
	}
#endif /* saber.h */
