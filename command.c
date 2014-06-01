#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "saber.h"

scmd command_list[] = {
	/* list commands */
	{"LPUSH", 0x01, 2, LPushCmd},
	{"RPUSH", 0x01, 2, RPushCmd},
	{"LPOP" , 0x02, 1, LPopCmd},
	{"RPOP" , 0x02, 1, LPopCmd},
	{"LSET" , 0x03, 3, LSetCmd},
	{"LLEN" , 0x00, 1, LLenCmd},
	{"LINS" , 0x01, 2, LInsCmd},
	{"LREM" , 0x02, 3, LRemCmd},
	{"LIDX" , 0x00, 2, LIdxCmd},
	{"LRNG" , 0x00, 3, LRngCmd},
	/* hash commands */
	{"HSET" , 0x03, 3, HSetCmd},
	{"HSETX", 0x03, 3, HSetXstCmd},
	{"HGET" , 0x00, 2, HGetCmd},
	{"HGETM", 0x00, 3, HGetMCmd},
	{"HGETA", 0x00, 1, HGetACmd},
	{"HDEL" , 0x02, 2, HDelCmd},
	{"HLEN" , 0x00, 1, HLenCmd},
	{"HXST" , 0x00, 2, HXstCmd},
	{"HKEYS", 0x00, 1, HKeysCmd},
	{"HVALS", 0x00, 1, HValsCmd},
	{"HINCR" ,0x03, 3, HIncrCmd},
};

/* sort the command list by type field, it's a mask,
** each mask could combined, the flag field used for local persistence
** flag set 0, query command, never modify the memory using,
** first  2 bits: indicate the memory changing
** 00(0x00), memory won't by change,
** 01(0x01), add command, will increce memory using,
** 10(0x02), remove command, will decrece memory using,
** 11(0x03), set command, may modify memory using,
** bit 3, system command, would modify some system setting */
void CommandCat(){
	int i, cmd_list_len = sizeof(command_list) /
		sizeof(command_list[0]);
	scmd * p;
	
	for(i = 0; i < cmd_list_len; ++i){
		p = command_list + i;
		fdictAdd(server.command, p->name, p);
	}
}

/* judge if this command would modify memory use
** if memory sensetive, return 1, else return 0 */
inline int CommandIsMem(scmd * cmd){
	return (cmd->type & 3) != 0; 
}

/* decrypt command, and then re-format command
** | ..0.. |s| ..1.. |s| ..2.. |s|.....
** 0, command strings
** s, spaces, unsure length of bits
** 1, argument 1
** 2, argument 2
** ..
** n,  argument n
** end up with \r\n */
int CommandDo(unsigned long ip, char * buf){
	fdListNode * client_node;
	sclnt * client;
	int ip_exist;
	dictNode * cmd_node;

	server.client_list->CmpValFunc = listCmpFuncIp;
	ip_exist = fdListGet(server.client_list, &ip, &client_node);
	if(ip_exist == FDLIST_NONE){
		SetResultStr(client, server.share->err_svr);
		return 0;
	}else{
		client = (sclnt *)client_node->data;
	}
	
	/* get cmd */
	char * start, * next, * s = buf;
	char key_buf[128] = "\0";
	size_t sec_len;
	int re;

	re = ValueSplit(buf, &sec_len, &start, &next);
	AssertUResultReturn(client, re, 1, err_cmd);
	cpystr(key_buf, start, sec_len);

	/* get cmd struct */
	cmd_node  = fdictSearch(server.command, (void *)key_buf);
	AssertResultReturn(client, cmd_node, NULL, err_cmd);

	if(!client->argv){
		client->argv = malloc(client->argc_max * sizeof(char *));
		AssertResultReturn(client, client->argv, NULL, err_mem);
	}

	buf = next;
	client->argc = 0;
	/* get key */	
	if((re = ValueSplit(buf, &sec_len, &start, &next)) <= 0){
		SetResultStr(client, server.share->err_cmd_syx);
		return 0;
	}
	client->argv[0] = start;
	++client->argc;

	if(*next){
		client->argv[1] = next + 1;
		++client->argc;
	}else{
		client->argv[1] = NULL;
	}
	client->argv[0][sec_len] = '\0';
	
	client->cmd = (scmd *)cmd_node->value.val;
	re = client->cmd->func(client);

	/* if command successfully execute, persistence */
	if(1 == re && CommandIsMem(client->cmd) && client->ip != 0){
		client->argv[0][sec_len] = ' ';
		re = PersistSend(s);
	}

	client->last_cmd = client->cmd;
	client->last_action_time = time(0);
	
	return 1;
}
void EmptyResult(sclnt * client){
	fstrEmpty(client->result);
}

void AddResultStr(sclnt * client, char * str){
	fstr * s = client->result;

	client->result = fstrCat(s, str);
}
void SetResultStr(sclnt * client, char * str){
	fstr * s = client->result;

	client->result = fstrSet(s, str, 0);
}
void AddResultInt(sclnt * client, size_t value){
	char buf[30] = "\0";
	fstr * s = client->result;

	sprintf(buf, "(Int)%zu\r\n", value);

	client->result = fstrCat(s, buf);
}
void SetResultInt(sclnt * client, size_t value){
	char buf[30] = "\0";
	fstr * s = client->result;

	sprintf(buf, "(Int)%zu\r\n", value);

	client->result = fstrSet(s, buf, 0);
}
void AddResultSobj(sclnt * client, sobj * o){
	char buf[30];
	fstr * s = client->result;
		
	switch(o->encode){
	case SABER_ENCODE_NULL:
		client->result = fstrCat(s, "NULL\r\n");
		break;
	case SABER_ENCODE_INTEGER:
		sprintf(buf, "(Int)%"PRId32"\r\n", (int32_t)o->value);
		client->result = fstrCat(s, buf);
		break;
	case SABER_ENCODE_STRING:
		client->result = fstrCat(s, "\"");
		/* note that result's pointor may changed */
		client->result = fstrCat(client->result,
								 ((fstr*)o->value)->buf);
		client->result = fstrCat(client->result, "\"\r\n");
		break;
	}
}
void SetResultSobj(sclnt * client, sobj * o){
	char buf[30];
	fstr * s = client->result;
		
	switch(o->encode){
	case SABER_ENCODE_NULL:
		client->result = fstrSet(s, "NULL\r\n", 0);
		break;
	case SABER_ENCODE_INTEGER:
		sprintf(buf, "(Int)%"PRId32"\r\n", (int32_t)o->value);
		client->result = fstrSet(s, buf, 0);
		break;
	case SABER_ENCODE_STRING:
		client->result = fstrSet(s, "\"", 0);
		client->result = fstrCat(client->result,
								 ((fstr*)o->value)->buf);
		client->result = fstrCat(client->result, "\"\r\n");
		break;
	}
}
void InsResultStr(sclnt * client, char * str, const size_t pos){
	fstr * s = client->result;

	client->result = fstrInsert(s, str, pos);

}
void InsResultInt(sclnt * client, size_t value, const size_t pos){
	char buf[30] = "\0";
	fstr * s = client->result;

	sprintf(buf, "(Int)%zu\r\n", value);

	client->result = fstrInsert(s, buf, pos);
}
