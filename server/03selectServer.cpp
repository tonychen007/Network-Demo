#include "../common/common.h"

void selectServer(const char* port, SOCKET& listenSocket, SOCKET& clientSocket) {
	fd_set master;
	FD_ZERO(&master);
	FD_SET(listenSocket, &master);
	SOCKET maxSocket = listenSocket;
	char buf[1024];
	int by = 0;

	printf("Waiting for connections...\n");

	while (1) {
		fd_set reads;
		reads = master;
		if (select(maxSocket + 1, &reads, 0, 0, 0) < 0) {
			printf("select() failed. (%d)\n", GETSOCKETERRNO());
			break;
		}
		
		for (int i = 0; i <= maxSocket; ++i) {
			if (FD_ISSET(i, &reads)) {

				// new client
				if (i == listenSocket) {
					struct sockaddr clientAddr;
					int clientlen = sizeof(clientAddr);
					clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientlen);
					if (!ISVALIDSOCKET(listenSocket)) {
						printf("accept() failed. (%d)\n", GETSOCKETERRNO());
						exit(-1);
					}

					FD_SET(clientSocket, &master);
					if (clientSocket > maxSocket)
						maxSocket = clientSocket;

					char addrBuf[128];
					getnameinfo(
						(struct sockaddr*)&clientAddr, clientlen, 
						addrBuf, sizeof(addrBuf), 0, 0, NI_NUMERICHOST);
					printf("New connection from %s\n", addrBuf);
				}
				// already connected client
				else {
					memset(buf, 0, sizeof(buf));
					by = recv(i, buf, sizeof(buf), 0);
					if (by < 1) {
						FD_CLR(i, &master);
						CLOSESOCKET(i);
						continue;
					}
					
					send(i, buf, sizeof(buf), 0);
				}
			} // FD_ISSET
		} // for 
	} // while

	printf("Closing listening socket...\n");
	CLOSESOCKET(listenSocket);
}

void testSelectServer() {
	INIT_SOCK
	
	const char* port = "8080";
	SOCKET listenSocket;
	SOCKET clientSocket;

	setupServerSocket(port, listenSocket);
	selectServer(port, listenSocket, clientSocket);

	DESTORY_SOCK
}