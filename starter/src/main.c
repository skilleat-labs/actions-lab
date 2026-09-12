#include "logic.h"
#include <stdio.h>
int main(void)
{
    const uint8_t frame[] = { 0xA5, 0x5A, 0x01 };
    printf("app build ok, crc=0x%02X\n", crc8(frame, sizeof frame));
    return 0;
}
