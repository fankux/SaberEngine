#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "saber.h"

/* shared const */
sshare shared = {
	"\r\n",
	"+ok\r\n",
	"+success\r\n",
	"-none\r\n",
	"-faild\r\n",
	"-arguement error\r\n",
	"-no such key\r\n",
	"-memory error\r\n",
	"-server error\r\n",
	"-command error\r\n",
	"-type error\r\n",
	"-command syntax error\r\n",
	"-index out of range\r\n",
};

ssvr server;

/* API Implements */
void SaberEngineStart(){
	/* initial server data struct */
	if(!(server.client_list = fdListCreate()))
	   	exit(EXIT_FAILURE);

	/* all users' table are in a fdict, key is user id */
	if(!(server.table = fdictCreate()))
		exit(EXIT_FAILURE);
	server.table->type = &dictTypeSvrTable;

	/* initial command table */
	if(!(server.command = fdictCreate()))
		exit(EXIT_FAILURE);
	server.command->type = &dictTypeCmdTable;
	
	server.share = &shared;

	server.ip = inet_addr(SABER_SERVER_IP);
	server.port = SABER_SERVER_PORT;

	server.event_list_len = SABER_SERVER_CLIENT_MAX;
	if(!(server.event_list =
		(struct epoll_event *)malloc(server.event_list_len *
									 sizeof(struct epoll_event))))
		exit(EXIT_FAILURE);

	/* classify the command_list */
	CommandCat();
}

sclnt * sclntCreate(int fd, unsigned int ip){
	sclnt * new;
	int re;

	if(!(new = (sclnt *)malloc(sizeof(sclnt))))
		return NULL;

	new->fd = fd;
	new->name = NULL;
	new->table = NULL;
	
	new->flags = SABER_CLIENT_DEFAULT;
	new->argc = 0;
	new->argc_max = 4;
	new->argv = NULL;
	new->result = NULL;
	
	new->create_time = time(0);
	new->last_action_time = new->create_time;
	new->cmd = NULL;
	new->last_cmd = NULL;
	
	new->authed = 1;
	new->ip = ip;

	/* initial users private table, is a saberTable */
	if(!(new->table = stabCreate()))
		return NULL;
	new->table->id = fdListLen(server.client_list);
	if(!(new->result = fstrCreate(NULL)))
		goto err;
	re = fdictAdd(server.table, &new->table->id,
				  (void *)new->table);
	if(re != FDICT_OK){
		goto err;
	}

	return new;

err:
	sclntFree(new);
	return NULL;
}
void sclntFree(sclnt * c){
	if(c->table)
		stabFree(c->table);
	if(c->name)
		sobjFree(c->name);
	if(c->argv)
		free(c->argv);
	if(c->result)
		fstrFree(c->result);

	free(c);
}
sobj * sobjCreate(uint8_t type, void * value){
	sobj * o;
	if(!(o = (sobj *)malloc(sizeof(sobj)))) return NULL;

	o->type = type;
	o->encode = SABER_ENCODE_STRING;
	o->value = value;

	return o;
}

sobj * sobjCreateString(char * str){
	return sobjCreate(SABER_STRING, (void *)fstrCreate(str));
}

sobj * sobjCreateStringLen(char * str, size_t len){
	return sobjCreate(SABER_STRING, (void *)fstrCreateLen(str, len));
}

sobj * sobjCreateStringInt(const int64_t value){
	sobj * o;
	if(!(o = (sobj *)malloc(sizeof(sobj)))) return NULL;
	
	if(value >= INT32_MIN && value <= INT32_MAX){
		o->type = SABER_STRING;
		o->encode = SABER_ENCODE_INTEGER;
		//if value less then 4 bytes, just cast to a pointer value
		o->value = (void *)((int32_t)value);
	}else
		o = sobjCreate(SABER_STRING, (void *)fstrCreateInt(value));

	return o;
}

sobj * sobjCreateList(void){
	sobj * o = sobjCreate(SABER_LIST, (void *)fdListCreate());
	o->encode = SABER_ENCODE_DLIST;

	return o;
}

sobj * sobjCreateSort(void){
	sobj * o = sobjCreate(SABER_SORT, (void *)fintsetCreate());
	/* use intset first,
	   if it's size increment to a size
	   which large than 512,
	   change it to bintree */
	o->encode = SABER_ENCODE_INTSET;

	return o;
}

sobj * sobjCreateSortBinTree(void){
	sobj * o = sobjCreate(SABER_SORT, (void *)fbintreeCreate());
	o->encode = SABER_ENCODE_BINTREE;

	return o;
}

void sobjFree(sobj * o){
	if(!o) return;
	switch(o->encode){
	case SABER_ENCODE_STRING :
		fstrFree((fstr *)o->value);
		free(o);
		break;
	case SABER_ENCODE_INTEGER :
		free(o);
		break;
	case SABER_ENCODE_DLIST :
		fdListFree((fdList *)o->value);
		free(o);
		break;
	case SABER_ENCODE_BINTREE :
		fbintreeFree((fbintree *)o->value);
		free(o);
		break;
	case SABER_ENCODE_INTSET :
		fintsetFree((fintset *)o->value);
		free(o);
		break;
	case SABER_ENCODE_HASH :
		fdictFree((fdict *)o->value);
		free(o);
		break;
	default:break;
	}
}

sobj * sobjCreateHash(void){
	sobj * o = sobjCreate(SABER_HASH, (void *)fdictCreate());
	o->encode = SABER_ENCODE_HASH;

	return o;
}


/* hash type object */
int hashObjAdd(sobj * o, sobj * new){
	/* if(o->encode != SABER_HASH || */
	/*    o->encode != SABER_HASH) return SABER_WRONGTYPE; */
	
	/* return fdictAdd((fdict *)o->value, ((fdict *)new->value)->key, */
	/* 		 &((fdict *)new->value)->value); */
}


static void sobjInfo(sobj * o){
	printf("\ntype:%d;encode:%d\n", o->type, o->encode);
}

int main(void){
	SaberEngineStart();
	EventInit();
	EventLoopStart();
	
	/* sobj * list = sobjCreateList(); */

	/* int value[] = {1,2,3,4,5,6,7,8,9,0}; */
	/* for(int i = 0; i < 10; ++i){ */
	/* 	listObjPushHead(list, sobjCreateStringInt(value[i])); */
	/* } */
	/* fdListInfo((fdList *)list->value); */
	/* sobj * p; */
	/* p = listObjGetIndex(list, 0); */
	/* if(p) printf("p value:%d\n", (int)p->value); */
	/* else printf("p is null\n"); */

	/* int re = fdListRemove(list, &value[0], 1, 2); */
	/* printf("result num:%d\n", re); */
	/* fdListInfo(list); */
}
