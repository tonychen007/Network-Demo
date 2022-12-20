#include "../common/common.h"

void serverLogic(SOCKET& listenSocket) {
	SOCKET clientSocket;

	while (1) {
		sockaddr clientAddr;
		int cliLen = sizeof(clientAddr);
		clientSocket = accept(listenSocket, &clientAddr, &cliLen);
		if (!ISVALIDSOCKET(clientSocket)) {
			fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
			break;
		}

		printf("Ingore client, do not call recv\n");
	}
}

void testServerIgnore() {
	INIT_SOCK;

	const char* port = "8080";
	SOCKET listenSocket;

	setupServerSocket(port, listenSocket);
	serverLogic(listenSocket);

	DESTORY_SOCK;
}