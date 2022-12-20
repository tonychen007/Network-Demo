#include "../common/common.h"

const char* response =
"HTTP/1.1 200 OK\r\n"
"Connection: close\r\n"
"Content-Type: text/plain\r\n\r\n";

void initSSL(SSL_CTX** ctx) {
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	*ctx = SSL_CTX_new(TLS_server_method());

	if (!ctx) {
		fprintf(stderr, "SSL_CTX_new() failed.\n");
	}

	if (!SSL_CTX_use_certificate_file(*ctx, "cert.pem", SSL_FILETYPE_PEM)
		|| !SSL_CTX_use_PrivateKey_file(*ctx, "key.pem", SSL_FILETYPE_PEM)) {
		fprintf(stderr, "SSL_CTX_use_certificate_file() failed.\n");
		ERR_print_errors_fp(stderr);
		exit(-1);
	}
}

void setupSSL(SSL** ssl, SSL_CTX* ctx, const char* hostname, SOCKET& listenSocket) {

	while (1) {
		printf("Waiting for connection...\n");
		struct sockaddr_storage clientAddr;
		socklen_t clientLen = sizeof(clientAddr);
		SOCKET clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientLen);
		if (!ISVALIDSOCKET(clientSocket)) {
			fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
			exit(-1);
		}

		printf("Client is connected... ");
		char address_buffer[100];
		getnameinfo((struct sockaddr*)&clientAddr,
			clientLen, address_buffer, sizeof(address_buffer), 0, 0,
			NI_NUMERICHOST);
		printf("%s\n", address_buffer);

		*ssl = SSL_new(ctx);
		if (!ssl) {
			fprintf(stderr, "SSL_new() failed.\n");
			exit(-1);
		}

		SSL_set_fd(*ssl, clientSocket);
		if (SSL_accept(*ssl) == -1) {
			fprintf(stderr, "SSL_accept() failed.\n");
			ERR_print_errors_fp(stderr);

			SSL_shutdown(*ssl);
			CLOSESOCKET(clientSocket);
			SSL_free(*ssl);

			continue;
		}

		printf("SSL/TLS using %s\n", SSL_get_cipher(*ssl));

		char read1[128];
		char read2[128];

		time_t timer;
		time(&timer);
		ctime_s(read2, sizeof(read2), &timer);
		strcpy_s(read1, sizeof(read1), "Hi, I am SSL Server.");
		strcat_s(read1, read2);
		SSL_write(*ssl, response, strlen(response));
		SSL_write(*ssl, read1, strlen(read1));

		SSL_shutdown(*ssl);
		CLOSESOCKET(clientSocket);
		SSL_free(*ssl);
	}
}

// openssl req -x509 -newkey rsa:2048 -nodes -sha256 -keyout key.pem -out cert.pem -days 365
// test in browser https://127.0.0.1:443

void testSSLServer() {
	INIT_SOCK;

	SOCKET listenSocket;
	addrinfo* peerAddr;
	SSL_CTX* ctx;
	SSL* ssl;
	const char* hostname = 0;
	const char* port = "443";

	int bys = 0;
	int err;

	initSSL(&ctx);
	setupServerSocket(port, listenSocket);
	setupSSL(&ssl, ctx, hostname, listenSocket);

	DESTORY_SOCK;
}