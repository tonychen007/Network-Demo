#include "../common/common.h"

#define MAX_REQUEST_SIZE 2047

const char* getContenType(const char* path) {
    const char* last_dot = strrchr(path, '.');
    if (last_dot) {
        if (strcmp(last_dot, ".css") == 0) return "text/css";
        if (strcmp(last_dot, ".csv") == 0) return "text/csv";
        if (strcmp(last_dot, ".gif") == 0) return "image/gif";
        if (strcmp(last_dot, ".htm") == 0) return "text/html";
        if (strcmp(last_dot, ".html") == 0) return "text/html";
        if (strcmp(last_dot, ".ico") == 0) return "image/x-icon";
        if (strcmp(last_dot, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".jpg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".js") == 0) return "application/javascript";
        if (strcmp(last_dot, ".json") == 0) return "application/json";
        if (strcmp(last_dot, ".png") == 0) return "image/png";
        if (strcmp(last_dot, ".pdf") == 0) return "application/pdf";
        if (strcmp(last_dot, ".svg") == 0) return "image/svg+xml";
        if (strcmp(last_dot, ".txt") == 0) return "text/plain";
    }

    return "application/octet-stream";
}

void serveResource(const char* path, char(*buffer)[4096], FILE** fp) {
    printf("serve_resource %s\n", path);

    if (strcmp(path, "/") == 0) path = "/index.html";

    if (strlen(path) > 100 || strstr(path, "..")) {
        return;
    }

    char full_path[128];
    sprintf_s(full_path, sizeof(full_path), "public%s", path);

#if defined(_WIN32)
    char* p = full_path;
    while (*p) {
        if (*p == '/') *p = '\\';
        ++p;
    }
#endif
    
    fopen_s(fp, full_path, "rb");
    if (!*fp) {
        return;
    }

    fseek(*fp, 0L, SEEK_END);
    size_t cl = ftell(*fp);
    rewind(*fp);

    const char* ct = getContenType(full_path);
    int i;

    i = sprintf_s(*buffer, sizeof(*buffer), "HTTP/1.1 200 OK\r\n");
    i += sprintf_s(*buffer + strlen(*buffer), sizeof(*buffer) - i, "Connection: close\r\n");
    i += sprintf_s(*buffer + strlen(*buffer), sizeof(*buffer) - i, "Content-Length: %u\r\n", cl);
    i += sprintf_s(*buffer + strlen(*buffer), sizeof(*buffer) - i, "Content-Type: %s\r\n", ct);
    i += sprintf_s(*buffer + strlen(*buffer), sizeof(*buffer) - i, "\r\n");
}

void serveResource(SOCKET clientSocket, const char* path, int isClose = 1) {
#define BSIZE 4096

    char buffer[BSIZE] = { 0 };
    FILE* fp;

    serveResource(path, &buffer, &fp);
    send(clientSocket, buffer, strlen(buffer), 0);

    if (fp) {
        int r = fread(buffer, 1, BSIZE, fp);
        while (r) {
            send(clientSocket, buffer, r, 0);
            r = fread(buffer, 1, BSIZE, fp);
        }

        fclose(fp);
    }

    if (isClose)
        CLOSESOCKET(clientSocket);
}

void httpSimpleServer(SOCKET listenSocket) {
	SOCKET clientSocket;
    char req[2048];

	while (1) {
		clientSocket = accept(listenSocket, 0, 0);
        int r = recv(clientSocket, req, sizeof(req), 0);
        if (r < 1) {
            continue;
        }
        char* path = req + 4;
        char* endPath = strstr(path, " ");
        *endPath = 0;
        serveResource(clientSocket, path);
		CLOSESOCKET(clientSocket);
	}
}

void httpSelectServer(SOCKET listenSocket) {
    SOCKET clientSocket;
    char req[2048];

    fd_set master;
    FD_ZERO(&master);
    FD_SET(listenSocket, &master);
    SOCKET maxSocket = listenSocket;

    struct clientInfo* clientList = 0;

    while (1) {
        fd_set reads;
        reads = master;
        if (select(maxSocket + 1, &reads, 0, 0, 0) < 0) {
            printf("select() failed. (%d)\n", GETSOCKETERRNO());
            break;
        }
        for (int i = 1; i <= maxSocket; ++i) {
            if (FD_ISSET(i, &reads)) {
                if (i == listenSocket) {
                    clientSocket = accept(listenSocket, 0, 0);
                    if (!ISVALIDSOCKET(listenSocket)) {
                        printf("accept() failed. (%d)\n", GETSOCKETERRNO());
                        exit(-1);
                    }
                    
                    FD_SET(clientSocket, &master);
                    if (clientSocket > maxSocket) {
                        maxSocket = clientSocket;
                    }
                }
                else {
                    int r = recv(i, req, sizeof(req), 0);
                    if (r < 1) {
                        FD_CLR(i, &master);
                        CLOSESOCKET(i);
                        continue;
                    }

                    char* path = req + 4;
                    char* endPath = strstr(path, " ");
                    *endPath = 0;
                    serveResource(i, path, 0);
                }
            }
        }
    }

    CLOSESOCKET(listenSocket);
}

void testHttpServer() {
	INIT_SOCK;

	const char* port = "80";
	SOCKET listenSocket;

	setupServerSocket(port, listenSocket);
    //httpSimpleServer(listenSocket);
    httpSelectServer(listenSocket);


	DESTORY_SOCK;
}