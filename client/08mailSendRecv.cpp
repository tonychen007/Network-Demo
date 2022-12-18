
#include "../common/common.h"
#include "../common/base64.h"

#include <ctype.h>
#include <stdarg.h>
#include <stack>
#include <string>
using namespace std;

#define MAXINPUT 512
#define BOUNDARY_TEXT "__MESSAGE__ID__1"
#define DELM "\r\n"

const int MAXRESPONSE = 4096;
const int BUFFER_SIZE = 4096;
const char* OUTPUT_PATH = "z:\\";
#pragma message("[!!!]change the output_path in " __FILE__ "const char* OUTPUT_PATH");

const char* username = "";
const char* password = "";

void sendFormat(SOCKET server, const char* text, ...) {
    char buffer[BUFFER_SIZE];
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

int waitOnResponse(SOCKET server, int expecting, string* out = NULL, int isNeed = 1) {
    int buflen = MAXRESPONSE;
    string response;

    response.reserve(buflen);
    char* p = (char*)response.c_str();
    char* end = p + buflen;

    int code = 0;
    int bytes_received;

    do {
        bytes_received  = recv(server, response.data(), buflen-1, 0);
        if (bytes_received < 1) {
            fprintf(stderr, "Connection dropped.\n");
            exit(1);
        }


        p += bytes_received;
        *p = 0;

    } while (bytes_received < 1);

    code = parseResponse(response.c_str(), isNeed);
    if (code != expecting) {
        fprintf(stderr, "Error from server:\n");
        fprintf(stderr, "%s", response);
        exit(1);
    }

    
    if (out) {
        out->append(response.c_str());
    }
    
    return bytes_received;
}

void addAttachment(const char* filename, SOCKET clientSocket) {
    string file;
    string sendBuf;

    sendBuf.reserve(BUFFER_SIZE);
    file.reserve(BUFFER_SIZE);
    char* p = base64_encode((unsigned char*)filename, strlen(filename));
    file.append("=?UTF-8?B?");
    file.append(p);
    file.append("?=");

    sendBuf.append("--" BOUNDARY_TEXT DELM);
    sendBuf.append("Content-Type: application/x-msdownload; name=\"");
    sendBuf.append(file);
    sendBuf.append("\"" DELM);
    sendBuf.append("Content-Transfer-Encoding: base64" DELM);
    sendBuf.append("Content-Disposition: attachment; filename=\"");
    sendBuf.append(file);
    sendBuf.append("\"" DELM);
    sendBuf.append(DELM);
    sendFormat(clientSocket, "%s", sendBuf.c_str());
    sendBuf.clear();

    const int bb =  1024;
    FILE* f;
    unsigned char fileBuf[bb];
    fopen_s(&f, filename, "rb");
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
            sendBuf.clear();
        }
        sendBuf.append(pb64);
        sendBuf.append(DELM);

        part += res + 2;
        if (part >= BUFFER_SIZE / 2) {
            part = 0;
            sendFormat(clientSocket, "%s", sendBuf.c_str());
        }
    }
    if (part) {
        sendFormat(clientSocket, "%s", sendBuf.c_str());
    }
    fclose(f);
    f = NULL;
}

void sendMail0(const char* hostname, const char* port, int isAttachment = 0, int isHTML = 0) {
    SOCKET clientSocket;
    addrinfo* peerAddr;
    string sendBuf;
    sendBuf.reserve(BUFFER_SIZE);
    setupClientSocket(hostname, port, clientSocket, &peerAddr);
    
    waitOnResponse(clientSocket, 220);
    sendFormat(clientSocket, "HELO HONPWC" DELM);
    waitOnResponse(clientSocket, 250);

    // auth
    sendFormat(clientSocket, "AUTH LOGIN" DELM);
    waitOnResponse(clientSocket, 334);

    char* p1 = base64_encode((unsigned char*)username, strlen(username));
    char* p2 = base64_encode((unsigned char*)password, strlen(password));

    sendFormat(clientSocket, "%s" DELM, p1);
    waitOnResponse(clientSocket, 334);
    sendFormat(clientSocket, "%s" DELM, p2);
    waitOnResponse(clientSocket, 235);

    // send info
    const char* sender = username;
    sendFormat(clientSocket, "MAIL FROM:<%s>" DELM, sender);
    waitOnResponse(clientSocket, 250);

    // to,cc,bcc all in here
    const char* recipient = username;
    // to
    sendFormat(clientSocket, "RCPT TO:<%s>" DELM, recipient);
    waitOnResponse(clientSocket, 250);
    
    sendFormat(clientSocket, "RCPT TO:<%s>" DELM, recipient);
    waitOnResponse(clientSocket, 250);
    // cc
    sendFormat(clientSocket, "RCPT TO:<%s>" DELM, recipient);
    waitOnResponse(clientSocket, 250);
    sendFormat(clientSocket, "RCPT TO:<%s>" DELM, recipient);
    waitOnResponse(clientSocket, 250);
    // bcc
    sendFormat(clientSocket, "RCPT TO:<%s>" DELM, recipient);
    waitOnResponse(clientSocket, 250);
    sendFormat(clientSocket, "RCPT TO:<%s>" DELM, recipient);
    waitOnResponse(clientSocket, 250);

    sendFormat(clientSocket, "DATA" DELM);
    waitOnResponse(clientSocket, 354);

    // header
    const char* subject = "Test title";
    sendFormat(clientSocket, "From:%s" DELM, "Tony Chen<1@1.com>");
    sendFormat(clientSocket, "To:%s" DELM, "tony1<1@1.com>,tony2<1@1.com>");
    //sendFormat(clientSocket, "Cc:%s\r\n", "tony3<xx@xx.com>,tony4<xx@xx.com>");
    //sendFormat(clientSocket, "Bcc:%s\r\n", "tony5<xx@xx.com>,tony6<xx@xx.com>");
    sendFormat(clientSocket, "Subject:%s" DELM, subject);
    const char* body = "<h1>Hi Tony:</h1>\n\n<div style=\"color:red\">Welcome to our pary.</div>\n<p>Have a good time.</p><p>Yours Sincerely,</p><p>Tom.</p>";
   
    sendBuf.append("MIME-Version: 1.0" DELM);
    if (isAttachment) {
        sendBuf.append("Content-Type: multipart/mixed; boundary=\"" BOUNDARY_TEXT);
        sendBuf.append("\"" DELM DELM "--" BOUNDARY_TEXT DELM);
    }
    if (isHTML)
        sendBuf.append("Content-type: text/html; charset=UTF-8" DELM);
    else
        sendBuf.append("Content-type: text/plain; charset=UTF-8" DELM);
    sendBuf.append("Content-Transfer-Encoding: 8bit" DELM DELM);
    sendFormat(clientSocket, "%s", sendBuf.c_str());
    sendFormat(clientSocket, "%s" DELM, body);
    sendBuf.clear();

    // 1.jpg, 08mailSend.cpp
    if (isAttachment) {
        addAttachment("1.jpg", clientSocket);
        addAttachment(__FILE__, clientSocket);

        sendBuf.append(DELM "--" BOUNDARY_TEXT "--" DELM);
        sendFormat(clientSocket, "%s", sendBuf.c_str());
    }

    // !!! IMPORTANT, DO NOT FORGET send "."
    sendFormat(clientSocket, "%s", DELM "." DELM);
    waitOnResponse(clientSocket, 250);
    sendFormat(clientSocket, "QUIT" DELM);
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
    strtok_s(p1, DELM, &p2);
    int size = strtol(p1, 0, 10);
    printf("Mail size is: %d\n", size);

    sprintf_s(buffer.data(), buffer.capacity(), "retr %d" DELM, idx + 1);
    sendFormat(clientSocket, buffer.c_str());
    buffer.clear();
    
    waitOnResponse(clientSocket, 200, &buffer);
    while (1) {
        const char* pend = DELM "." DELM;
        int lpend = strlen(pend);
        int ret = memcmp(buffer.data() + buffer.size() - lpend, pend, lpend);
        if (ret) {
            string tmpsb;
            waitOnResponse(clientSocket, -1, &tmpsb, 0);
            buffer.append(tmpsb);
        }
        else {
            break;
        }
    }
}

int parseMetaInfo(char* buffer, stack<const char*>& stkBoundary2) {
    char* p1;
    char* p2;
    char* p3;
    char* contentType;
    char* boundaryPart = 0;
    std::stack<const char*> stkBoundary1;

    p1 = buffer;
    strtok_s(p1, DELM, &p2);
    p1 = strtok_s(NULL, DELM, &p2);
    p1 = strstr(p1, "Received: from");
    if (!p1) {
        exit(-1);
    }

    while (1) {
        p1 = strtok_s(NULL, DELM, &p2);
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
                p1 = strtok_s(NULL, DELM, &p2);
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

int parseBody(char* buffer, stack<const char*>& stkBoundary) {
    char* p1 = 0;
    char* p2;
    char* p3 = 0;
    string bodyData;

    rewindBuffer(buffer, p1, &p2);

    while (1) {
        p1 = strtok_s(NULL, DELM, &p2);
        if (stkBoundary.size() > 0) {
            p3 = strstr(p1, stkBoundary.top());
        }
        if (p3) {
            printf("body data:\n%s\n", bodyData.data());
            stkBoundary.pop();
            p3 = 0;
            break;
        }
        else {
            bodyData.append(p1);
            bodyData.append(DELM);
        }
    }

    return p1 - buffer;
}

int writeAttachment(const char* filename, char* buffer, const char* encoding, const char* boundary) {
    char* p1 = 0;
    char* p2;
    string fileData;
    string outData;
    int offset = 0;

    rewindBuffer(buffer, p1, &p2);

    while (1) {
        p1 = strtok_s(NULL, DELM, &p2);
        if (!p1)
            break;
        if (strstr(p1, boundary)) {
            fileData.append("0");
            FILE* f;
            outData += OUTPUT_PATH;
            outData += filename;

            wchar_t* wstr = char2wchar(outData.c_str());
            if (!wstr) {
                break;
            }

            _wfopen_s(&f, wstr, L"wb+");
            if (!f) {
                break;
            }
            free(wstr);

            outData.clear();
            outData.reserve(fileData.size() * 2);
            if (strcmp(encoding, "base64") == 0) {
                int res = base64_decode((unsigned char*)outData.data(), (unsigned char*)fileData.data());
                fwrite(outData.data(), 1, res - 1, f);
            }
            else {
                if (fileData.size() > 1)
                    fwrite(fileData.data(), 1, fileData.size(), f);
            }

            fclose(f);
            fileData.clear();
            fileData.resize(0);
            offset = p1 - buffer;
            break;
        }

        fileData.append(p1);
        if (encoding) {
            if (strcmp(encoding, "base64") != 0) {
                fileData.append("\n");
            }
        }
    }

    return offset;
}

void parseAttachment(char* buffer, std::stack<const char*>& stkBoundary2) {
    char* p1 = 0;
    char* p2;
    char* p3;
    char* encoding = 0;
    char filename[MAX_PATH] = { 0 };
    char decFilename[MAX_PATH] = { 0 };
    string fileData;
    string outData;

    rewindBuffer(buffer, p1, &p2);

    // attachment in the body
    while (1) {
        p1 = strtok_s(NULL, DELM, &p2);
        if (strstr(p1, stkBoundary2.top())) {
            while (1) {
                p1 = strtok_s(NULL, DELM, &p2);

                if (p3 = strstr(p1, "name=")) {
                    p3 += 6;
                    p3[strlen(p3) - 1] = 0;
                    strcpy_s(filename, sizeof(filename), p3);
                    printf("attachment in body: %s\n", filename);

                    p1 = strtok_s(NULL, DELM, &p2);
                    if (p3 = strstr(p1, "Content-Transfer-Encoding")) {
                        strtok_s(p1, " ", &encoding);
                    }
                    p1 = strtok_s(NULL, DELM, &p2);
                    int offset = writeAttachment(filename, p1, encoding, stkBoundary2.top());
                    p1 += offset;
                    rewindBuffer(p1, p1, &p2);
                    if (strstr(p1, stkBoundary2.top())) {
                        stkBoundary2.pop();
                        if (stkBoundary2.empty()) {
                            stkBoundary2.push(p1);
                            goto attachment;
                        }
                    }
                }
            }
        }
    }

 attachment:
    // attachment standalone
    while (1) {
        p1 = strtok_s(NULL, DELM, &p2);
        if (!p1)
            break;

        if (p3 = strstr(p1, "Content-Transfer-Encoding")) {
            strtok_s(p1, " ", &encoding);
        }

        if (p3 = strstr(p1, "filename=")) {
            p3 += 10;
            p3[strlen(p3) - 1] = 0;
            strcpy_s(filename, sizeof(filename), p3);
            
            if (p3 = strstr(filename, "?B?")) {
                string tmp;
                tmp.assign(p3 + 3, strlen(p3) - 7);
                base64_decode((unsigned char*)decFilename, (unsigned char*)tmp.c_str());
                strcpy_s(filename, MAX_PATH, decFilename);
                wchar_t* wstr = char2wchar(filename);
                wprintf(L"attachment filename: %s\n", wstr);
            }
            else {
                printf("attachment filename: %s\n", filename);
            }

            int l = strlen(p1);
            p1 += l;
            *p1 = 0x1;
            
            int offset = writeAttachment(filename, p1, encoding, stkBoundary2.top());
            p1 += offset;
            rewindBuffer(p1, p1, &p2);
            if (strstr(p1, stkBoundary2.top())) {
                int a = 0;
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
}

void recvMail0(const char* hostname, const char* port) {
    SOCKET clientSocket;
    addrinfo* peerAddr;
    
    string buffer;
    buffer.reserve(MAXRESPONSE);
    char* p1 = (char*)buffer.c_str();
    char* p2;
    int count, i;

    setupClientSocket(hostname, port, clientSocket, &peerAddr);
    waitOnResponse(clientSocket, 200);
    
    sendFormat(clientSocket, "user %s" DELM, username);
    waitOnResponse(clientSocket, 200, &buffer);
    sendFormat(clientSocket, "pass %s" DELM, password);
    waitOnResponse(clientSocket, 200, &buffer);
    
    p1 += 4;
    strtok_s(p1, DELM, &p2);
    p1 = strtok_s(NULL, DELM, &p2);
    p1 += 4;
    count = strtol(p1, 0, 10);
    printf("Total %d emails.\n", count);

    for (i = 0; i < count; i++) {
        buffer.clear();
        sprintf_s((char*)buffer.data(), buffer.capacity(), "list %d" DELM, i+1);
        sendFormat(clientSocket, buffer.c_str());
        waitOnResponse(clientSocket, 200, &buffer);

        parseMail(clientSocket, buffer, MAXRESPONSE, i);
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