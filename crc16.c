#include "crc16.h"

uint16_t crc16_xmodem_update(uint16_t crc, uint8_t data){
        size_t i;
        crc = crc ^ ((uint16_t)data << 8);
        for (i = 0;  i < 8; i++){
                if (crc & 0x8000){
                        crc = (crc << 1) ^ 0x1021;
                }
                else{
                        crc <<= 1;
                }
        }
        return crc;
}

uint16_t crc16_ccitt_update(uint16_t crc, uint8_t data) {
	data ^= (crc & 0xff);
	data ^= data << 4;
	return ((((uint16_t) data << 8) | (crc >> 8)) ^ (uint8_t)(data >> 4) ^ ((uint16_t) data << 3));
}
