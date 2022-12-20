#include "../common/common.h"

void nonBlocking(SOCKET& clientSocket, int ret) {

	u_long nonblock = 0;
	ioctlsocket(clientSocket, FIONBIO, &nonblock);

	if (ret == 0) {
		printf("Already connected.\n");
		printf("Perhaps non-blocking failed?\n");
	}
	else {
		fd_set set;
		FD_ZERO(&set);
		FD_SET(clientSocket, &set);

		struct timeval timeout;
		timeout.tv_sec = 5; timeout.tv_usec = 0;
		select(clientSocket + 1, 0, &set, 0, &timeout);
	}

	printf("Testing for connection...\n");
	if (send(clientSocket, "a", 1, 0) < 0) {
		printf("First send() failed. Connection was not successful.\n");
	}
	else {
		printf("First send() succeeded. Connection was successful.\n");
	}
}

void sendBigData(SOCKET& clientSocket) {
	printf("Connected.\n");
	printf("Sending lots of data.\n");

	const int size = 1024*4;
	char buffer[size] = { 0 };
	int write;
	int i = 1;

	 //for (int i = 1; i <= size; ++i) {
	while(1) {
		write = 0;
		fd_set set;
		FD_ZERO(&set);
		FD_SET(clientSocket, &set);

		struct timeval timeout;
		timeout.tv_sec = 0; timeout.tv_usec = 0;
		select(clientSocket + 1, 0, &set, 0, &timeout);

		if (FD_ISSET(clientSocket, &set)) {
			//printf("Socket is ready to write.\n");
			write = 1;
		}
		else {
			//printf("Socket is not ready to write.\n");
		}

		if (!write)
			continue;
	
		printf("Sending %lld KB (%lld total KB).\n", size >> 10, (i * size) >> 10);
		int r = send(clientSocket, buffer, size, 0);
		if (r < 0) {
			fprintf(stderr, "send() failed. (%d)\n", GETSOCKETERRNO());
			break;
		}
		if (r != size) {
			printf("send() only consumed %d bytes.\n", r);
			break;
		}
		i++;
	}
}

void testNonBlocking(const char* hostname) {
	INIT_SOCK;

	const char* port = "80";
	SOCKET clientSocket;
	addrinfo* peerAddr;
	int ret;

	ret = setupClientSocket(hostname, port, clientSocket, &peerAddr, 0, 1);
	nonBlocking(clientSocket, ret);

	DESTORY_SOCK;
}

void testSendBigData() {
	INIT_SOCK;

	const char* port = "8080";
	SOCKET clientSocket;
	addrinfo* peerAddr;

	// nc -l on remote machine
	setupClientSocket("192.168.232.129", port, clientSocket, &peerAddr);
	sendBigData(clientSocket);

	DESTORY_SOCK;
}