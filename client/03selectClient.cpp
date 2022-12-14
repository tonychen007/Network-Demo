#include "../common/common.h"

void selectClient(const char* port, SOCKET& clientSocket) {
	char read[4096];

	while (1) {
		fd_set reads;
		FD_ZERO(&reads);
		FD_SET(clientSocket, &reads);

		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;

		if (select(clientSocket + 1, &reads, 0, 0, &timeout) < 0) {
			printf("select() failed. (%d)\n", GETSOCKETERRNO());
			break;
		}

		if (FD_ISSET(clientSocket, &reads)) {
			
			int bytes_received = recv(clientSocket, read, sizeof(read), 0);
			if (bytes_received < 1) {
				printf("Connection closed by peer.\n");
				break;
			}
			printf("Received (%d bytes): %.*s", bytes_received, bytes_received, read);
		}
		
		if (_kbhit()) {
			if (!fgets(read, sizeof(read), stdin)) break;
			printf("Sending: %s", read);
			int bytes_sent = send(clientSocket, read, strlen(read), 0);
			printf("Sent %d bytes.\n", bytes_sent);
		}
	}

	printf("Closing socket...\n");
	CLOSESOCKET(clientSocket);
}

void testSelectClient() {
	INIT_SOCK

	const char* hostname = 0;
	const char* port = "8080";
	SOCKET clientSocket;
	addrinfo* peerAddr;

	setupClientSocket(hostname, port, clientSocket, &peerAddr);
	selectClient(port, clientSocket);

	DESTORY_SOCK
}