#include "common.h"

void getBindAddr(const char* port, struct addrinfo** bindAddr, int isUDP) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = !isUDP ? SOCK_STREAM : SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(0, port, &hints, bindAddr);
}

void getRemoteAddr(const char* hostname, const char* port, struct addrinfo** addr, int isUDP) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = !isUDP ? SOCK_STREAM : SOCK_DGRAM;

	getaddrinfo(hostname, port, &hints, addr);

	printf("Remote address is: ");
	char addrBuf[128];
	char srvBuf[128];
	getnameinfo((*addr)->ai_addr, (*addr)->ai_addrlen,
		addrBuf, sizeof(addrBuf),
		srvBuf, sizeof(srvBuf),
		NI_NUMERICHOST);
	printf("%s %s\n", addrBuf, srvBuf);
}

void parseUrl(const char* url, char** hostname, char** port, char** path) {
	char buf[256] = { '\0' };
	char* proto = 0;
	memcpy(buf, url, strlen(url));

	char *p = strstr(buf, "://");
	if (p) {
		proto = buf;
		*p = 0;
		p += 3;  // "://"
	}
	else {
		p = (char*)buf;
	}
	
	if (proto) {
		if (strcmp(proto, "http")) {
			fprintf(stderr, "Unknown protocol '%s'. Only 'http' is supported.\n", proto);
			exit(1);
		}
	}

	*hostname = (char*)malloc(256);
	memcpy(*hostname, p, strlen(p) + 1);
	
	while (*p && *p != ':' && *p != '/' && *p != '#') ++p;

	*port = (char*)"80";
	if (*p == ':') {
		*p = 0;
		p++;
		*port = p;
	}
	while (*p && *p != '/' && *p != '#') ++p;
	*p = 0;
	p++;

	*path = (char*)malloc(256);
	memcpy(*path, p, strlen(p) + 1);
	if (*p == '/') {
		*path = p + 1;
	}

	while (*p && *p != '#') ++p;
	if (*p == '#') *p = 0;

	printf("hostname: %s\n", *hostname);
	printf("port: %s\n", *port);
	printf("path: %s\n", *path);
}

void setupSocket(const char* hostname, const char* port, SOCKET& listenSocket, struct addrinfo** addr, int isUDP) {
	if (hostname == 0)
		getBindAddr(port, addr, isUDP);
	else
		getRemoteAddr(hostname, port, addr, isUDP);
	listenSocket = socket((*addr)->ai_family, (*addr)->ai_socktype, (*addr)->ai_protocol);
	if (!ISVALIDSOCKET(listenSocket)) {
		printf("Socket failed %d\n", GETSOCKETERRNO());
		exit(-1);
	}
}

void setupServerSocket(const char* port, SOCKET& listenSocket, int isUDP) {
	int ret;
	struct addrinfo* bindAddr = 0;

	setupSocket("0.0.0.0", port, listenSocket, &bindAddr, isUDP);

	ret = bind(listenSocket, bindAddr->ai_addr, bindAddr->ai_addrlen);
	if (ret) {
		printf("Socket bind failed %d\n", GETSOCKETERRNO());
		exit(-1);
	}
	freeaddrinfo(bindAddr);

	if (!isUDP) {
		printf("Listening...\n");
		ret = listen(listenSocket, 10);
		if (ret) {
			printf("Socket listen failed %d\n", GETSOCKETERRNO());
			exit(-1);
		}
	}
}

void setupClientSocket(const char* hostname, const char* port,SOCKET& clientSocket, struct addrinfo** peerAddr, int isUDP) {
	setupSocket(hostname, port, clientSocket, peerAddr, isUDP);

	clientSocket = socket((*peerAddr)->ai_family, (*peerAddr)->ai_socktype, (*peerAddr)->ai_protocol);

	if (!ISVALIDSOCKET(clientSocket)) {
		printf("socket() failed. (%d)\n", GETSOCKETERRNO());
		exit(-1);
	}

	if (!isUDP) {
		printf("Connecting...\n");
		if (connect(clientSocket, (*peerAddr)->ai_addr, (*peerAddr)->ai_addrlen)) {
			printf("connect() failed. (%d)\n", GETSOCKETERRNO());
			exit(-1);
		}
		printf("Connected.\n");
	}
	else {

	}
}