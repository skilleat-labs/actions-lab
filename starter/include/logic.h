#ifndef LOGIC_H
#define LOGIC_H
#include <stdint.h>
#include <stddef.h>

/* CRC-8 체크섬 — 데이터 무결성 검사 예제 */
uint8_t crc8(const uint8_t *data, size_t len);
#endif
