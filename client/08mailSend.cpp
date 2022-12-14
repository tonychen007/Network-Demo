#include "../common/common.h"
#include "../common/base64.h"

#include <ctype.h>
#include <stdarg.h>

#define MAXINPUT 512
#define MAXRESPONSE  1024
#define BUFFER_SIZE  1024
const char BOUNDARY_TEXT[] = "__MESSAGE__ID__1";

void sendFormat(SOCKET server, const char* text, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, text);
    vsprintf_s(buffer, sizeof(buffer), text, args);
    va_end(args);

    int r = send(server, buffer, strlen(buffer), 0);

    printf("C: %s", buffer);
}

int parseResponse(const char* response) {
    const char* k = response;
    if (!k[0] || !k[1] || !k[2]) return 0;
    for (; k[3]; ++k) {
        if (k == response || k[-1] == '\n') {
            if (isdigit(k[0]) && isdigit(k[1]) && isdigit(k[2])) {
                if (k[3] != '-') {
                    if (strstr(k, "\r\n")) {
                        return strtol(k, 0, 10);
                    }
                }
            }
        }
    }
    return 0;
}

void waitOnResponse(SOCKET server, int expecting) {
    char response[MAXRESPONSE + 1];
    char* p = response;
    char* end = response + MAXRESPONSE;

    int code = 0;

    do {
        int bytes_received = recv(server, p, end - p, 0);
        if (bytes_received < 1) {
            fprintf(stderr, "Connection dropped.\n");
            exit(1);
        }

        p += bytes_received;
        *p = 0;

        if (p == end) {
            fprintf(stderr, "Server response too large:\n");
            fprintf(stderr, "%s", response);
            exit(1);
        }

        code = parseResponse(response);

    } while (code == 0);

    if (code != expecting) {
        fprintf(stderr, "Error from server:\n");
        fprintf(stderr, "%s", response);
        exit(1);
    }

    printf("S: %s", response);
}

void sendMail0(const char* hostname, const char* port, int isAttachment = 0, int isHTML = 0) {
    SOCKET clientSocket;
    addrinfo* peerAddr;
    setupClientSocket(hostname, port, clientSocket, &peerAddr);
    
    waitOnResponse(clientSocket, 220);
    sendFormat(clientSocket, "HELO HONPWC\r\n");
    waitOnResponse(clientSocket, 250);

    // auth
    sendFormat(clientSocket, "AUTH LOGIN\r\n");
    waitOnResponse(clientSocket, 334);

    const char* username = "xx@xx.com";
    const char* password = "xxxx";
    char* p1 = base64_encode((unsigned char*)username, strlen(username));
    char* p2 = base64_encode((unsigned char*)password, strlen(password));

    sendFormat(clientSocket, "%s\r\n", p1);
    waitOnResponse(clientSocket, 334);
    sendFormat(clientSocket, "%s\r\n", p2);
    waitOnResponse(clientSocket, 235);

    // send info
    const char* sender = "xx@xx.com";
    sendFormat(clientSocket, "MAIL FROM:<%s>\r\n", sender);
    waitOnResponse(clientSocket, 250);

    // to,cc,bcc all in here
    const char* recipient = "xx@xx.com";
    // to
    sendFormat(clientSocket, "RCPT TO:<%s>\r\n", sender);
    waitOnResponse(clientSocket, 250);
    sendFormat(clientSocket, "RCPT TO:<%s>\r\n", recipient);
    waitOnResponse(clientSocket, 250);
    // cc
    sendFormat(clientSocket, "RCPT TO:<%s>\r\n", recipient);
    waitOnResponse(clientSocket, 250);
    sendFormat(clientSocket, "RCPT TO:<%s>\r\n", recipient);
    waitOnResponse(clientSocket, 250);
    // bcc
    sendFormat(clientSocket, "RCPT TO:<%s>\r\n", recipient);
    waitOnResponse(clientSocket, 250);
    sendFormat(clientSocket, "RCPT TO:<%s>\r\n", recipient);
    waitOnResponse(clientSocket, 250);

    sendFormat(clientSocket, "DATA\r\n");
    waitOnResponse(clientSocket, 354);

    // header
    const char* subject = "Test title";
    sendFormat(clientSocket, "From:%s\r\n", "Tony Chen<lxx@xx.com>");
    sendFormat(clientSocket, "To:%s\r\n", "tony1<xx@xx.com>,tony2<xx@xx.com>");
    sendFormat(clientSocket, "Cc:%s\r\n", "tony3<xx@xx.com>,tony4<xx@xx.com>");
    sendFormat(clientSocket, "Bcc:%s\r\n", "tony5<xx@xx.com>,tony6<xx@xx.com>");
    sendFormat(clientSocket, "Subject:%s\r\n", subject);
    const char* body = "<h1>Hi Tony:</h1>\n\n<div style=\"color:red\">Welcome to our pary.</div>\n<p>Have a good time.</p><p>Yours Sincerely,</p><p>Tom.</p>";
    
    char SendBuf[BUFFER_SIZE] = { '\0' };
    
    strcat_s(SendBuf, "MIME-Version: 1.0\r\n");
    if (isAttachment) {
        strcat_s(SendBuf, "Content-Type: multipart/mixed; boundary=\"");
        strcat_s(SendBuf, BOUNDARY_TEXT);
        strcat_s(SendBuf, "\"\r\n");
        strcat_s(SendBuf, "\r\n");
        strcat_s(SendBuf, "--");
        strcat_s(SendBuf, BOUNDARY_TEXT);
        strcat_s(SendBuf, "\r\n");
    }
    if (isHTML)
        strcat_s(SendBuf, "Content-type: text/html; charset=UTF-8\r\n");
    else
        strcat_s(SendBuf, "Content-type: text/plain; charset=UTF-8\r\n");
    strcat_s(SendBuf, "Content-Transfer-Encoding: 7bit\r\n");
    strcat_s(SendBuf, "\r\n");
    sendFormat(clientSocket, "%s", SendBuf);
    sendFormat(clientSocket, "%s\r\n", body);

    if (isAttachment) {
        char szFileName[512] = { '\0' };
        const char* attach = "1.jpg";
        strcat_s(szFileName, "=?UTF-8?B?");
        p1 = base64_encode((unsigned char*)attach, strlen(attach));
        strcat_s(szFileName, p1);
        strcat_s(szFileName, "?=");

        snprintf(SendBuf, BUFFER_SIZE, "--%s\r\n", BOUNDARY_TEXT);
        strcat_s(SendBuf, "Content-Type: application/x-msdownload; name=\"");
        strcat_s(SendBuf, szFileName);
        strcat_s(SendBuf, "\"\r\n");
        strcat_s(SendBuf, "Content-Transfer-Encoding: base64\r\n");
        strcat_s(SendBuf, "Content-Disposition: attachment; filename=\"");
        strcat_s(SendBuf, szFileName);
        strcat_s(SendBuf, "\"\r\n");
        strcat_s(SendBuf, "\r\n");
        sendFormat(clientSocket, "%s", SendBuf);

        // read file
        int bb = 55;
        FILE* f;
        unsigned char* fileBuf = (unsigned char*)malloc(bb);
        fopen_s(&f, attach, "rb");
        unsigned int fileSize = 0;
        unsigned int res = 0;
        unsigned long part = 0;
        if (f != NULL) {
            // get file size:
            fseek(f, 0, SEEK_END);
            fileSize = ftell(f);
            fseek(f, 0, SEEK_SET);
        }
        for (int i = 0; i < fileSize / (bb - 1) + 1; i++) {
            res = fread(fileBuf, sizeof(char), (bb - 1), f);
            char* pb64 = base64_encode(fileBuf, res);
            if (part == 0) {
                strcpy_s(SendBuf, pb64);
            }
            else {
                strcat_s(SendBuf, pb64);
            }
            strcat_s(SendBuf, "\r\n");
            part += res + 2;
            if (part >= BUFFER_SIZE / 2) {
                part = 0;
                sendFormat(clientSocket, "%s", SendBuf);
            }
        }
        if (part) {
            sendFormat(clientSocket, "%s", SendBuf);
        }
        fclose(f);
        f = NULL;

        snprintf(SendBuf, BUFFER_SIZE, "\r\n--%s--\r\n", BOUNDARY_TEXT);
        sendFormat(clientSocket, "%s", SendBuf);
    }

    // !!! IMPORTANT, DO NOT FORGET send "."
    sendFormat(clientSocket, "%s","\r\n.\r\n");
    waitOnResponse(clientSocket, 250);
    sendFormat(clientSocket, "QUIT\r\n");
    waitOnResponse(clientSocket, 221);
}

void testSendMail(const char* hostname, const char* port) {
    INIT_SOCK;

    sendMail0(hostname, port, 0, 1);

    DESTORY_SOCK;
}

void testSendMailWithAttachment(const char* hostname, const char* port) {
    INIT_SOCK;

    sendMail0(hostname, port, 1, 1);

    DESTORY_SOCK;
}