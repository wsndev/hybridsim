#ifndef _CRC_16_XMODEM_H_
#define _CRC_16_XMODEM_H_

#include <stdint.h>
#include <stdlib.h>

uint16_t crc16_xmodem_update(uint16_t, uint8_t);
uint16_t crc16_ccitt_update(uint16_t, uint8_t);

#endif//_CRC_16_XMODEM_H_
