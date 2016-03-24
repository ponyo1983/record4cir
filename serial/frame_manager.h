/*
 * frame_manager.h
 *
 *  Created on: Mar 10, 2014
 *      Author: lifeng
 */

#ifndef FRAME_MANAGER_H_
#define FRAME_MANAGER_H_

#include <pthread.h>

#include "../lib/block_manager.h"

#define BUFFER_SIZE (1*1024)

enum manager_type
{
	CONTROL_MANAGER, //控制板
	GPS_MANAGER, //GPS板
};

struct frame_manager
{
	struct block_manager manager;
	enum manager_type type;
	char serial_name[256];
	pthread_t threadRx;
	int serialFd;
	pthread_mutex_t mutex;

	struct frame_manager *next;

	char buffer_rx[BUFFER_SIZE];
	char frame_buffer[BUFFER_SIZE];
	char metDLE;
	int frame_length;
	char buffer_tx[BUFFER_SIZE];

};
void init_frame_manager();
struct frame_manager* get_frame_manager( enum manager_type type);
struct block_filter * get_frame_filter(struct frame_manager *manager);
void send_frame(struct frame_manager *manager, char src, char dest, char oper,
		char cmd, char *data, int length) ;

#endif /* FRAME_MANAGER_H_ */
