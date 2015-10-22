#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "crc16.h"
#include "rs232.h"


#define PHY_DATA_BYTES 127

#define FRAME_TYPE_SHIFT 5
#define FRAME_TYPE_MASK 0b11100000
#define SEC_ENABLED_SHIFT 4
#define SEC_ENABLED_MASK 0b00010000
#define FRAME_PENDING_SHIFT 3
#define FRAME_PENDING_MASK 0b00001000
#define ACK_REQUEST_SHIFT 2
#define ACK_REQUEST_MASK 0b00000100
#define INTRA_PAN_SHIFT 1
#define INTRA_PAN_MASK 0b00000010
#define DST_ADDR_MODE_SHIFT 4
#define DST_ADDR_MODE_MASK 0b00110000
#define FRAME_VERSION_SHIFT 2
#define FRAME_VERSION_MASK 0b00001100
#define SRC_ADDR_MODE_SHIFT 0
#define SRC_ADDR_MODE_MASK 0b00000011

#define ADDR_16 2
#define ADDR_64 3
#define IEEE_802_15_4_2003 0
#define IEEE_802_15_4_2006 1
#define FRAME_BEACON 0
#define FRAME_DATA 1
#define FRAME_ACK 2
#define FRAME_MAC_CMD 3

int main(int argc, char** argv){
	if (argc == 4 && (argv[1][0] == 's' || argv[1][0] == 'r')){
		if (argv[1][0] == 's'){
			send(argv[2], argv[3]);
		}
		else{
			receive(argv[2], argv[3]);
		}
	}
	else {
		printf("argument error:\n send: s /dev/ttyUSB0 send.txt\n receive: r /dev/ttyUSB1 receive.txt\n");
	}
	exit(EXIT_SUCCESS);
}

send(char* port_name, char* file){
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen(file, "r");
	if (fp == NULL){
		exit(EXIT_FAILURE);	
	}

	uint8_t phy_data[PHY_DATA_BYTES];
	memset(phy_data, 0, PHY_DATA_BYTES);
	uint8_t frame_length;

	uint8_t frame_type = 0;
	bool sec_enabled = false;
	bool frame_pending = false;
	bool ack_request = false;
	bool intra_pan = false;
	uint8_t dst_addr_mode = 0;
	uint8_t frame_version = 0;
	uint8_t src_addr_mode = 0;
	uint8_t sequence_num = 0;
	uint16_t dst_pan_id = 0;
	uint16_t dst_addr = 0;
	uint16_t src_pan_id = 0;
	uint16_t src_addr = 0;
	uint8_t* mac_payload = NULL;
	uint16_t crc16 = 0;

        size_t i;
	uint8_t line_num = 0;
	size_t output = 0;
	char temp[2];
	while ((read = getline(&line, &len, fp)) != -1) {
		for (i = 0; i < len; ++i){
			if (line[i] == ':'){
				line[i] = '\0';
			}
		}
		switch (line_num){
			case 0:// frame type
				frame_type = atoi(line);
				phy_data[0] |= frame_type << FRAME_TYPE_SHIFT;
				printf("%d: frame type\n", frame_type);
				break;
            case 1:// sec enabled
				sec_enabled = atoi(line);
				phy_data[0] |= sec_enabled << SEC_ENABLED_SHIFT;
				printf("%d: sec enabled\n", sec_enabled);
                break;
            case 2:// frame pending
				frame_pending = atoi(line);
				phy_data[0] |= frame_pending << FRAME_PENDING_SHIFT;
                printf("%d: frame pending\n", frame_pending);
                break;
            case 3:// ACK request
				ack_request = atoi(line);
				phy_data[0] |= ack_request << ACK_REQUEST_SHIFT;
                printf("%d: ACK request\n", ack_request);
                break;
            case 4:// intra PAN
				intra_pan = atoi(line);
				phy_data[0] |= intra_pan << INTRA_PAN_SHIFT;
                printf("%d: intra PAN\n", intra_pan);
                break;
            case 5:// dst addr mode
				dst_addr_mode = atoi(line);
				phy_data[1] |= dst_addr_mode << DST_ADDR_MODE_SHIFT;
		        printf("%d: dst addr mode\n", dst_addr_mode);
		        break;
            case 6:// frame version
				frame_version = atoi(line);
				phy_data[1] |= frame_version << FRAME_VERSION_SHIFT;
                printf("%d: frame version\n", frame_version);
                break;
			case 7:// src addr mode
				src_addr_mode = atoi(line);
				phy_data[1] |= src_addr_mode << SRC_ADDR_MODE_SHIFT;
                printf("%d: src addr mode\n", src_addr_mode);
				break;
			case 8:// sequence num
				sequence_num = atoi(line);
				phy_data[2] = sequence_num;
				printf("%d: sequence num\n", sequence_num);
				break;
            case 9:// dst PAN id
				dst_pan_id = strtoul(line, NULL, 16);
				phy_data[3] = (uint8_t) (dst_pan_id >> 8);
		        phy_data[4] = (uint8_t) dst_pan_id;
		        printf("%04x: dst PAN id\n", dst_pan_id);
		        break;
            case 10:// dst addr
				dst_addr = strtoull(line, NULL, 16);
				phy_data[5] = (uint8_t) (dst_addr >> 8);
				phy_data[6] = (uint8_t) dst_addr;
				printf("%04x: dst addr\n", dst_addr);
                break;
            case 11:// src PAN id
				src_pan_id = strtoul(line, NULL, 16);
				phy_data[7] = (uint8_t) (src_pan_id >> 8);
				phy_data[8] = (uint8_t) src_pan_id;
				printf("%04x: src PAN id\n", src_pan_id);
                break;
            case 12:// src addr
				src_addr = strtoull(line, NULL, 16);
				phy_data[9] = src_addr >> 8;
				phy_data[10] = src_addr;
                printf("%04x: src add\n", src_addr);
                break;
            case 13:// mac data unit
				len = strlen(line);
				for (i = 0; i < len; i += 3){
					temp[0] = line[i + 0];
					temp[1] = line[i + 1];
					phy_data[11 + i / 3] = strtoul(temp, NULL, 16);
				}

				frame_length = len / 3 + 11;
				// for (i = 11; i < frame_length; ++i){
				// 	printf("%c", phy_data[i]);
				// }
				// printf("\n");
				for (i = 11; i < frame_length; ++i){
					printf("%02x ", phy_data[i]);
				}
				printf("\n");

				for (i = 0; i < frame_length; ++i){// calculate crc-16-xmodem checksum
					crc16 = crc16_ccitt_update(crc16, phy_data[i]);
				}
				crc16 = (crc16 >> 8) | (crc16 << 8);
				printf("%04x: crc16-ccitt\n", crc16);
				phy_data[frame_length++] = crc16 >> 8;
				phy_data[frame_length++] = crc16;
				printf("%d: frame length\n", frame_length);
                break;
		}
		++line_num;
	}

	fclose(fp);
	if (line){
		free(line);
	}

	/*int port = rs232_open(port_name, 9600, "8N1");
	if (port < 0){
		perror("can't open comport");
		exit(0);
	}
	rs232_write_byte(port, frame_length);
	rs232_write_buffer(port, phy_data, frame_length);
	rs232_close(port);*/
}

receive(char* port_name, char* file){
	uint8_t phy_data[PHY_DATA_BYTES];
	memset(phy_data, 0, PHY_DATA_BYTES);
	uint8_t frame_length;

	int port = rs232_open(port_name, 9600, "8N1");
	if (port < 0){
		perror("can't open comport");
		exit(0);
	}
	while (!rs232_read(port, &frame_length, 1));
	int length = 0;
	int size;
	while (length < frame_length){
		size = rs232_read(port, phy_data + length, frame_length - length);
		length += size;
	}
	rs232_close(port);

	uint8_t frame_type = 0;
	bool sec_enabled = false;
	bool frame_pending = false;
	bool ack_request = false;
	bool intra_pan = false;
	uint8_t dst_addr_mode = 0;
	uint8_t frame_version = 0;
	uint8_t src_addr_mode = 0;
	uint8_t sequence_num = 0;
	uint16_t dst_pan_id = 0;
	uint16_t dst_addr = 0;
	uint16_t src_pan_id = 0;
	uint16_t src_addr = 0;
	uint8_t* mac_payload = NULL;
	uint16_t crc16 = 0;

	frame_type = (phy_data[0] & FRAME_TYPE_MASK) >> FRAME_TYPE_SHIFT;
	sec_enabled = (phy_data[0] & SEC_ENABLED_MASK) >> SEC_ENABLED_SHIFT;
	frame_pending = (phy_data[0] & FRAME_PENDING_MASK) >> FRAME_PENDING_SHIFT;
	ack_request = (phy_data[0] & ACK_REQUEST_MASK) >> ACK_REQUEST_SHIFT;
	intra_pan = (phy_data[0] & INTRA_PAN_MASK) >> INTRA_PAN_SHIFT;
	dst_addr_mode = (phy_data[1] & DST_ADDR_MODE_MASK) >> DST_ADDR_MODE_SHIFT;
	frame_version = (phy_data[1] & FRAME_VERSION_MASK) >> FRAME_VERSION_SHIFT;
	src_addr_mode = (phy_data[1] & SRC_ADDR_MODE_MASK) >> SRC_ADDR_MODE_SHIFT;
	sequence_num = phy_data[2];
	dst_pan_id = phy_data[3];
	dst_pan_id = (dst_pan_id << 8) | phy_data[4];
	dst_addr = phy_data[5];
	dst_addr = (dst_addr << 8) | phy_data[6];
	src_pan_id = phy_data[7];
	src_pan_id = (src_pan_id << 8) | phy_data[8];
	src_addr = phy_data[9];
	src_addr = (src_addr << 8) | phy_data[10];
	mac_payload = phy_data + 11;
	crc16 = phy_data[frame_length - 2] << 8;
	crc16 |= phy_data[frame_length - 1];

	FILE * fp;
	fp = fopen(file, "w");
	if (fp == NULL){
		exit(EXIT_FAILURE);
	}

	fprintf(fp, "%d: frame type\n", frame_type);
	fprintf(fp, "%d: sec enabled\n", sec_enabled);
    fprintf(fp, "%d: frame pending\n", frame_pending);
    fprintf(fp, "%d: ACK request\n", ack_request);
    fprintf(fp, "%d: intra PAN\n", intra_pan);
    fprintf(fp, "%d: dst addr mode\n", dst_addr_mode);
    fprintf(fp, "%d: frame version\n", frame_version);
    fprintf(fp, "%d: src addr mode\n", src_addr_mode);
	fprintf(fp, "%d: sequence num\n", sequence_num);
    fprintf(fp, "%04x: dst PAN id\n", dst_pan_id);
	fprintf(fp, "%04x: dst addr\n", dst_addr);
	fprintf(fp, "%04x: src PAN id\n", src_pan_id);
    fprintf(fp, "%04x: src add\n", src_addr);
	uint8_t i;
	for (i = 11; i < frame_length - 2; ++i){
		fprintf(fp, "%02x ", phy_data[i]);
	}
	fprintf(fp, "\n");
	fprintf(fp, "%04x: crc16-ccitt\n", crc16);
	fprintf(fp, "%d: frame length\n", frame_length);

	for (i = 11; i < frame_length - 2; ++i){
		if (phy_data[i] >= 0x20 && phy_data[i] < 0x7f){
			fprintf(fp, "%c", phy_data[i]);
		}
		else{
			fprintf(fp, ".");
		}
	}
	fprintf(fp, "\n");

	fclose(fp);
}