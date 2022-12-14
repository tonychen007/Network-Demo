#include "../common/common.h"

// some machine: nc -ul 8080
void testUDPClient() {
	INIT_SOCK

	const char* hostname = "192.168.232.129";
	const char* port = "8080";
	SOCKET clientSocket;
	addrinfo* peerAddr;
	int bys = 0;
	char buf[1024] = "Hi, Tony!";

	setupClientSocket(hostname, port, clientSocket, &peerAddr, 1);
	bys = sendto(clientSocket, buf, strlen(buf), 0, peerAddr->ai_addr, peerAddr->ai_addrlen);
	DESTORY_SOCK
}