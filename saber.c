#include "saber.h"
#include <string.h>
#include <signal.h>
#include <ctype.h>

/* shared const */
sshare shared = {
	"\r\n",
	"+ok\r\n",
	"+success\r\n",
	"+yes\r\n",
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

static void SaberExitHandler(int i){
	SaberEngineExit();
}

/* API Implements */
void SaberEngineStart(){
	/* register for exit signal */
	signal(SIGINT, SaberExitHandler);
	signal(SIGTERM, SaberExitHandler);
	
	/* initialize server data struct */
	if(!(server.client_list = fdListCreate()))
	   	exit(EXIT_FAILURE);

	/* all users' table are in a fdict, key is user's ip */
	if(!(server.table = stabCreate()))
		exit(EXIT_FAILURE);
	/* server.table->type = &dictTypeSvrTable; */

	/* initialize command table */
	if(!(server.command = fdictCreate()))
		exit(EXIT_FAILURE);
	server.command->type = &dictTypeCmdTable;
	
	server.share = &shared;

	server.ip = inet_addr(SABER_SERVER_IP);
	server.port = SABER_SERVER_PORT;

	server.event_list_len = SABER_SERVER_CLIENT_MAX;
	if(!(server.event_list = malloc(server.event_list_len *
									sizeof(struct epoll_event))))
		exit(EXIT_FAILURE);

	memcpy(server.persist_file, "localdata", sizeof("localdata"));

	/* classify the command_list */
	CommandCat();
}

void SaberEngineExit(){
	printf("SaberEngine exit!ByeBye!\n");
	
	/* close persistence child process */
	kill(server.persist_pid, SIGINT);

	//waitpid(server.persist_pid);

	/* make epoll event loop exit */
	close(server.epoll_fd);
	close(server.listen_fd);
	
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

	/* initial users private table,its a saberTable */
	/* if(!(new->table = stabCreate())) */
	/* 	return NULL; */
	/* new->table->id = fdListLen(server.client_list); */
	new->table = server.table;
	if(!(new->result = fstrCreate(NULL)))
		goto err;
	/* if(FDICT_OK != (re = fdictAdd(server.table, &new->ip, */
	/* 							  (void *)new->table))) */

	/* goto err; */

	return new;

err:
	sclntFree(new);
	return NULL;
}
void sclntFree(sclnt * c){
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
	
	if(value >= INT32_MIN && value <= INT32_MAX){
		if(!(o = (sobj *)malloc(sizeof(sobj))))
			return NULL;
		o->type = SABER_STRING;
		o->encode = SABER_ENCODE_INTEGER;
		//if value less then 4 bytes, just cast to a pointer value
		o->value = (void *)((int32_t)value);
	}else
		o = sobjCreate(SABER_STRING, (void *)fstrCreateInt(value));

	return o;
}
sobj * sobjCreateStringFloat(const double value){
	char buffer[50] = "\0";
	sprintf(buffer, "%lf", value);
	
	return sobjCreateString(buffer);
}
sobj * sobjCreateList(void){
	sobj * o = sobjCreate(SABER_LIST, (void *)fdListCreate());
	o->encode = SABER_ENCODE_DLIST;

	return o;
}
sobj * sobjCreateHash(void){
	sobj * o = sobjCreate(SABER_HASH, (void *)fdictCreate());
	o->encode = SABER_ENCODE_DICT;

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
		break;
	case SABER_ENCODE_INTEGER :
		break;
	case SABER_ENCODE_DLIST :
		fdListFree((fdList *)o->value);
		break;
	case SABER_ENCODE_BINTREE :
		fbintreeFree((fbintree *)o->value);
		break;
	case SABER_ENCODE_INTSET :
		fintsetFree((fintset *)o->value);
		break;
	case SABER_ENCODE_DICT :
		fdictFree((fdict *)o->value);
		break;
	}
	free(o);
}

sobj * sobjDup(sobj * o){
	sobj * new;
	
	if(!o) return NULL;

	if(!(new = malloc(sizeof(sobj)))) return NULL;
	new->type = o->type;
	new->encode = o->encode;
	
	switch(o->encode){
	case SABER_ENCODE_STRING :
		new->value = fstrDup((fstr *)o->value);
		return new;
	case SABER_ENCODE_INTEGER :
		new->value = o->value;
		return new;
	default:
		free(new);
		return NULL;
	}
}

/* convert sobj to number which encode is string or integer,
** if string could convert to number
** then value out put to 'value',
** if success, return 1, else return 0 */
int sobjToInt(sobj * o, int64_t * value){
	if(SABER_ENCODE_INTEGER == o->encode){
		if(value) *value = (int)o->value;
		
	}else if(SABER_ENCODE_STRING == o->encode){
		char * s = ((fstr*)o->value)->buf;
		int64_t t = atoll(s);
		if(0 == t){/* judge if it contains charactors */
			if(!(*s >= '0' && *s <= '9') && *s != '-')
				return 0;
			++s;
			while(*s){
				if( *s < '0' || *s > '9')
					return 0;
				++s;
			}
		}
		
		if(value) *value = t;
	}else return 0;
}

int sobjCmp(sobj * a, sobj * b){
	if(a->encode != b->encode)
		return SABER_WRONGTYPE;
	switch(a->encode){
	case SABER_ENCODE_STRING:
		return fstrCompare((fstr *)a->value, (fstr *)b->value);
	case SABER_ENCODE_INTEGER:{
		int32_t m = (int32_t)a->value;
		int32_t n = (int32_t)b->value;
		return (m < n)? -1:(m > n? 1: 0);
	}
	}
}

static void sobjInfo(sobj * o){
	printf("\ntype:%d;encode:%d\n", o->type, o->encode);
}

#ifdef DEBUG_ALL
int main(void){
	int re;
	
	printf("Init Server Info...\n");
	SaberEngineStart();
	printf("Server info good!\n");

	printf("Recoverying local storage...\n");
	re = PersistRecovery();
	if(!re) printf("Local storage recoveried wrong!\n");
	else printf("Local storage good!\n");

	printf("Init Event Info...\n");
	re = EventInit();
	if(!re){
		printf("Event info wrong!\nSaberEngine init falid, exit!\n");
		return 0;
	}
	printf("Event info good!\n");

	printf("Init persistence process...\n");
	PersistStart();
	printf("Persistence process good!\n");

	printf("Init eventing loop...\n");
	EventLoopStart();

	printf("SaberEngine exit!ByeBye!\n");

	/* sobj * o = sobjCreateString("10"); */
	/* int64_t n; */
	/* re = sobjToInt(o, &n); */
	/* if(!re) printf("faild..\n"); */
	/* else printf("object value:%lld\n", n); */
}
#endif
