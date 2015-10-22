#include "rs232.h"

int rs232_open(const char* port_name, int baudrate, const char *mode){
	int baudr;

	switch(baudrate) {
	case      50 :
		baudr = B50;
		break;
	case      75 :
		baudr = B75;
		break;
	case     110 :
		baudr = B110;
		break;
	case     134 :
		baudr = B134;
		break;
	case     150 :
		baudr = B150;
		break;
	case     200 :
		baudr = B200;
		break;
	case     300 :
		baudr = B300;
		break;
	case     600 :
		baudr = B600;
		break;
	case    1200 :
		baudr = B1200;
		break;
	case    1800 :
		baudr = B1800;
		break;
	case    2400 :
		baudr = B2400;
		break;
	case    4800 :
		baudr = B4800;
		break;
	case    9600 :
		baudr = B9600;
		break;
	case   19200 :
		baudr = B19200;
		break;
	case   38400 :
		baudr = B38400;
		break;
	case   57600 :
		baudr = B57600;
		break;
	case  115200 :
		baudr = B115200;
		break;
	case  230400 :
		baudr = B230400;
		break;
	case  460800 :
		baudr = B460800;
		break;
	case  500000 :
		baudr = B500000;
		break;
	case  576000 :
		baudr = B576000;
		break;
	case  921600 :
		baudr = B921600;
		break;
	case 1000000 :
		baudr = B1000000;
		break;
	case 1152000 :
		baudr = B1152000;
		break;
	case 1500000 :
		baudr = B1500000;
		break;
	case 2000000 :
		baudr = B2000000;
		break;
	case 2500000 :
		baudr = B2500000;
		break;
	case 3000000 :
		baudr = B3000000;
		break;
	case 3500000 :
		baudr = B3500000;
		break;
	case 4000000 :
		baudr = B4000000;
		break;
	default      :
		printf("invalid baudrate\n");
		return -1;
		break;
	}

	int cbits=CS8,
		cpar=0,
		ipar=IGNPAR,
		bstop=0;

	if(strlen(mode) != 3) {
		printf("invalid mode \"%s\"\n", mode);
		return -1;
	}

	switch(mode[0]) {
	case '8':
		cbits = CS8;
		break;
	case '7':
		cbits = CS7;
		break;
	case '6':
		cbits = CS6;
		break;
	case '5':
		cbits = CS5;
		break;
	default :
		printf("invalid number of data-bits '%c'\n", mode[0]);
		return -1;
		break;
	}

	switch(mode[1]) {
	case 'N':
	case 'n':
		cpar = 0;
		ipar = IGNPAR;
		break;
	case 'E':
	case 'e':
		cpar = PARENB;
		ipar = INPCK;
		break;
	case 'O':
	case 'o':
		cpar = (PARENB | PARODD);
		ipar = INPCK;
		break;
	default :
		printf("invalid parity '%c'\n", mode[1]);
		return -1;
		break;
	}

	switch(mode[2]) {
	case '1':
		bstop = 0;
		break;
	case '2':
		bstop = CSTOPB;
		break;
	default :
		printf("invalid number of stop bits '%c'\n", mode[2]);
		return -1;
		break;
	}

	int port = open(port_name, O_RDWR | O_NOCTTY | O_NDELAY);
	if(port==-1) {
		perror("unable to open comport ");
		return -1;
	}

	/* lock access so that another process can't also use the port */
	if(flock(port, LOCK_EX | LOCK_NB) != 0) {
		close(port);
		perror("Another process has locked the comport.");
		return -1;
	}
	
	struct termios new_port_settings;
	memset(&new_port_settings, 0, sizeof(new_port_settings));  /* clear the new struct */
	new_port_settings.c_cflag = cbits | cpar | bstop | CLOCAL | CREAD;
	new_port_settings.c_iflag = ipar;
	new_port_settings.c_oflag = 0;
	new_port_settings.c_lflag = 0;
	new_port_settings.c_cc[VMIN] = 0;      /* block untill n bytes are received */
	new_port_settings.c_cc[VTIME] = 0;     /* block untill a timer expires (n * 100 mSec.) */

	cfsetispeed(&new_port_settings, baudr);
	cfsetospeed(&new_port_settings, baudr);

	if(tcsetattr(port, TCSANOW, &new_port_settings) == -1) {
		close(port);
		perror("unable to adjust portsettings ");
		return -1;
	}

	int status;
	if(ioctl(port, TIOCMGET, &status) == -1) {
		close(port);
		perror("unable to get portstatus");
		return -1;
	}

	status |= TIOCM_DTR;    /* turn on DTR */
	status |= TIOCM_RTS;    /* turn on RTS */

	if(ioctl(port, TIOCMSET, &status) == -1) {
		close(port);
		perror("unable to set portstatus");
		return -1;
	}

	return port;
}

int rs232_read(int port, unsigned char *buf, int size){
	return read(port, buf, size);
}

int rs232_write_byte(int port, unsigned char byte){
	return write(port, &byte, 1);
}

int rs232_write_buffer(int port, unsigned char *buf, int size){
	return write(port, buf, size);
}

void rs232_write(int port, const char *text) { /* sends a string to serial port */
	while(*text){   
		rs232_write_byte(port, *(text++));
	}
}

void rs232_close(int port){
	int status;
	if(ioctl(port, TIOCMGET, &status) == -1) {
		perror("unable to get portstatus");
	}

	status &= ~TIOCM_DTR;    /* turn off DTR */
	status &= ~TIOCM_RTS;    /* turn off RTS */

	if(ioctl(port, TIOCMSET, &status) == -1) {
		perror("unable to set portstatus");
	}

	close(port);
	flock(port, LOCK_UN); /* free the port so that others can use it. */
}

int rs232_dcd_enabled(int port){
	int status;
	ioctl(port, TIOCMGET, &status);
	return (status & TIOCM_CAR);
}

int rs232_cts_enabled(int port){
	int status;
	ioctl(port, TIOCMGET, &status);
	return (status & TIOCM_CTS);
}

int rs232_dsr_enabled(int port){
	int status;
	ioctl(port, TIOCMGET, &status);
	return (status & TIOCM_DSR);
}

void rs232_enable_dtr(int port){
	int status;
	if(ioctl(port, TIOCMGET, &status) == -1) {
		perror("unable to get portstatus");
	}
	status |= TIOCM_DTR;    /* turn on DTR */
	if(ioctl(port, TIOCMSET, &status) == -1) {
		perror("unable to set portstatus");
	}
}

void rs232_disable_dtr(int port){
	int status;
	if(ioctl(port, TIOCMGET, &status) == -1) {
		perror("unable to get portstatus");
	}
	status &= ~TIOCM_DTR;    /* turn off DTR */
	if(ioctl(port, TIOCMSET, &status) == -1) {
		perror("unable to set portstatus");
	}
}

void rs232_enable_rts(int port){
	int status;
	if(ioctl(port, TIOCMGET, &status) == -1) {
		perror("unable to get portstatus");
	}
	status |= TIOCM_RTS;    /* turn on RTS */
	if(ioctl(port, TIOCMSET, &status) == -1) {
		perror("unable to set portstatus");
	}
}

void rs232_disable_rts(int port){
	int status;
	if(ioctl(port, TIOCMGET, &status) == -1) {
		perror("unable to get portstatus");
	}
	status &= ~TIOCM_RTS;    /* turn off RTS */
	if(ioctl(port, TIOCMSET, &status) == -1) {
		perror("unable to set portstatus");
	}
}