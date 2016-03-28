/*
 * storage_manager.h
 *
 *  Created on: Mar 11, 2014
 *      Author: lifeng
 */

#ifndef STORAGE_MANAGER_H_
#define STORAGE_MANAGER_H_

#include <stdio.h>
#include <pthread.h>

#include "record_dic.h"

#include "../lib/block_manager.h"

#include "../lib/block_filter.h"

#define SERIAL_BUFFER_SIZE	(4*1024)

struct record_manager {
	struct block_manager manager;
	struct record_dic dics[2];
	int date_tbl[4][128];
	pthread_t thread_store;
	FILE * file;

	char serial_buffer[SERIAL_BUFFER_SIZE];
	int serial_length;

};

struct record_manager* get_record_manager();
void request_playback(struct record_manager*manager, int play_index,
		struct block*wav_block);
void request_data(struct record_manager*manager, int section, char *data,
		int length);
void store_serial_data(struct record_manager * manager, char *data, int length);
void store_wave_data(struct record_manager * manager, struct block *pblock);
#endif /* STORAGE_MANAGER_H_ */
