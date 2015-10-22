#ifndef _RS232_H_
#define _RS232_H_

#include <stdio.h>
#include <string.h>

#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/file.h>

int rs232_open(const char*, int, const char *);
int rs232_read(int, unsigned char *, int);
int rs232_write_byte(int, unsigned char);
int rs232_write_buffer(int, unsigned char *, int);
void rs232_write(int, const char *);
void rs232_close(int);
int rs232_dcd_enabled(int);
int rs232_cts_enabled(int);
int rs232_dsr_enabled(int);
void rs232_enable_dtr(int);
void rs232_disable_dtr(int);
void rs232_enable_rts(int);
void rs232_disable_rts(int);

#endif// _RS232_H_


