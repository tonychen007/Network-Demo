#include "common.h"


wchar_t* char2wchar(const char* src) {
	int cp = GetACP();
	int size = MultiByteToWideChar(cp, 0, src, -1, NULL, 0);
	wchar_t* wstr = (wchar_t*)malloc(size * sizeof(wchar_t));
	if (utf8_to_wchar_no_alloc(cp, src, wstr, size) != size) {
		printf("Error to convert string :%s\n", src);
		wstr = NULL;
	}

	return wstr;
}


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

void setupSocket(const char* hostname, const char* port, SOCKET& peerSocket, struct addrinfo** addr, int isUDP) {
	if (hostname == 0)
		getBindAddr(port, addr, isUDP);
	else
		getRemoteAddr(hostname, port, addr, isUDP);
	peerSocket = socket((*addr)->ai_family, (*addr)->ai_socktype, (*addr)->ai_protocol);
	if (!ISVALIDSOCKET(peerSocket)) {
		printf("Socket failed %d\n", GETSOCKETERRNO());
		exit(-1);
	}
}

int setupServerSocket(const char* port, SOCKET& listenSocket, int isUDP, int nonBlock) {
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

	return ret;
}

int setupClientSocket(const char* hostname, const char* port,SOCKET& clientSocket, struct addrinfo** peerAddr, int isUDP, int nonBlock) {
	int ret;
	setupSocket(hostname, port, clientSocket, peerAddr, isUDP);

	clientSocket = socket((*peerAddr)->ai_family, (*peerAddr)->ai_socktype, (*peerAddr)->ai_protocol);

	if (nonBlock) {
		u_long nonBlock = 1;
		ioctlsocket(clientSocket, FIONBIO, &nonBlock);
	}

	if (!ISVALIDSOCKET(clientSocket)) {
		printf("socket() failed. (%d)\n", GETSOCKETERRNO());
		exit(-1);
	}

	if (!isUDP) {
		printf("Connecting...\n");
		if ((ret = connect(clientSocket, (*peerAddr)->ai_addr, (*peerAddr)->ai_addrlen))) {
			if (nonBlock) {
				if (GETSOCKETERRNO() != WSAEWOULDBLOCK) {
					printf("connect() failed. (%d)\n", GETSOCKETERRNO());
					exit(-1);
				}
			}
			else {
				printf("connect() failed. (%d)\n", GETSOCKETERRNO());
				exit(-1);
			}
		}
		printf("Connected.\n");
	}

	return ret;
}

void connectSSH(ssh_session& ssh, const char* hostname, int port, const char* user) {

	int verbosity = SSH_LOG_PROTOCOL;
#ifdef DEBUG
	ssh_options_set(ssh, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
#endif
	ssh_options_set(ssh, SSH_OPTIONS_HOST, hostname);
	ssh_options_set(ssh, SSH_OPTIONS_PORT, &port);
	ssh_options_set(ssh, SSH_OPTIONS_USER, user);

	int ret = ssh_connect(ssh);
	if (ret != SSH_OK) {
		fprintf(stderr, "ssh_connect() failed.\n%s\n", ssh_get_error(ssh));
		exit(-1);
	}

	printf("Connected to %s on port %d.\n", hostname, port);
	printf("Banner:\n%s\n", ssh_get_serverbanner(ssh));

	ssh_key key;
	if (ssh_get_server_publickey(ssh, &key) != SSH_OK) {
		fprintf(stderr, "ssh_get_server_publickey() failed.\n%s\n", ssh_get_error(ssh));
		exit(-1);
	}

	unsigned char* hash = 0;
	size_t hash_len = 0;
	printf("Host public key hash:\n"); ssh_print_hash(SSH_PUBLICKEY_HASH_SHA1, hash, hash_len);
	ssh_clean_pubkey_hash(&hash);
	ssh_key_free(key);

	printf("Checking ssh_session_is_known_server()\n");
	enum ssh_known_hosts_e known = ssh_session_is_known_server(ssh);
	switch (known) {
	case SSH_KNOWN_HOSTS_OK: printf("Host Known.\n"); break;
	case SSH_KNOWN_HOSTS_CHANGED: printf("Host Changed.\n"); break;
	case SSH_KNOWN_HOSTS_OTHER: printf("Host Other.\n"); break;
	case SSH_KNOWN_HOSTS_UNKNOWN: printf("Host Unknown.\n"); break;
	case SSH_KNOWN_HOSTS_NOT_FOUND: printf("No host file.\n"); break;
	case SSH_KNOWN_HOSTS_ERROR: printf("Host error. %s\n", ssh_get_error(ssh)); return;
	default: printf("Error. Known: %d\n", known); return;
	}

	if (known == SSH_KNOWN_HOSTS_CHANGED ||
		known == SSH_KNOWN_HOSTS_OTHER ||
		known == SSH_KNOWN_HOSTS_UNKNOWN ||
		known == SSH_KNOWN_HOSTS_NOT_FOUND) {
		printf("Do you want to accept and remember this host? Y/N\n");
		char answer[10];
		fgets(answer, sizeof(answer), stdin);
		if (answer[0] != 'Y' && answer[0] != 'y') {
			return;
		}

		ssh_session_update_known_hosts(ssh);
	}

	/*
	printf("Password: ");
	char password[128];
	fgets(password, sizeof(password), stdin);
	password[strlen(password) - 1] = 0;
	*/

	if (ssh_userauth_password(ssh, 0, "1") != SSH_AUTH_SUCCESS)
	{
		fprintf(stderr, "ssh_userauth_password() failed.\n%s\n", ssh_get_error(ssh));
		return;
	}
	else {
		printf("Authentication successful!\n");
	}
}