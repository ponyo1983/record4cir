/*
 * block_filter.h
 *
 *  Created on: Mar 15, 2014
 *      Author: lifeng
 */

#ifndef BLOCK_FILTER_H_
#define BLOCK_FILTER_H_

#include <pthread.h>

#include "block.h"

enum block_status {
	BLOCK_EMPTY, BLOCK_FULL,
};

struct block_filter {
	char * block_data;
	int block_size;
	int block_num;
	struct block * blosk_list;
	struct block * empty_list;
	struct block * full_list;
	int (*match)(char *date, int length);
	pthread_cond_t cond_empt;
	pthread_mutex_t mutex_empt;

	pthread_cond_t cond_full;
	pthread_mutex_t mutex_full;

	struct block_filter *next;
};

struct block_filter * create_block_filter(int block_size, int block_num);
struct block *get_block(struct block_filter* filter, int timeout,
		enum block_status status);
void put_block(struct block *pblock,
		enum block_status status);
#endif /* BLOCK_FILTER_H_ */
