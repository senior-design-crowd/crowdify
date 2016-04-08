#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <malloc.h>
#include <resolv.h>
#include <iostream>
#include <cyassl/ctaocrypt/settings.h>
#include <cyassl/ssl.h>
#include <cyassl/test.h>
#include "../disk-read-ng/configsql.h"


#define FAIL -1

#define PACKET_SIZE 8192
#define UDP_BROADCAST_PORT 1234
#define RESPOND_PORT 1235
#define HANDSHAKE_PORT 1236
#define SSH_KEY "/root/.ssh/id_rsa.pub"


int main(int argc, char *argv[])
{    

	//the main loop

    while(1){

	//listen for the initial UDP broadcast
    int handle = socket( AF_INET, SOCK_DGRAM, 0 );
    if(handle < 0){
      perror("socket");
      exit(1);
   }
    
   struct sockaddr_in servaddr;
   servaddr.sin_family = AF_INET;
   servaddr.sin_port=htons(UDP_BROADCAST_PORT);
   servaddr.sin_addr.s_addr= INADDR_ANY;

    if ( bind( handle, (struct sockaddr*)&servaddr, sizeof(servaddr) ) < 0 ){
   perror("bind");
   exit(1);
   }
     
      struct sockaddr_in cliaddr;   
	char *ip;   
	char packet_data[PACKET_SIZE];
    while ( 1 )
    { 

      socklen_t len= sizeof(cliaddr); 
      int received_bytes = recvfrom( handle, packet_data, sizeof(packet_data),0, (struct sockaddr*)&cliaddr, &len );
      if ( received_bytes > 0 ){
	//received the broadcast
	ip = inet_ntoa(((sockaddr_in)cliaddr).sin_addr);
       printf("Here is the message: %s, received from %s\n",packet_data, ip);
	break;
	}
    }

	

     close(handle);
	//now respond with a TCP connection to give the client your ip address

	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		perror("socket");
	cliaddr.sin_port = htons(RESPOND_PORT);
	if(connect(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) < 0)
		perror("connect");

	close(sockfd);

	//now wait for the client to connect with OpenSSL and perform the handshake

	StartTCP();
	CyaSSL_Init();
	ChangeToWolfRoot();

	word16 port = HANDSHAKE_PORT;
	tcp_listen(&sockfd, &port, 1, 0);
	CYASSL_METHOD* method = CyaSSLv23_server_method();
	CYASSL_CTX* ctx = CyaSSL_CTX_new(method);
	//CyaSSL_CTX_set_default_passwd_cb(ctx, PasswordCallBack);
	if(CyaSSL_CTX_use_certificate_file(ctx, "server-cert.pem", SSL_FILETYPE_PEM) != SSL_SUCCESS)
		std::cout << "can't load server cert file, " << std::endl;

	if(CyaSSL_CTX_use_PrivateKey_file(ctx, "server-key.pem", SSL_FILETYPE_PEM) != SSL_SUCCESS)
		std::cout << "can't load server key file" << std::endl;


	SOCKADDR_IN_T client;
	socklen_t client_len = sizeof(client);
	int clientfd = accept(sockfd, (struct sockaddr*) &client, (ACCEPT_THIRD_T)&client_len);
	if(WOLFSSL_SOCKET_IS_INVALID(clientfd)) std::cout << "tcp accept failed" << std::endl;

	CYASSL* ssl = 0;
	ssl = CyaSSL_new(ctx);
	if(ssl == NULL) std::cout << "SSL_new failed" << std::endl;
	CyaSSL_set_fd(ssl, clientfd);
	CyaSSL_SetTmpDH_file(ssl, dhParam, SSL_FILETYPE_PEM);
	if(CyaSSL_accept(ssl) != SSL_SUCCESS){
		std::cout << "SSL_accept failed\n";
		CyaSSL_free(ssl);
		CloseSocket(clientfd);
		//continue;
	}

	char buf[PACKET_SIZE];
	char reply[PACKET_SIZE];
	int sd, bytes;

	CyaSSL_read(ssl, buf, sizeof(buf));
	std::cout << "Client message: " << buf << std::endl;
	//sprintf(reply, "hello");
	FILE *fp = fopen(SSH_KEY, "r");
	fgets(reply, PACKET_SIZE, fp);
	CyaSSL_write(ssl, reply, sizeof(reply));

	ClientConfig* cc = new ClientConfig(ip, packet_data, "");

	CyaSSL_shutdown(ssl);
	CyaSSL_free(ssl);
	CloseSocket(clientfd);
	CloseSocket(sockfd);
	CyaSSL_CTX_free(ctx);

	CyaSSL_Cleanup();
	sleep(5);//wait 5 seconds before accepting new connections, to allow sockets time to reset
	}
}