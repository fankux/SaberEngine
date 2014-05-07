#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "saber.h"

static int set_noblocking(int fd){
	int sock_opts;
	if(-1 == (sock_opts = fcntl(fd, F_GETFL))){
		return 0;
	}
	if(-1 == fcntl(fd, F_SETFL, sock_opts | O_NONBLOCK)){
		return 0;
	}
	return 1;
}

int EventInit(){
	int sockfd, listenfd, connfd, epollfd;
	struct sockaddr_in addr_in;
	struct epoll_event ev;
	
	memset(&addr_in, 0, sizeof(addr_in));
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(server.port);
	addr_in.sin_addr.s_addr = htonl(INADDR_ANY);

	/* the arg not need since kernel 2.6.8 */
	epollfd = epoll_create(SABER_EVENT_SIZE);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	set_noblocking(sockfd);
	ev.data.fd = listenfd;
	ev.events = EPOLLIN | EPOLLET;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);

	if(-1 == bind(listenfd, (struct sockaddr *)&addr_in, sizeof(addr_in)))
		return 0;
	
	if(-1 == listen(listenfd, 10))
		return 0;

	set_noblocking(listenfd);
	server.listen_fd = listenfd;
	server.epoll_fd = epollfd;
	
	return 1;
}

/* a fdList CmpValFunc implemention,
** compare through ip addres
** a is type of 'sclnt'
** b is the ip to be compared, type of 'unsigned long' */
int listCmpFuncIp(void * a, void * b){
	unsigned long ip1 = ((sclnt *)a)->ip;
	unsigned long ip2 = *(unsigned long *)b;
	
	return (ip1 != ip2);
}
/* a fdList CmpValFunc implemention,
** compare through ip, and this client can't has flag KEEPALIVE
** a is type of 'sclnt', it's data field is 'sclnt'
** b is the ip to be compared, type of unsigned long */
int listCmpFuncIpAlive(void * a, void * b){
	sclnt * clnt = (sclnt *)a;
	unsigned long ip1 = clnt->fd;
	unsigned long ip2 = *(unsigned long *)b;

	if(clnt->flags & SABER_CLIENT_KEEPALIVE)
		return 1;
	return (ip1 != ip2);
}

#define epoll_delete(epfd, sockfd, ev) do{				\
		epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, &ev);	\
		close(sockfd);									\
	}while(0)

#define epoll_setread(epfd, sockfd, ev) do{				\
		ev.data.fd = sockfd;							\
		ev.events = EPOLLET | EPOLLIN;					\
		epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);	\
	}while(0)

#define epoll_setwrite(epfd, sockfd, ev) do{			\
		ev.data.fd = sockfd;							\
		ev.events = EPOLLET | EPOLLOUT;					\
		epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);	\
	}while(0)

#define clientlist_trydelete(list, fd) do{		\
		list->CmpValFunc = listCmpFuncIpAlive;	\
		fdListRemoveValue(list, fd, NULL);		\
	}while(0)

#define clientlist_forcedelete(list, fd) do{	\
		list->CmpValFunc = listCmpFuncIp;	\
		fdListRemoveValue(list, fd, NULL);		\
	}while(0)

/* epoll event loop, the buffer recevied while send to "CommandDo",
** it would prase data package, add command to a command queue;
** if get a connect event, it will create a client, if a client has
** 'keep alive' flag, the sock wouldn't close, but any socket which
** recevied event occured error would kicked out from epoll list  */
int EventLoopStart(){
	struct epoll_event ev;
	struct sockaddr_in addr_client;
	socklen_t addr_client_len = sizeof(addr_client);
	int sockfd, connfd;
	int nfds; /* number of events */
	int i, flag, buf_len, ip_exist, send_buf_len, buf_index = 0;
	char buf[SABER_RECVBUF_MAX + 1] = "\0";
	fdList * client_list;
	fdListNode * client_list_node;

	memset(buf, '\0', sizeof(buf));
	do{
		nfds = epoll_wait(server.epoll_fd, server.event_list,
						  SABER_EVENT_SIZE, 50);
		if(nfds > 0) {
		}else if(nfds == -1){
			exit(EXIT_FAILURE);
		}
		
		for(i = 0; i < nfds; ++i){
			sockfd = server.event_list[i].data.fd;
			client_list = server.client_list;
			if(sockfd == server.listen_fd){/* connect event */
				/* read all request may be come out at same time */

				printf("connect event fired!\n");
				while((connfd = accept(server.listen_fd,
									   (struct sockaddr *)&addr_client,
									   &addr_client_len)) > 0){
					printf("accepted, connfd is:%d\n", connfd);
					
					if(client_list->len >= SABER_SERVER_CLIENT_MAX)
						break;

					set_noblocking(connfd);
					ev.data.fd = connfd;
					ev.events = EPOLLIN | EPOLLET;
					epoll_ctl(server.epoll_fd, EPOLL_CTL_ADD, connfd, &ev);

					client_list->CmpValFunc = listCmpFuncIp;
					/* if this ip exist */
					ip_exist = fdListGet(client_list,
										 &addr_client.sin_addr.s_addr,
										 &client_list_node);
					printf("list get result:%d\n", ip_exist);
                    /* add new one to list */
					if(ip_exist == FDLIST_NONE){
						printf("new client added\n");
						
						fdListAddHead(
							server.client_list,
							(void *)sclntCreate(connfd,
												addr_client.sin_addr.s_addr));
					}else{ /* change the ip with new connfd */
						printf("got the client\n");
						
						((sclnt *)client_list_node->data)->fd = connfd;
					}
				}
			}else if(server.event_list[i].events & EPOLLIN ){
				/* read event, a connect fd will be deleted from
				** epoll_list when reading complete */
				printf("read event fired,buf_index:%d\n", buf_index);
				flag = 1;
				while(flag){
					buf_len = read(sockfd, buf + buf_index, SABER_RECVBUF_BLOCK);
					if(-1 == buf_len && errno != EAGAIN){ /* error */
						printf("read error occur\n");

						epoll_delete(server.epoll_fd, sockfd, ev);
						clientlist_forcedelete(client_list, &sockfd);

						break;
					}else if(-1 == buf_len && errno == EAGAIN){/* completed */
						printf("reading complete, buf_len:%d\n", buf_index);

						buf[buf_index] = '\0';
						CommandDo(addr_client.sin_addr.s_addr, buf);
						
						epoll_setwrite(server.epoll_fd, sockfd, ev);

						break;
					}else if(0 == buf_len){ /* client shutdown */
						printf("client shutdown\n");
						
						epoll_delete(server.epoll_fd, sockfd, ev);
						clientlist_forcedelete(client_list, &sockfd);
						
						break;
					}/* else{} recevied a part of buf */
					/* max buf size achieved */
					if(SABER_RECVBUF_MAX - buf_index <= SABER_RECVBUF_BLOCK){
						printf("achieved buf max, buf_index:%d\n", buf_index);

						flag = 0;
						buf[SABER_RECVBUF_MAX] = '\0';
						CommandDo(addr_client.sin_addr.s_addr, buf);

						epoll_setwrite(server.epoll_fd, sockfd, ev);
					}else if(SABER_RECVBUF_BLOCK == buf_len){
						/* still reading */
						printf("still reading...buf_index: %d\n", buf_index);

						flag = 1;
						buf_index += buf_len;
					}else{/* complete */
						printf("read buf:%s\n", buf);
						
						flag = 0;
						buf[buf_len] = '\0';
						CommandDo(addr_client.sin_addr.s_addr, buf);

						epoll_setwrite(server.epoll_fd, sockfd, ev);
					}
				}
				buf_index = 0;
			}
			if(server.event_list[i].events & EPOLLOUT){/* wirte event */
				printf("write event fired\n");

				client_list->CmpValFunc = listCmpFuncIp;
				ip_exist = fdListGet(client_list,
									 &addr_client.sin_addr.s_addr,
									 &client_list_node);
				if(ip_exist == FDLIST_NONE){
					printf("client info destoryed\n");
					epoll_delete(server.epoll_fd, sockfd, ev);
				}else{/* got the client info */
					send_buf_len =
						((sclnt *)client_list_node->data)->result->len;
					buf_index = 0;
					
					while(send_buf_len != buf_index){
						buf_len =
							write(sockfd,
								  ((sclnt *)client_list_node->data)->result->buf+
								  buf_index, send_buf_len);
						if(-1 == buf_len && errno == EAGAIN){
                            /* writing complete */
							printf("writing complete!\n");

							break;
						}else if(-1 == buf_len && errno != EAGAIN){/* error */
							printf("write error occur:%s \n", strerror(errno));

							break;
						}else if(buf_len >= 0){/* writing, maybe complete */
							printf("write content:%s, buf_len:%d, buf_index:%d\n",
								   ((sclnt *)client_list_node->data)->result->buf,
								   buf_len, buf_index);

							buf_index += buf_len;
						}
					}
					buf_index = 0;
					
					epoll_delete(server.epoll_fd, sockfd, ev);
					//clientlist_trydelete(client_list, &sockfd);
				}
			}
		}
	}while(1);
}
