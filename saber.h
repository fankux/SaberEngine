#ifndef SABER_H
#define SABER_H

#include <inttypes.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

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
#define SABER_ENCODE_HASH 6
#define SABER_ENCODE_HEAP 7

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
#define SABER_EVENT_SIZE 512

#define SABER_SERVER_CLIENT_MAX 256
#define SABER_SERVER_TABLE_NUM 8
#define SABER_SERVER_IP "127.0.0.1"
#define SABER_SERVER_PORT 7213

#define SABER_CLIENT_DEFAULT 0
#define SABER_CLIENT_KEEPALIVE 1

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
	char * crlf, * ok, * success, *none,* faild,
		* err_arg, * no_key,* err_mem, * err_svr,
		* err_cmd, *err_type,
		*err_cmd_syx, *err_out_range ;
}sshare;

typedef struct saberServer{
	char * config_file; /* configuration file's path */
	char * log_file; /* log file's path */

	fdList * client_list;	
	fdict * table; /* table pointer list */
	fdict * command;
	sshare * share;

	/* event */
	unsigned long ip;
	unsigned short port;
	int listen_fd;
	int epoll_fd;
	struct epoll_event * event_list;
	int event_list_len;
}ssvr;

typedef struct saberCommand{
	char * name;
	uint8_t type;
	size_t arity;

	void ( * func)(struct saberClient * c);

	int key_first;
	int key_last;
	int key_step;
}scmd;


	
/* extern */
extern ssvr server;
extern sshare shared;
extern scmd command_list[];
/****************************** API **************************************/
/* saber object */
sobj * sobjCreate(uint8_t type, void * value);
sobj * sobjCreateString(char * str);
sobj * sobjCreateStringLen(char * str, size_t len);
sobj * sobjCreateStringInt(const int64_t value);
sobj * sobjCreateList(void);
sobj * sobjCreateSort(void);
sobj * sobjCreateSortBinTree(void);
sobj * sobjCreateHash(void);
void sobjFree(sobj * o);
sobj * sobjGet(sobj * o);
size_t sobjGetValLen(sobj * o);

/************** list type object(listType.c) *************/
#define listObjLen(o) (o->encode == SABER_ENCODE_DLIST?	  \
					   (fdListLen(((fdList *)o->value))): \
					   SABER_WRONGTYPE)
int listObjPushHead(sobj * o, sobj * new);
int listObjPushTail(sobj * o, sobj * new);
sobj * listObjPopHead(sobj * o);
sobj * listObjPopTail(sobj * o);
int listObjSet(sobj * o, const size_t index, void * value);
sobj * listObjGetIndex(sobj * o, const size_t index);
sobj * listObjGetRandom(sobj * o);
int listObjIndexOf(sobj * o, void * value);
int listObjRemoveAt(sobj * o, const size_t pos, void ** out_val);
int listObjRemoveObj(sobj * o, void * value, void ** out_val);
/* list iterator */
#define SABER_LIST_HEADFIRST 0
#define SABER_LIST_TAILFIRST 1
typedef struct listObjIter{
	int direct;
	size_t rank;
	sobj * next;
}listObjIter;
listObjIter * listObjIterCreate(sobj * o, const uint8_t direct,
								const size_t start_pos);
sobj * listObjIterNext(listObjIter * iter);
void listObjIterCancel(listObjIter * iter);
void listObjIterRewind(sobj * o, listObjIter * iter);

/************************ command *************************/
void LPushCmd(sclnt * client);
void RPushCmd(sclnt * client);
void LPopCmd(sclnt * client);
void RPopCmd(sclnt * client);
void LLenCmd(sclnt * client);
void LSetCmd(sclnt * client);
void LInsCmd(sclnt * client);
void LRemCmd(sclnt * client);
void LIdxCmd(sclnt * client);
void LRngCmd(sclnt * client);
/************* sort type object ****************/
int sortObjAdd(sobj * o, sobj * new);

/************* hash type object ****************/
int hashObjAdd(sobj * o, sobj * new);
int hashObjRemove(sobj * o, const char * key);
int hashObjGet(sobj * o, const char * key, void * value);
int hashObjSet(sobj * o, const char * key, const void * vlaue);
int hashObjExist(sobj * o, const char * key);

/********** table(table.c) ***********************/
stab * stabCreate();
void stabFree(stab * t);
int stabAdd(stab * t, char * key, sobj * value);
int stabRemove(stab * t, char * key);
int stabSet(stab * t, char * key, sobj * value);
sobj * stabGet(stab * t, char * key);
int stabReplace(stab * t, char * key, sobj * value);
int stabExist(stab * t, char * key);

/********** saber client ****************/
sclnt * sclntCreate(int fd, unsigned int ip);
void sclntFree(sclnt * c);
/********** saber engine ****************/
void SaberEngineStart();
/* request event(event.c) */
int EventInit();
/* all request and events birth place */
int EventLoopStart();
/* list comparation function */
extern int listCmpFuncIp(void * a, void * b);
extern int listCmpFuncFdForce(void * a, void * b);
extern int listCmpFuncFdAlive(void * a, void * b);

/**************** command(command.c) *********************/
void CommandCat();
void CommandDo(unsigned long fd, char * buf);
void AddResultStr(sclnt * client, char * str);
void SetResultStr(sclnt * client, char * str);
void AddResultInt(sclnt * client, size_t value);
void SetResultInt(sclnt * client, size_t value);
#define AssertResultReturn(client, obj, value, err_name)	\
	if(obj == value){										\
		SetResultStr(client, server.share->err_name);		\
		return;												\
	}

#define AssertResultGoto(client, obj, value, err_name, dest)	\
	if(obj == value){											\
		SetResultStr(client, server.share->err_name);			\
		goto dest;												\
	}
#define AssertUResultReturn(client, obj, value, err_name)	\
	if(obj != value){										\
		SetResultStr(client, server.share->err_name);		\
		return;												\
	}
#define AssertUResultGoto(client, obj, value, err_name, dest)	\
	if(obj != value){											\
		SetResultStr(client, server.share->err_name);			\
		goto dest;												\
	}
#define AssertSEResultReturn(client, obj, value, err_name) \
	if(obj <= value){									   \
		SetResultStr(client, server.share->err_name);	   \
	}
#define AssertSResultReturn(client, obj, value, err_name)  \
	if(obj < value){									   \
		SetResultStr(client, server.share->err_name);	   \
	}
#endif /* saber.h */
