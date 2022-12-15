
#include "../common/common.h"
#include <locale.h>

#define PRINT_DESC(x, desc) {printf(#x "with " #desc ":\n");}

#define RUN_TEST0(x) printf(#x":\n"); x(); printf("\n")
#define RUN_TEST1(x,desc,a) PRINT_DESC(x, desc); x(a); printf("\n")
#define RUN_TEST2(x,desc,a,b) PRINT_DESC(x, desc); x(a,b); printf("\n")
#define RUN_TEST3(x,desc,a,b,c) PRINT_DESC(x, desc); x(a,b,c); printf("\n")
#define RUN_TEST4(x,desc,a,b,c,d) PRINT_DESC(x, desc); x(a,b,c,d); printf("\n")
#define RUN_TEST5(x,desc,a,b,c,d,e) PRINT_DESC(x, desc); x(a,b,c,d,e); printf("\n")

int main() {
    setlocale(LC_ALL, "zh_CN.UTF-8");

    RUN_TEST0(testInitSocket);
    RUN_TEST0(testInterfaceAddr);
    //RUN_TEST0(testTimeServer);
    //RUN_TEST1(testTimeServer, 0, loop);
    //RUN_TEST0(testSelectClient);
    //RUN_TEST0(testUDPClient);
    //RUN_TEST2(testResolveDNS, "", "www.baidu.com", "a");
    //RUN_TEST1(testHttpClient, "", "www.baidu.com");
    //RUN_TEST1(testSendMail, "", "smtp.163.com");
    //RUN_TEST1(testSendMailWithAttachment, "", "smtp.163.com");
    RUN_TEST1(testRecvMail, "", "pop.163.com");
}