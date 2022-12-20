#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <libssh/libssh.h>

#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())

#define INIT_SOCK \
	WSADATA d;  \
	if (WSAStartup(MAKEWORD(2, 2), &d)) { \
		printf("Init socket failed.\n"); \
		exit(0); \
	} 

#define DESTORY_SOCK { WSACleanup(); }


#define utf8_to_wchar_no_alloc(cp, src, wdest, wdest_size) \
	MultiByteToWideChar(cp, 0, src, -1, wdest, wdest_size)

wchar_t* char2wchar(const char* src);

void getBindAddr(const char* port, struct addrinfo** bindAddr, int isUDP = 0);
void getRemoteAddr(const char* hostname, const char* port, struct addrinfo** addr, int isUDP = 0);
void parseUrl(const char* url, char** hostname, char** port, char** path);

void setupSocket(const char* hostname, const char* port, SOCKET & peerSocket, struct addrinfo** addr, int isUDP = 0);
int setupServerSocket(const char* port, SOCKET & listenSocket, int isUDP = 0, int nonBlock = 0);
int setupClientSocket(const char* hostname, const char* port, SOCKET & clientSocket, struct addrinfo** peerAddr = 0, int isUDP = 0, int nonBlock = 0);

void timeServerLogic(const char* port, SOCKET& listenSocket, SOCKET& clientSocket, int isOnce = 1);
void timeServerLogic0(SOCKET & listenSocket, SOCKET & clientSocket);
void selectClient(const char* port, SOCKET & clientSocket);
void selectServer(const char* port, SOCKET & listenSocket, SOCKET & clientSocket);


void testInitSocket();
void testInterfaceAddr();
void testTimeServer(int isOnce = 1);
void testSelectClient();
void testSelectServer();
void testUDPClient();
void testUDPServer();
void testResolveDNS(const char* hostname, const char* service);
void testHttpClient(const char* url);
void testHttpServer();
void testSendMail(const char* hostname, const char* port = "25");
void testSendMailWithAttachment(const char* hostname, const char* port = "25");
void testRecvMail(const char* hostname, const char* port = "110");

void testNonBlocking(const char* hostname);
void testServerIgnore();
void testSendBigData();