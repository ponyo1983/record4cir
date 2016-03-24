/*
 * frame_manager.c
 *
 *  Created on: Mar 10, 2014
 *      Author: lifeng
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>


#include "frame_manager.h"

#include "../lib/crc16.h"
#include "../storage/record_manager.h"

#define DLE (0x10)
#define STX	(0x02)
#define ETX	(0x03)

#ifdef __x86_64 //模拟机

#define CONTROL_SERIAL_NAME	("/dev/ttyUSB0")
#define CONTROL_BAUDRATE	(115200)

#define GPS_SERIAL_NAME	("/dev/ttyUSB1")
#define GPS_BAUDRATE	(115200)

#else //实际ARM开发板

#define CONTROL_SERIAL_NAME	("/dev/ttyS2")
#define CONTROL_BAUDRATE	(19200)

#define GPS_SERIAL_NAME	("/dev/ttyS1")
#define GPS_BAUDRATE	(9600)

#endif




static struct frame_manager * gpManager = NULL;


// converts integer baud to Linux define
static int get_baud(int baud) {
	switch (baud) {
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
	case 1000000:
		return B1000000;
	case 1152000:
		return B1152000;
	case 1500000:
		return B1500000;
	case 2000000:
		return B2000000;
	case 2500000:
		return B2500000;
	case 3000000:
		return B3000000;
	case 3500000:
		return B3500000;
	case 4000000:
		return B4000000;
	default:
		return B9600;
	}
}

static int open_serial(char * serial, int baudrate) {
	int fd = -1;
	fd = open(serial, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd >= 0) {
		struct termios newtio;
		bzero(&newtio, sizeof(newtio));

		/* man termios get more info on below settings */
		newtio.c_cflag = get_baud(baudrate) | CS8 | CLOCAL | CREAD;

		newtio.c_iflag = 0;
		newtio.c_oflag = 0;
		newtio.c_lflag = 0;

		// block for up till 128 characters
		newtio.c_cc[VMIN] = 128;

		// 0.5 seconds read timeout
		newtio.c_cc[VTIME] = 5;

		/* now clean the modem line and activate the settings for the port */
		tcflush(fd, TCIFLUSH);
		tcsetattr(fd, TCSANOW, &newtio);
	}

	return fd;

}

static void serial_read(struct frame_manager *manager) {

	int i;
	int fd = manager->serialFd;
	char *buffer = manager->buffer_rx;
	char *frame_buffer = manager->frame_buffer;
	int count = read(fd, buffer, BUFFER_SIZE);
	int *pframe_length = &(manager->frame_length);
	char *pmetDLE = &(manager->metDLE);

	struct record_manager * record_manager=get_record_manager();
	if (count > 0) {

		for (i = 0; i < count; i++) {

			switch (buffer[i]) {

			case DLE: {
				*pmetDLE = 1 - *pmetDLE;
				if (*pmetDLE) {
					frame_buffer[*pframe_length] = DLE;
					*pframe_length = *pframe_length + 1;
				}
			}
				break;
			case STX:
				if (*pmetDLE) {
					frame_buffer[0] = DLE;
					frame_buffer[1] = STX;
					*pframe_length = 2;

				} else if (*pframe_length >= 2) {
					frame_buffer[*pframe_length] = STX;
					*pframe_length = *pframe_length + 1;
				}
				*pmetDLE = 0;
				break;
			case ETX:
				if (*pmetDLE) {

					frame_buffer[*pframe_length] = ETX;
					*pframe_length = *pframe_length + 1;
					if (*pframe_length >= 4) {

						store_serial_data(record_manager,frame_buffer,*pframe_length); //先存储，这样记录的就不会乱续拉
						usleep(2000);

						add_block_data((struct block_manager*)manager,frame_buffer,*pframe_length,0);


					}
					*pframe_length = 0;

				} else if (*pframe_length >= 2) {
					frame_buffer[*pframe_length] = ETX;
					*pframe_length = *pframe_length + 1;
				}
				*pmetDLE = 0;
				break;
			default:
				*pmetDLE = 0;
				if (*pframe_length >= 2) {
					frame_buffer[*pframe_length] = buffer[i];
					*pframe_length = *pframe_length + 1;
				}
				break;
			}
			if (*pframe_length >= BUFFER_SIZE) {
				*pframe_length = 0;
				*pmetDLE = 0;
			}
		}

	}
}

static void * serial_proc(void *args) {

	struct frame_manager *manager = (struct frame_manager*) args;

	struct pollfd serial_poll;
	bzero(&serial_poll, sizeof(serial_poll));
	serial_poll.fd = manager->serialFd;
	serial_poll.events |= POLLIN;
	//serial_poll.events &= ~POLLOUT;
	//serial_poll.events &= ~POLLWRNORM;

	while (1) {
		int retval = poll(&serial_poll, 1, 10000);

		if (retval == -1) {
			perror("poll()");
		} else if (retval) {
			if (serial_poll.revents & POLLIN) {
				serial_read(manager);
			}
		} else {
			//printf("No data within ten seconds.\n");
		}
	}

	return NULL;

}

struct frame_manager* get_frame_manager(enum manager_type type) {
	struct frame_manager* manager = gpManager;

	while (manager != NULL) {
		if (manager->type == type) {
			break;
		} else {
			manager = manager->next;
		}
	}
	return manager;

}


struct block_filter * get_frame_filter(struct frame_manager * manager)
{
	return manager->manager.fliters;
}

static struct frame_manager* create_frame_manager(char * serial, int baudrate,
		enum manager_type type) {

	struct frame_manager * manager = malloc(sizeof(struct frame_manager));
	if (manager == NULL)
		return NULL;

	memset(manager,0,sizeof(struct frame_manager));

	add_block_filter(&(manager->manager),NULL,512,100);


	pthread_mutex_init(&(manager->mutex), NULL);
	manager->type = type;
	manager->next = NULL;
	manager->serialFd = -1;
	manager->metDLE = 0;
	manager->frame_length = 0;
	bzero(manager->serial_name, sizeof(manager->serial_name));

	strcpy(manager->serial_name, serial);

	//open serial
	manager->serialFd = open_serial(serial, baudrate);
	if (manager->serialFd >= 0) {
		pthread_create(&manager->threadRx, NULL, serial_proc, manager);
		manager->next = gpManager;
		gpManager = manager;
	}

	return manager;
}

void init_frame_manager() {
	create_frame_manager(CONTROL_SERIAL_NAME, CONTROL_BAUDRATE,
			CONTROL_MANAGER);
#ifndef __x86_64
	create_frame_manager(GPS_SERIAL_NAME, GPS_BAUDRATE,GPS_MANAGER);
#endif

}
struct frame_manager * get_manager() {

	return NULL;
}

void send_frame(struct frame_manager *manager, char src, char dest, char oper,
		char cmd, char *data, int length) {

	struct record_manager *record_manager=get_record_manager();
	char * buffer = (char*) manager->buffer_tx;
	int len = length + 8;
	int i, index;
	char tmp;
	unsigned short crc = 0;

	if (length < 0 || length >= 1024)
		return;

	usleep(2000);

	pthread_mutex_lock(&(manager->mutex));

	// compute crc first
	buffer[0]=0x10;
	buffer[1]=0x02;
	buffer[2] = (char) (len >> 8);
	buffer[3] = (char) (len & 0xff);
	buffer[4] = src;
	buffer[5] = 0;
	buffer[6] = dest;
	buffer[7] = 0;
	buffer[8] = oper;
	buffer[9] = cmd;
	for (i = 0; i < length; i++) {
		buffer[10 + i] = data[i];
	}

	crc = compute_crc16(buffer + 2, len);
	buffer[10+length]=(char) (crc >> 8);
	buffer[11+length]=(char) (crc & 0xff);
	buffer[12+length]=0x10;
	buffer[13+length]=0x03;

	store_serial_data(record_manager,buffer,14+length);

	// make a frame

	buffer[0] = 0x10;
	buffer[1] = 0x02;

	index = 2;

	tmp = (char) (len >> 8);
	buffer[index++] = tmp;
	if (tmp == 0x10) {
		buffer[index++] = 0x10;
	}

	tmp = (char) (len & 0xff);
	buffer[index++] = tmp;
	if (tmp == 0x10) {
		buffer[index++] = 0x10;
	}

	buffer[index++] = src;
	if (src == 0x10) {
		buffer[index++] = 0x10;
	}
	buffer[index++] = 0;

	buffer[index++] = dest;
	if (dest == 0x10) {
		buffer[index++] = 0x10;
	}
	buffer[index++] = 0;

	buffer[index++] = oper;
	if (oper == 0x10) {
		buffer[index++] = 0x10;
	}

	buffer[index++] = cmd;
	if (cmd == 0x10) {
		buffer[index++] = 0x10;
	}
	for (i = 0; i < length; i++) {
		buffer[index++] = data[i];
		if (data[i] == 0x10) {
			buffer[index++] = 0x10;
		}
	}
	tmp = (char) (crc >> 8);
	buffer[index++] = tmp;
	if (tmp == 0x10) {
		buffer[index++] = 0x10;
	}
	tmp = (char) (crc & 0xff);
	buffer[index++] = tmp;
	if (tmp == 0x10) {
		buffer[index++] = 0x10;
	}
	buffer[index++] = 0x10;
	buffer[index++] = 0x03;

	write(manager->serialFd, buffer, index);

	pthread_mutex_unlock(&(manager->mutex));
}

void start_serial() {
	init_frame_manager();
	extern void start_control_process();
	extern void start_gps_process();
	start_control_process();
#ifndef __x86_64
	start_gps_process();
#endif
}
