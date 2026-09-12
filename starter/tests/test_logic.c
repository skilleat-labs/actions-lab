#include "logic.h"
#include <stdio.h>
static int fails = 0;
#define CHECK(c,m) do{ if(c){printf("  ok   : %s\n",m);} else {printf("  FAIL : %s\n",m); fails++;} }while(0)
int main(void)
{
    printf("== 유닛테스트 ==\n");
    const uint8_t f[] = {0x12,0x34,0x56,0x78};
    uint8_t c = crc8(f, sizeof f);
    CHECK(crc8(f,sizeof f)==c, "같은 입력이면 같은 값");
    const uint8_t g[] = {0x12,0x34,0x56,0x79};
    CHECK(crc8(g,sizeof g)!=c, "1비트 변조를 검출");
    CHECK(crc8((const uint8_t*)"",0)==0x00, "길이 0이면 0x00");
    printf("== 실패 %d건 ==\n", fails);
    return fails ? 1 : 0;
}
