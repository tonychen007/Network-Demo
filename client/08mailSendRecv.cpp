
#include "../common/common.h"
#include "../common/base64.h"

#include <ctype.h>
#include <stdarg.h>
#include <stack>
#include <string>
using namespace std;

#define MAXINPUT 512

const int MAXRESPONSE = 4096;
const int BUFFER_SIZE = 4096;
const char BOUNDARY_TEXT[] = "__MESSAGE__ID__1";
const char* OUTPUT_PATH = "z:\\";

const char* username = "xxxxxx@211.com";
const char* password = "xxxxx";

void sendFormat(SOCKET server, const char* text, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, text);
    vsprintf_s(buffer, sizeof(buffer), text, args);
    va_end(args);

    int r = send(server, buffer, strlen(buffer), 0);
}

int parseResponse(const char* response, int isNeed = 1) {
    const char* k = response;
    if (!isNeed) {
        return -1;
    }

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
            else {
                if (k[0] == '+' && k[1] == 'O' && k[2] == 'K') {
                    return 200;
                }
                else {
                    return -1;
                }
            }
        }
    }
    return -1;
}

int waitOnResponse(SOCKET server, int expecting, char* out = NULL, int outSize = 0, int isNeed = 1) {
    int buflen = MAXRESPONSE;
    char* response = (char*)malloc(buflen);
    char* p = response;
    char* end = response + buflen;

    int code = 0;
    int bytes_received;

    do {
        bytes_received  = recv(server, p, buflen-1, 0);
        if (bytes_received < 1) {
            fprintf(stderr, "Connection dropped.\n");
            exit(1);
        }

        p += bytes_received;
        *p = 0;

    } while (bytes_received < 1);

    code = parseResponse(response, isNeed);
    if (code != expecting) {
        fprintf(stderr, "Error from server:\n");
        fprintf(stderr, "%s", response);
        exit(1);
    }

    
    if (out) {
        strcpy_s(out, outSize, response);
    }

    free(response);
    return bytes_received;
}

void addAttachment(const char* filename, SOCKET clientSocket) {
    char SendBuf[BUFFER_SIZE] = { '\0' };

    char szFileName[512] = { '\0' };
    const char* attach = filename;
    strcat_s(szFileName, "=?UTF-8?B?");
    char* p1 = base64_encode((unsigned char*)attach, strlen(attach));
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

    const char* username = "xxx@xx.com";
    const char* password = "xxxx@";
    char* p1 = base64_encode((unsigned char*)username, strlen(username));
    char* p2 = base64_encode((unsigned char*)password, strlen(password));

    sendFormat(clientSocket, "%s\r\n", p1);
    waitOnResponse(clientSocket, 334);
    sendFormat(clientSocket, "%s\r\n", p2);
    waitOnResponse(clientSocket, 235);

    // send info
    const char* sender = username;
    sendFormat(clientSocket, "MAIL FROM:<%s>\r\n", sender);
    waitOnResponse(clientSocket, 250);

    // to,cc,bcc all in here
    const char* recipient = "yyy@xx.com";
    // to
    sendFormat(clientSocket, "RCPT TO:<%s>\r\n", recipient);
    waitOnResponse(clientSocket, 250);
    /*
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
    */

    sendFormat(clientSocket, "DATA\r\n");
    waitOnResponse(clientSocket, 354);

    // header
    const char* subject = "Test title";
    sendFormat(clientSocket, "From:%s\r\n", "Tony Chen<1@1.com>");
    sendFormat(clientSocket, "To:%s\r\n", "tony1<1@1.com>,tony2<1@1.com>");
    //sendFormat(clientSocket, "Cc:%s\r\n", "tony3<xx@xx.com>,tony4<xx@xx.com>");
    //sendFormat(clientSocket, "Bcc:%s\r\n", "tony5<xx@xx.com>,tony6<xx@xx.com>");
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
    strcat_s(SendBuf, "Content-Transfer-Encoding: 8bit\r\n");
    strcat_s(SendBuf, "\r\n");
    sendFormat(clientSocket, "%s", SendBuf);
    sendFormat(clientSocket, "%s\r\n", body);

    // 1.jpg, 08mailSend.cpp
    if (isAttachment) {
        addAttachment("1.jpg", clientSocket);
        addAttachment("08mailSend.cpp", clientSocket);

        snprintf(SendBuf, BUFFER_SIZE, "\r\n--%s--\r\n", BOUNDARY_TEXT);
        sendFormat(clientSocket, "%s", SendBuf);
    }

    // !!! IMPORTANT, DO NOT FORGET send "."
    sendFormat(clientSocket, "%s","\r\n.\r\n");
    waitOnResponse(clientSocket, 250);
    sendFormat(clientSocket, "QUIT\r\n");
    waitOnResponse(clientSocket, 221);
}

void rewindBuffer(char* buffer, char* p1, char** p2) {
    p1 = buffer;
    p1 += strlen(p1);
    *p1 = 0xd;
    p1--;
    strtok_s(p1, "\r\n", p2);
}

void parseHeader(SOCKET clientSocket, string& buffer, int idx) {
    char* p1;
    char* p2;

    p1 = (char*)buffer.c_str();
    p1 += 4;
    strtok_s(p1, " ", &p2);
    p1 += p2 - p1;
    strtok_s(p1, "\r\n", &p2);
    int size = strtol(p1, 0, 10);
    printf("Mail size is: %d\n", size);

    sprintf_s(buffer.data(), buffer.capacity(), "retr %d\r\n", idx + 1);
    sendFormat(clientSocket, buffer.c_str());

    waitOnResponse(clientSocket, 200, buffer.data(), MAXRESPONSE);
    buffer.append(buffer.data());
    while (1) {
        char* tmp = strstr(buffer.data(), "\r\n.\r\n");
        if (!tmp) {
            char tmpBuf[MAXRESPONSE];
            waitOnResponse(clientSocket, -1, tmpBuf, MAXRESPONSE, 0);
            buffer.append(tmpBuf);
        }
        else {
            break;
        }
    }
}

int parseMetaInfo(char* buffer, std::stack<const char*>& stkBoundary2) {
    char* p1;
    char* p2;
    char* p3;
    char* contentType;
    char* boundaryPart = 0;
    std::stack<const char*> stkBoundary1;

    p1 = buffer;
    strtok_s(p1, "\r\n", &p2);
    p1 = strtok_s(NULL, "\r\n", &p2);
    p1 = strstr(p1, "Received: from");
    if (!p1) {
        exit(-1);
    }

    while (1) {
        p1 = strtok_s(NULL, "\r\n", &p2);
        if (!p1)
            break;

        if (p3 = strstr(p1, "From")) {
            printf("%s\n", p3);
        }
        else if (p3 = strstr(p1, "To")) {
            printf("%s\n", p3);
        }
        else if (p3 = strstr(p1, "Subject")) {
            printf("%s\n", p3);
        }
        else if (p3 = strstr(p1, "Date")) {
            printf("%s\n", p3);
        }
        else if (p3 = strstr(p1, "Content-Type: ")) {
            strtok_s(p1, " ", &contentType);
            contentType[strlen(contentType) - 1] = 0;

            if (strstr(contentType, "multipart")) {
                p1 = strtok_s(NULL, "\r\n", &p2);
                p3 = strstr(p1, "boundary=\"");
                if (p3) {
                    strtok_s(p3, "boundary=\"", &boundaryPart);
                    boundaryPart[strlen(boundaryPart) - 1] = 0;
                    stkBoundary1.push(boundaryPart);
                    stkBoundary2.push(boundaryPart);
                }
            }
        }
        else if (p3 = strstr(p1, "Content-Transfer-Encoding")) {
            //here we really go to the body
            break;
        }

        if (stkBoundary1.size() > 0) {
            if (boundaryPart) {
                if (p3 = strstr(p1, stkBoundary1.top())) {
                    stkBoundary1.pop();
                    continue;
                }
            }
        }
    }

    return p1 - buffer;
}

int parseBody(char* buffer, std::stack<const char*>& stkBoundary2) {
    char* p1 = 0;
    char* p2;
    char* p3 = 0;
    string bodyData;

    rewindBuffer(buffer, p1, &p2);

    while (1) {
        p1 = strtok_s(NULL, "\r\n", &p2);
        if (stkBoundary2.size() > 0) {
            p3 = strstr(p1, stkBoundary2.top());
        }
        if (p3) {
            printf("body data:\n%s\n", bodyData.data());
            stkBoundary2.pop();
            p3 = 0;
            break;
        }
        else {
            bodyData.append(p1);
            bodyData.append("\r\n");
        }
    }

    return p1 - buffer;
}

void parseAttachment(char* buffer, std::stack<const char*>& stkBoundary2) {
    char* p1 = 0;
    char* p2;
    char* p3;
    char* encoding = 0;
    char filename[MAX_PATH] = { 0 };
    string fileData;
    string outData;

    rewindBuffer(buffer, p1, &p2);

    while (1) {
        p1 = strtok_s(NULL, "\r\n", &p2);
        if (!p1)
            break;

        if (p3 = strstr(p1, "Content-Transfer-Encoding")) {
            strtok_s(p1, " ", &encoding);
        }

        if (p3 = strstr(p1, "filename=")) {
            p3 += 10;
            p3[strlen(p3) - 1] = 0;
            strcpy_s(filename, sizeof(filename), p3);
            
            while (1) {
                p1 = strtok_s(NULL, "\r\n", &p2);
                if (!p1)
                    break;
                if (strstr(p1, stkBoundary2.top())) {
                    FILE* f;
                    outData += OUTPUT_PATH;
                    outData += filename;
                    fopen_s(&f, outData.data(), "wb+");

                    outData.clear();
                    outData.reserve(fileData.size() * 2);
                    if (strcmp(encoding, "base64") == 0) {
                        int res = base64_decode((unsigned char*)outData.data(), (unsigned char*)fileData.data());
                        fwrite(outData.data(), 1, res, f);
                    }
                    else {
                        fwrite(fileData.data(), 1, fileData.size(), f);
                    }

                    fclose(f);
                    fileData.clear();
                    fileData.resize(0);
                    break;
                }

                fileData.append(p1);
                if (encoding) {
                    if (strcmp(encoding, "base64") != 0) {
                        fileData.append("\n");
                    }
                }
            }
        }
    }
}

void parseMail(SOCKET clientSocket, string& buffer, int len, int i) {
    char* p1;
    char* p2;
    char* p3 = 0;
    int offset = 0;
    stack<const char*> stkBoundary;

    parseHeader(clientSocket, buffer, i);
    offset += parseMetaInfo(buffer.data(), stkBoundary);    
    offset += parseBody(buffer.data() + offset, stkBoundary);
    parseAttachment(buffer.data() + offset, stkBoundary);
    int a = 0;
}

void recvMail0(const char* hostname, const char* port) {
    SOCKET clientSocket;
    addrinfo* peerAddr;
    int buflen = MAXRESPONSE;
    //char* buffer = (char*)malloc(buflen);
    string buffer;
    buffer.reserve(buflen);
    char* p1 = (char*)buffer.c_str();
    char* p2;
    int count, i;

    setupClientSocket(hostname, port, clientSocket, &peerAddr);
    waitOnResponse(clientSocket, 200);
    
    sendFormat(clientSocket, "user %s\r\n", username);
    waitOnResponse(clientSocket, 200, buffer.data(), buflen);
    sendFormat(clientSocket, "pass %s\r\n", password);
    waitOnResponse(clientSocket, 200, buffer.data(), buflen);
    
    p1 += 4;
    strtok_s(p1, " ", &p2);
    count = strtol(p1, 0, 10);
    printf("Total %d emails.\n", count);

    for (i = 0; i < count; i++) {
        buffer.clear();
        sprintf_s((char*)buffer.data(), buffer.capacity(), "list %d\r\n", i+1);
        sendFormat(clientSocket, buffer.c_str());
        waitOnResponse(clientSocket, 200, buffer.data(), buflen);

        parseMail(clientSocket, buffer, buflen, i);
    }
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

void testRecvMail(const char* hostname, const char* port) {
    INIT_SOCK;

    recvMail0(hostname, port);

    DESTORY_SOCK;
}