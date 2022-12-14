/*
 * MIT License
 *
 * Copyright (c) 2018 Lewis Van Winkle
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "../common/common.h"

const int DNS_HEADER_LEN = 12;

const unsigned char *print_name(const unsigned char *msg,
        const unsigned char *p, const unsigned char *end) {

    if (p + 2 > end) {
        fprintf(stderr, "End of message.\n"); exit(1);}

    if ((*p & 0xC0) == 0xC0) {
        const int k = ((*p & 0x3F) << 8) + p[1];
        p += 2;
        printf(" (pointer %d) ", k);
        print_name(msg, msg+k, end);
        return p;

    } else {
        const int len = *p++;
        if (p + len + 1 > end) {
            fprintf(stderr, "End of message.\n"); exit(1);}

        printf("%.*s", len, p);
        p += len;
        if (*p) {
            printf(".");
            return print_name(msg, p, end);
        } else {
            return p+1;
        }
    }
}

void print_dns_message(const char *message, int msg_length) {

    if (msg_length < DNS_HEADER_LEN) {
        fprintf(stderr, "Message is too short to be valid.\n");
        exit(1);
    }

    const unsigned char *msg = (const unsigned char *)message;

    printf("ID = %0X %0X\n", msg[0], msg[1]);

    // 1000-0000 >> 7 qr is 1bit
    const int qr = (msg[2] & 0x80) >> 7;
    printf("QR = %d %s\n", qr, qr ? "response" : "query");

    // 0111-1000 >> 3 opcode is 4bit
    const int opcode = (msg[2] & 0x78) >> 3;
    printf("OPCODE = %d ", opcode);
    switch(opcode) {
        case 0: printf("standard\n"); break;
        case 1: printf("reverse\n"); break;
        case 2: printf("status\n"); break;
        default: printf("?\n"); break;
    }

    // 0000-0100 >> 2
    const int aa = (msg[2] & 0x04) >> 2;
    printf("AA = %d %s\n", aa, aa ? "authoritative" : "");

    // 0000-0010 >> 2
    const int tc = (msg[2] & 0x02) >> 1;
    printf("TC = %d %s\n", tc, tc ? "message truncated" : "");

    // 0000-0001 >> 2
    const int rd = (msg[2] & 0x01);
    printf("RD = %d %s\n", rd, rd ? "recursion desired" : "");

    if (qr) {
        const int rcode = msg[3] & 0x0F;
        printf("RCODE = %d ", rcode);
        switch(rcode) {
            case 0: printf("success\n"); break;
            case 1: printf("format error\n"); break;
            case 2: printf("server failure\n"); break;
            case 3: printf("name error\n"); break;
            case 4: printf("not implemented\n"); break;
            case 5: printf("refused\n"); break;
            default: printf("?\n"); break;
        }
        if (rcode != 0) return;
    }

    // 2bit [0] [1], so the cnt = [0] << 8 + [1]
    const int qdcount = (msg[4] << 8) + msg[5];
    const int ancount = (msg[6] << 8) + msg[7];
    const int nscount = (msg[8] << 8) + msg[9];
    const int arcount = (msg[10] << 8) + msg[11];

    printf("QDCOUNT = %d\n", qdcount);
    printf("ANCOUNT = %d\n", ancount);
    printf("NSCOUNT = %d\n", nscount);
    printf("ARCOUNT = %d\n", arcount);


    // format: name, qtype, qclass
    const unsigned char *p = msg + DNS_HEADER_LEN;
    const unsigned char *end = msg + msg_length;

    
    if (qdcount) {
        for (int i = 0; i < qdcount; ++i) {
            if (p >= end) {
                fprintf(stderr, "End of message.\n"); exit(1);}

            /*
            printf("Query %2d\n", i + 1);
            printf("  name: ");
            p = print_name(msg, p, end); printf("\n");
            */

            while (*p != 0) p++;
            p++;

            if (p + 4 > end) {
                fprintf(stderr, "End of message.\n"); exit(1);}
            p += 4;

            /*
            const int type = (p[0] << 8) + p[1];
            printf("  type: %d\n", type);
            p += 2;

            const int qclass = (p[0] << 8) + p[1];
            printf(" class: %d\n", qclass);
            p += 2;
            */
        }
    }
    

    /* answer format
    *   0 - 15
    *   name
    *   type
    *   class
    *   ttl
    *   ttl
    *   rdlength
    *   rdata
    *   rdata
    */

    if (ancount || nscount || arcount) {
        int i;
        for (i = 0; i < ancount + nscount + arcount; ++i) {
            if (p >= end) {
                fprintf(stderr, "End of message.\n"); exit(1);}

            printf("Answer %2d\n", i + 1);
            printf("  name: ");

            p = print_name(msg, p, end); printf("\n");

            // 10 is ttl*2 + rdlength + rdata*2 = 2*2 + 2 + 2*2
            if (p + 10 > end) {
                fprintf(stderr, "End of message.\n"); exit(1);}

            const int type = (p[0] << 8) + p[1];
            printf("  type: %d\n", type);
            p += 2;

            const int qclass = (p[0] << 8) + p[1];
            printf(" class: %d\n", qclass);
            p += 2;

            const unsigned int ttl = (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
            printf("   ttl: %u\n", ttl);
            p += 4;

            const int rdlen = (p[0] << 8) + p[1];
            printf(" rdlen: %d\n", rdlen);
            p += 2;

            if (p + rdlen > end) {
                fprintf(stderr, "End of message.\n"); exit(1);}

            if (rdlen == 4 && type == 1) {
                /* A Record */
                printf("Address ");
                printf("%d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);

            } else if (rdlen == 16 && type == 28) {
                /* AAAA Record */
                printf("Address ");
                int j;
                for (j = 0; j < rdlen; j+=2) {
                    printf("%02x%02x", p[j], p[j+1]);
                    if (j + 2 < rdlen) printf(":");
                }
                printf("\n");

            } else if (type == 15 && rdlen > 3) {
                /* MX Record */
                const int preference = (p[0] << 8) + p[1];
                printf("  pref: %d\n", preference);
                printf("MX: ");
                print_name(msg, p+2, end); printf("\n");

            } else if (type == 16) {
                /* TXT Record */
                printf("TXT: '%.*s'\n", rdlen-1, p+1);

            } else if (type == 5) {
                /* CNAME Record */
                printf("CNAME: ");
                print_name(msg, p, end); printf("\n");
            }

            p += rdlen;
        }
    }

    if (p != end) {
        printf("There is some unread data left over.\n");
    }

    printf("\n");
}


void testResolveDNS(const char* hostname, const char* service) {
    INIT_SOCK;

    unsigned char type = 0;
    if (strcmp(service, "a") == 0) {
        type = 1;
    } else if (strcmp(service, "mx") == 0) {
        type = 15;
    } else if (strcmp(service, "txt") == 0) {
        type = 16;
    } else if (strcmp(service, "aaaa") == 0) {
        type = 28;
    } else if (strcmp(service, "any") == 0) {
        type = 255;
    } else {
        fprintf(stderr, "Unknown type '%s'. Use a, aaaa, txt, mx, or any.",
            type);
        exit(1);
    }

    SOCKET socket_peer;
    struct addrinfo* peer_address;
    setupClientSocket("8.8.8.8", "53", socket_peer, &peer_address, 1);
    char query[256] = {0xAB, 0xCD, /* ID */
                        0x01, 0x00, /* Set recursion */
                        0x00, 0x01, /* QDCOUNT */
                        0x00, 0x00, /* ANCOUNT */
                        0x00, 0x00, /* NSCOUNT */
                        0x00, 0x00 /* ARCOUNT */};


    char *p = query + DNS_HEADER_LEN;
    char *h = (char*)hostname;

    // convert www.baidu.com to 3wwww5baidu3com
    while(*h) {
        char *len = p;
        p++;
        if (h != hostname) 
            ++h;

        while(*h && *h != '.') 
            *p++ = *h++;

        *len = p - len - 1;
    }

    *p++ = 0; // '\0'
    *p++ = 0x00; *p++ = type; /* QTYPE */
    *p++ = 0x00; *p++ = 0x01; /* QCLASS */

    // does not include header
    const int query_size = p - query;

    int bytes_sent = sendto(socket_peer,
            query, query_size,
            0,
            peer_address->ai_addr, peer_address->ai_addrlen);
    printf("Sent %d bytes.\n", bytes_sent);

    print_dns_message(query, query_size);

    char read[256];
    int bytes_received = recvfrom(socket_peer,
            read, sizeof(read), 0, 0, 0);

    printf("Received %d bytes.\n", bytes_received);

    print_dns_message(read, bytes_received);
    printf("\n");


    freeaddrinfo(peer_address);
    CLOSESOCKET(socket_peer);

    DESTORY_SOCK;
}

