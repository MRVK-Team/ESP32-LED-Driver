/* CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection, xorout 0.

   Error detection only. This is not authentication and must never be
   treated as security. */
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t crc16_ccitt_false(const char *data, size_t len);

#ifdef __cplusplus
}
#endif
