#include "../common/common.h"


void initSSL(SSL_CTX **ctx) {
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	*ctx = SSL_CTX_new(TLS_client_method());

	if (!ctx) {
		fprintf(stderr, "SSL_CTX_new() failed.\n");
		exit(-1);
	}
}

void setupSSL(SSL** ssl, SSL_CTX* ctx, const char* hostname, SOCKET& clientSocket) {
	*ssl = SSL_new(ctx);
	if (!ssl) {
		fprintf(stderr, "SSL_new() failed.\n");
		exit(-1);
	}

	if (!SSL_set_tlsext_host_name(*ssl, hostname)) {
		fprintf(stderr, "SSL_set_tlsext_host_name() failed.\n");
		ERR_print_errors_fp(stderr);
		exit(-1);
	}

	SSL_set_fd(*ssl, clientSocket);
	if (SSL_connect(*ssl) == -1) {
		fprintf(stderr, "SSL_connect() failed.\n");
		ERR_print_errors_fp(stderr);
		exit(-1);
	}

	printf("SSL/TLS using %s\n", SSL_get_cipher(*ssl));


	X509* cert = SSL_get_peer_certificate(*ssl);
	if (!cert) {
		fprintf(stderr, "SSL_get_peer_certificate() failed.\n");
		exit(-1);
	}

	char* tmp;
	if ((tmp = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0))) {
		printf("subject: %s\n", tmp);
		OPENSSL_free(tmp);
	}

	if ((tmp = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0))) {
		printf("issuer: %s\n", tmp);
		OPENSSL_free(tmp);
	}

	X509_free(cert);
}

void testSSLClient() {
	INIT_SOCK;

	SOCKET clientSocket;
	addrinfo* peerAddr;
	SSL_CTX* ctx;
	SSL* ssl;
	const char* hostname = "0.0.0.0";
	const char* port = "443";
	char read[4096];
	int bys = 0;
	int err;

	initSSL(&ctx);
	setupClientSocket(hostname, port, clientSocket, &peerAddr);
	setupSSL(&ssl, ctx, hostname, clientSocket);

	strcpy_s(read, sizeof(read), "Hi, I am ssl client\r\n");
	bys = SSL_write(ssl, read, strlen(read));
	printf("Sent %d bytes.\n", bys);

work:
	bys = SSL_read(ssl, read, 4096);
	if (bys < 1) {
		if ((err = SSL_get_error(ssl, bys)) &&
			(err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)) {
			//Just waiting on SSL, nothing to do.
			goto work;
		}
	}
	else {
		printf("Received (%d bytes): %.*s\n", bys, bys, read);
	}


	SSL_shutdown(ssl);
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	DESTORY_SOCK;
}