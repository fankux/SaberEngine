#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "saber.h"

scmd command_list[] = {
	/* list commands */
	{"LPUSH", 0x01, 2, LPushCmd, 1, 1, 0},
	{"RPUSH", 0x01, 2, RPushCmd, 1, 1, 0},
	{"LPOP" , 0x02, 1, LPopCmd, 0, 0, 0},
	{"RPOP" , 0x02, 1, LPopCmd, 0, 0, 0},
	{"LSET" , 0x03, 3, LSetCmd, 0, 0, 0},
	{"LLEN" , 0x00, 1, LLenCmd, 0, 0, 0},
	{"LINS" , 0x01, 2, LInsCmd, 1, 2, 0},
	{"LREM" , 0x02, 3, LRemCmd, 1, 3, 0},
	{"LIDX" , 0x00, 2, LIdxCmd, 0, 0, 0},
	{"LRNG" , 0x00, 4, LRngCmd, 1, 4, 0},
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
	int cmd_list_len = sizeof(command_list) / sizeof(command_list[0]);
	int i;
	scmd * p;
	int re;
	
	for(i = 0; i < cmd_list_len; ++i){
		p = command_list + i;
		re = fdictAdd(server.command, p->name, p);
	}
}

/* decrypt command, and then re-format command
** | ..0.. |s| ..1.. |s| ..2.. |s|.....\r\n
** 0, command strings
** s, spaces, unsure length of bits
** 1, argument 1
** 2, argument 2
** ..
** n,  argument n
** end up with \r\n */
void CommandDo(unsigned long ip, char * buf){
	fdListNode * client_node;
	sclnt * client;
	int ip_exist;
	char * cmd;
	dictNode * cmd_node;

	server.client_list->CmpValFunc = listCmpFuncIp;
	ip_exist = fdListGet(server.client_list, &ip, &client_node);
	if(ip_exist == FDLIST_NONE){
		SetResultStr(client, server.share->err_svr);
		return;
	}else{
		client = (sclnt *)client_node->data;
	}
	
	/* get cmd */
	while(*buf == ' ') ++buf;/* skip spaces */
	cmd = buf;
	while(*buf != ' ') ++buf;
	*buf = '\0';
	++buf;
	printf("cmd:%s\n", cmd);

	/* get cmd struct */
	cmd_node  = fdictSearch(server.command, (void *)cmd);
	AssertResultReturn(client, cmd_node, NULL, err_cmd);

	if(!client->argv){
		client->argv = malloc(client->argc_max * sizeof(char *));
		AssertResultReturn(client, client->argv, NULL, err_mem);
	}
	
	client->argc = 0;
	/* get key */
	char * start, * next;
	char key_buf[128] = "\0";
	size_t sec_len;
	int re;
	if((re = ValueSplit(buf, &sec_len, &start, &next)) <= 0){
		SetResultStr(client, server.share->err_cmd_syx);
		return;
	}
	client->argv[0] = start;
	++client->argc;

	if(*next){
		client->argv[1] = next + 1;
		++client->argc;
		printf("value:%s\n", next + 1);
	}
	client->argv[0][sec_len] = '\0';
	printf("key:%s\n", start);
	
	client->cmd = (scmd *)cmd_node->value.val;
	client->cmd->func(client);

	client->last_cmd = client->cmd;
	client->last_action_time = time(0);
	
	return;
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
