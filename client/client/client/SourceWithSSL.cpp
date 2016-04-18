#pragma comment(lib, "Ws2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <winsock2.h>
#include <string>
#include <iostream>
#include <atlbase.h>
#include <Windows.h>
#include "includes.h"
#include <WS2tcpip.h>
#include <cstdlib>
#include <conio.h>
#include <process.h>
#include <cyassl/ctaocrypt/settings.h>
#include <cyassl/ssl.h>
#include <cyassl/openssl/ssl.h>
#include <cyassl/test.h>
#include <tchar.h>
/*#include <libssh/server.h>
#include <libssh/libssh.h>
#include <libssh/callbacks.h>*/
bool received_response = false;
SOCKADDR_IN server;

DWORD WINAPI ReceiveResponse(LPVOID lpArg)
{

	struct addrinfo *result = NULL, *ptr = NULL, hints;
	int iResult;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, RECEIVE_PORT, &hints, &result);
	if (iResult != 0) {
		std::cout << "getaddrinfo failed " << iResult << std::endl;
		return -1;
	}

	SOCKET ListenSocket = INVALID_SOCKET;

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		return -1;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		return -1;
	}

	freeaddrinfo(result);

	std::cout << "about to listen" << std::endl;

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		return -1;
	}

	SOCKET ClientSocket;

	ClientSocket = INVALID_SOCKET;

	// Accept a client socket
	std::cout << "about to accept" << std::endl;
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		return -1;
	}

	std::cout << "accepted" << std::endl;

	int namelen = sizeof(server);
	
	getpeername(ClientSocket, (sockaddr*)&server, &namelen);

	received_response = true;

}

void init(void) {
	memset(&server, 0, sizeof(server));
	FILE* fp = fopen("status.txt", "w+");
	fprintf(fp, "connecting\n");
	fclose(fp);
}

int main() {

	init();


	int portno = UDP_BROADCAST_PORT;
	USES_CONVERSION;

	WORD w = MAKEWORD(1, 1);
	WSADATA wsadata;
	::WSAStartup(w, &wsadata);

	SOCKET s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == -1)
	{
		std::cout << "Error in creating socket";
		return 0;
	}
	char opt = 1;
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(char));
	SOCKADDR_IN brdcastaddr;
	memset(&brdcastaddr, 0, sizeof(brdcastaddr));
	brdcastaddr.sin_family = AF_INET;
	brdcastaddr.sin_port = htons(portno);
	brdcastaddr.sin_addr.s_addr = INADDR_BROADCAST;
	int len = sizeof(brdcastaddr);
	char sbuf[PACKET_SIZE];
	int sbuf_size = PACKET_SIZE;
	GetUserName(sbuf, (LPDWORD)&sbuf_size);
	/*for (int i = 0; i < 1024; i++) {
	sbuf[i] = 'a';
	}*/

	int *lpArgPtr;
	lpArgPtr = (int *)malloc(sizeof(int));
	*lpArgPtr = 0;
	DWORD ThreadId;
	HANDLE h = CreateThread(NULL, 0, ReceiveResponse, lpArgPtr, 0, &ThreadId);
	if (h == NULL) {
		std::cout << "Could not create thread!" << std::endl;
		return -1;
	}

	while (!received_response) {
		int ret = sendto(s, sbuf, strlen(sbuf) + 1, 0, (sockaddr*)&brdcastaddr, len);
		if (ret < 0)
		{
			std::cout << "Error broadcasting to the clients";
		}
		else if (ret < strlen(sbuf))
		{
			std::cout << "Not all data broadcasted to the clients";
		}
		else {
			std::cout << "Broadcasting is done" << std::endl;
		}
		Sleep(3000);
	}
	std::cout << "received from " << inet_ntoa(server.sin_addr) << std::endl;

	/*int x;
	std::cin >> x;*/

	//now we're going to do the whole ssl handshake so we can receive a public key for the ssh
	//access to occur on

	CyaSSL_Init();

	ChangeToWolfRoot();

	SSL_METHOD* method = CyaSSLv23_client_method();
	SSL_CTX* ctx = SSL_CTX_new(method);
	if (SSL_CTX_load_verify_locations(ctx, CHAIN_CERT, 0) != SSL_SUCCESS)
		std::cout << "can't load ca file" << std::endl;

	SSL_CTX_set_default_passwd_cb(ctx, PasswordCallBack);

	SSL* ssl = SSL_new(ctx);
	SOCKET sockfd;
	word16 port = HANDSHAKE_PORT;
	int doDTLS = 0;
	tcp_connect(&sockfd, inet_ntoa(server.sin_addr), port, doDTLS, ssl);
	SSL_set_fd(ssl, sockfd);

	if (SSL_connect(ssl) != SSL_SUCCESS) {
		std::cout << "SSL_connect failed" << std::endl;
		int x;
		std::cin >> x;
	}

	sprintf(sbuf, "handshake");
	if (SSL_write(ssl, sbuf, sizeof(sbuf)) != sizeof(sbuf)) std::cout << "SSL_write failed" << std::endl;

	char rbuf[PACKET_SIZE];
	SSL_read(ssl, rbuf, sizeof(rbuf));

	std::cout << "received message: " << rbuf << std::endl;

	FILE* fp = fopen(SSH_KEYS, "a");
	fprintf(fp, "\n");
	fprintf(fp, rbuf);
	fclose(fp);


	std::cout << "SSL finished" << std::endl;
	int x;
	std::cin >> x;
	
	SSL_shutdown(ssl);
	SSL_free(ssl);
	SSL_CTX_free(ctx);

	CloseSocket(sockfd);

	CyaSSL_Cleanup();


	fp = fopen("web_interface.url", "w+");
	fprintf(fp, "[InternetShortcut]");
	fprintf(fp, "\n");
	fprintf(fp, "URL=");
	fprintf(fp, inet_ntoa(server.sin_addr));
	GetUserName(sbuf, (LPDWORD)&sbuf_size);
	fprintf(fp, "/index.php?user=%s", sbuf);
	fclose(fp);

	fp = fopen("status.txt", "w+");
	fprintf(fp, "ready\n");
	fclose(fp);
	/*
	ssh_bind sshbind;

	ssh_bind_options_set(sshbind);*/

}