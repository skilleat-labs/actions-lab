#ifndef LOGIC_H
#define LOGIC_H
#include <stdint.h>
#include <stddef.h>

/* SAE J1850 CRC-8 — 차량 네트워크 프레임 무결성 검사 방식 */
uint8_t crc8_j1850(const uint8_t *data, size_t len);
#endif
