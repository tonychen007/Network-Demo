#include "../common/common.h"

// test with netcat -u host port
void testUDPServer() {
	INIT_SOCK

	const char* hostname = 0;
	const char* port = "8080";
	SOCKET listenSocket;
	struct sockaddr cliAddr;
	int cliAddrLen = sizeof(cliAddr);
	int bys = 0;
	char buf[1024];

	setupServerSocket(port, listenSocket, 1);
	bys = recvfrom(listenSocket, buf, sizeof(buf), 0, &cliAddr, &cliAddrLen);
	buf[bys] = '\0';
	printf("Recv %d bytes. %s\n", bys, buf);

	getnameinfo(&cliAddr, cliAddrLen,
		buf, sizeof(buf), 0, 0, NI_NUMERICHOST);
	printf("Client address is %s\n", buf);

	DESTORY_SOCK
}