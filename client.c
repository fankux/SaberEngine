#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define PORT 7216
#define IP "127.0.0.1"
#define MAX_LEN 4096

int main(void){
	int sock_listen, sock_conn;
	sockaddr_in addr_in;
	char * buf[MAX_LEN];
	size_t buf_len = 0;
		
	addr_in.sin_family = AF_INET;
	addr_in.sin_post = htons(PORT);
	addr_in.sin_addr = htonl(inet_addr(IP));
	
	sock_listen = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == bind(sock, (struct sockaddr *)&addr_in, sizeof(addr_in))){
		printf("bind error:%d:%s", errno, strerror(errno));
		return 0;
	}
	if(-1 == listen(sock_listen, 10)){
		printf("listen error:%d:%s", errno, strerror(errno));
		return 0;
	}

	do{
		if(-1 == (sock_conn = accept(sock_listen, NULL, NULL))){
			printf("accept error:%d:%s", error, strerror(errno));
			return 0;
		}

		buf_len = recv(sock_conn, buf, MAX_LEN, 0);
		buf[buf_len] = "\0";
		printf("receviced msg is:%s\n", buf);

	}while(0);
	close(sock_conn);
	close(sock_listen);
	
	return 0;
}
