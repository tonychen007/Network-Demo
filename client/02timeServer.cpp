#include "../common/common.h"

void timeServerLogic(const char* port, SOCKET& listenSocket, SOCKET& clientSocket, int isOnce) {

	if (!isOnce) {
		while (1) {
			timeServerLogic0(listenSocket, clientSocket);
		}
	}
	else {
		timeServerLogic0(listenSocket, clientSocket);
	}

	CLOSESOCKET(listenSocket);
}

void timeServerLogic0(SOCKET& listenSocket, SOCKET& clientSocket) {
	struct sockaddr clientAddr;
	int clientSocketLen;
	char buf[1024];
	char timeBuf[32];

	const char* response =
		"HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n\r\n"
		"Local time is: ";

	printf("Waiting for connection...\n");
	memset(&clientAddr, 0, sizeof(sockaddr));
	clientSocketLen = sizeof(clientAddr);
	clientSocket = accept(listenSocket, &clientAddr, &clientSocketLen);
	if (!ISVALIDSOCKET(clientSocket)) {
		printf("Socket accept failed %d\n", GETSOCKETERRNO());
		exit(-1);
	}

	printf("Client is connected...\n");
	getnameinfo(&clientAddr, clientSocketLen, buf, sizeof(buf), 0, 0, NI_NUMERICHOST);
	printf("%s\n", buf);

	// coming from browser, it's HTTP. So, recv get request first
	recv(clientSocket, buf, sizeof(buf), 0);
	printf("Recv %s\n", buf);

	time_t timer;
	time(&timer);
	ctime_s(timeBuf, sizeof(timeBuf), &timer);

	strcpy_s(buf, response);
	strcat_s(buf, timeBuf);
	send(clientSocket, buf, strlen(buf), 0);

	CLOSESOCKET(clientSocket);
}

void testTimeServer(int isOnce) {
	INIT_SOCK

	const char* port = "8080";
	SOCKET listenSocket;
	SOCKET clientSocket;

	setupServerSocket(port, listenSocket);
	timeServerLogic(port, listenSocket, clientSocket, isOnce);

	DESTORY_SOCK
}