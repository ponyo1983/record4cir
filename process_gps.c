/*
 * process_gps.c
 *
 *  Created on: Mar 19, 2014
 *      Author: lifeng
 */

#include <stdio.h>

#include <pthread.h>

#include "./serial/frame_manager.h"
#include "./lib/block_manager.h"
#include "./lib/block_filter.h"

#include "./led/led.h"

static pthread_t thread_gps;

static void * proc_gps(void *args) {
	struct frame_manager *manager = get_frame_manager(GPS_MANAGER);

	struct block_filter *filter = NULL;

	struct block * pblock = NULL;


	if (manager == NULL)
		return NULL;
	filter = get_frame_filter(manager);
	while (1) {
		pblock = get_block(filter, 5000, BLOCK_FULL);
		if (pblock != NULL) {
			light_on(1);
			put_block(pblock,BLOCK_EMPTY);
		}
		else
		{ //超时

		}
	}
	return NULL;
}


void start_gps_process() {
	pthread_create(&thread_gps, NULL, proc_gps, NULL);
}
