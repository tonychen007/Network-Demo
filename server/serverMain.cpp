#include "../common/common.h"
#include <locale.h>

#define PRINT_DESC(x, desc) {if(desc) printf(#x " with " #desc ":\n"); else printf(#x ":\n");}

#define RUN_TEST0(x,desc) PRINT_DESC(x, desc); x(); printf("\n")
#define RUN_TEST1(x,a,desc) PRINT_DESC(x, desc); x(a); printf("\n")
#define RUN_TEST2(x,a,b,desc) PRINT_DESC(x, desc); x(a,b); printf("\n")
#define RUN_TEST3(x,a,b,c,desc) PRINT_DESC(x, desc); x(a,b,c); printf("\n")
#define RUN_TEST4(x,a,b,c,d,desc) PRINT_DESC(x, desc); x(a,b,c,d); printf("\n")
#define RUN_TEST5(x,a,b,c,d,e,desc) PRINT_DESC(x, desc); x(a,b,c,d,e); printf("\n")


int main() {
    setlocale(LC_ALL, "zh_TW.UTF-8");

    //RUN_TEST0(testTimeServer, 0);
    //RUN_TEST1(testTimeServer, 1, 0);
    //RUN_TEST0(testSelectServer, 0);
    //RUN_TEST0(testUDPServer, 0);
    //RUN_TEST0(testHttpServer, 0);
    RUN_TEST0(testServerIgnore, 0);
}

