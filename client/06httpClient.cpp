#include "../common/common.h"

void sendRequest(SOCKET& clientSocket, char* hostname, char* port, char* path) {
	char buffer[2048];
	int i;
	
	i = sprintf_s(buffer, sizeof(buffer), "GET /%s HTTP/1.1\r\n", path);
	i += sprintf_s(buffer + strlen(buffer), sizeof(buffer) - i, "Host: %s:%s\r\n", hostname, port);
	i += sprintf_s(buffer + strlen(buffer), sizeof(buffer) - i, "Connection: close\r\n");
	i += sprintf_s(buffer + strlen(buffer), sizeof(buffer) - i, "User-Agent: honpwc web_get 1.0\r\n");
	i += sprintf_s(buffer + strlen(buffer), sizeof(buffer) - i, "\r\n");

	send(clientSocket, buffer, strlen(buffer), 0);
	printf("Sent Headers:\n%s", buffer);
}

void httpClient(SOCKET& clientSocket) {
	int RESPONSE_SIZE = 65535;
	int encoding = 0;
	int remaining = 0;

	char* response = (char*)malloc(sizeof(wchar_t) * RESPONSE_SIZE);
	char* p = response, * q;
	char* end = response + RESPONSE_SIZE;
	char* body = 0;

	enum { 
		length, 
		chunked, 
		connection 
	};


	// recv
	while (1) {
		fd_set reads;
		FD_ZERO(&reads);
		FD_SET(clientSocket, &reads);
		int bys = 0;

		if (select(clientSocket + 1, &reads, 0, 0, 0) < 0) {
			fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
			exit(-1);
		}

		if (FD_ISSET(clientSocket, &reads)) {
			bys = recv(clientSocket, p, end - p, 0);
			if (bys < 1) {
				if (encoding == connection && body) {
					printf("%.*s", (int)(end - body), body);
				}

				printf("\nConnection closed by peer.\n");
				break;
	
			}

			p += bys;
			*p = 0;

			if (!body && (body = strstr(response, "\r\n\r\n"))) {
				*body = 0;
				body += 4;

				printf("Received Headers:\n%s\n", response);

				q = strstr(response, "\nContent-Length: ");
				if (q) {
					encoding = length;
					q = strchr(q, ' ');
					q += 1;
					remaining = strtol(q, 0, 10);

				}
				else {
					q = strstr(response, "\nTransfer-Encoding: chunked");
					if (q) {
						encoding = chunked;
						remaining = 0;
					}
					else {
						encoding = connection;
					}
				}
				printf("\nReceived Body:\n");
			}

			if (body) {
				if (encoding == length) {
					if (p - body >= remaining) {
						printf("%.*s", remaining, body);
						break;
					}
				}
				else if (encoding == chunked) {
					do {
						if (remaining == 0) {
							if ((q = strstr(body, "\r\n"))) {
								// fetch chunked size
								remaining = strtol(body, 0, 16);
								if (!remaining)
									goto finish;
								// skip 2 bytes 0d 0a
								body = q + 2;
							}
							else {
								break;
							}
						}
						if (remaining && p - body >= remaining) {
							printf("%.*s", remaining, body);
							body += remaining + 2; // 2 bytes for 0d 0a
							remaining = 0;
						}
					} while (!remaining);
				}
			} //if (body)
		}
	}

finish:
	free(response);
}



void testHttpClient(const char* url) {
	INIT_SOCK;

	SOCKET clientSocket;
	addrinfo* peerAddr;
	char *hostname, *port, * path;

	parseUrl(url, &hostname, &port, &path);
	setupClientSocket(url, port, clientSocket, &peerAddr);
	sendRequest(clientSocket, hostname, port, path);
	httpClient(clientSocket);

	free(hostname);
	free(path);

	DESTORY_SOCK;
}