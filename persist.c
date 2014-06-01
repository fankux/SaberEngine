#include "saber.h"

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <sys/msg.h>

typedef struct msg_buf{
	long mtype;
	char mtext[SABER_PERSIST_MSGBUF_MAX];
}msg_buf;

static void PersistChildExitHandler(int i){
	server.persist_flag = 0;

	//fclose(server.fp);
	return;
}

/* start persist process */
void PersistStart(){
	int msgid;
	msg_buf buf;
	pid_t pid;
	
	signal(SIGCHLD, SIG_IGN);/* ignore child exit signal */

	server.persist_flag = 1;
	if((msgid = msgget(SABER_PERSIST_MSGKEY,
					   IPC_CREAT | 0666)) < 0)
		exit(EXIT_FAILURE);
	server.msgid = msgid;

	pid = fork();
	if(pid < 0) exit(EXIT_FAILURE);
	if(pid == 0){/* child process */
		signal(SIGINT, PersistChildExitHandler);
		signal(SIGTERM, PersistChildExitHandler);
		
	    if((msgid = msgget(SABER_PERSIST_MSGKEY,
		 				   IPC_CREAT | 0666)) < 0)
		 	exit(EXIT_FAILURE);

		if(!(server.fp = fopen(server.persist_file, "a+")))
			exit(EXIT_FAILURE);
		
		
#ifdef DEBUG_INFO
		printf("child created,msgid:%d \n", msgid);
#endif		
		while(server.persist_flag){
			int re;
#ifdef DEBUG_INFO
			printf("persist in loop\n");
#endif			
			re = msgrcv(msgid, (void *)&buf, sizeof(buf.mtext),
				0, MSG_NOERROR);
			if(re != -1){
#ifdef DEBUG_INFO
				printf("persist message recevied\n");
#endif
				/* write to local file */
				strcat(buf.mtext, "\r\n");
				re = fwrite(buf.mtext, strlen(buf.mtext),
							1, server.fp);
				fflush(server.fp);
#ifdef DEBUG_INFO
				printf("persist write re:%d;content:%s\n",
					re, buf.mtext);
#endif
			}
		}
		exit(0);/* child exit */
	}else{/* parent */
		server.persist_pid = pid;
	}
}
/* send command string to persistence process through message,
** then persist(child) process write the command string to
** local file */
int PersistSend(char * str){
	msg_buf buf;
	int msgid, re;
	
	buf.mtype = 1;
	strcpy(buf.mtext, str);

	re = msgsnd(server.msgid, (void*)&buf, sizeof(buf.mtext), 0);
	
	return re;
}

/* recovery dataStroge from local file */
int PersistRecovery(){
	/* fd and ip both 0 is persistence dummy client */
	sclnt * client = sclntCreate(0, 0);

	if(FDLIST_OK != fdListAddHead(server.client_list,
								  (void*)client))
		return 0;

	FILE * fp = fopen(server.persist_file, "r");
	if(!fp) return 0;
	
	char * line;
	size_t len = 0;
	ssize_t read;
	
	while((read = getline(&line, &len, fp)) != EOF){
		while(isspace(line[read -1])){
			line[read-1] = '\0';
			--read;
		}
		CommandDo(0, line);
	}

	fdListNode * node = fdListPopTail(server.client_list);
	sclntFree(node->data);
	free(node);
	
	return 1;
}

#ifdef DEBUG_PERSIST
int main(void){
	int pid, msgid;
	int msg_key = SABER_PERSIST_MSGID;
	msgbuf buf = {1, "messagesssssss..\n"};
	
	
	pid = fork();
	if(pid < 0){/* error */
		
	}else if(0 == pid){/* parent */
		if((msgid = msgget(msg_key, IPC_CREAT | 0666)) < 0){
			printf("parent error\n");
			return 0;
		}
		msgsnd(msgid, (void*)&buf, sizeof(msgbuf), 0);
		
	}else{/* child */
		if((msgid = msgget(msg_key, IPC_CREAT | 0666)) < 0){
			printf("child error\n");
			return 0;
		}
		msgrcv(msgid, (void *)&buf, sizeof(msgbuf), 0, 0);
		FILE * fp = fopen("file", "a+");
		fwrite(buf.mbuf, strlen(buf.mbuf), 1, fp);
	}
	return 0;
}
#endif
