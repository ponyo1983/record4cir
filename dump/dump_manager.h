/*
 * dump_manager.h
 *
 *  Created on: Mar 14, 2014
 *      Author: lifeng
 */

#ifndef DUMP_MANAGER_H_
#define DUMP_MANAGER_H_

#include <pthread.h>

#include "../lib/block_manager.h"

struct dump_manager
{
	struct block_manager manager;
	char ID[50];
	char device_name[50];
	int device_exit_time;
	int acordding_time;
	int copy_all;
	int begin_time; //格式 yy[2]mm[1]dd[0]
	int end_time;

	struct tm time_export;

	pthread_t thread_dump;

};

struct dump_manager* get_dump_manager();
void send_dump(struct dump_manager *manager, int type,int num, char *data, int length);

#endif /* DUMP_MANAGER_H_ */
