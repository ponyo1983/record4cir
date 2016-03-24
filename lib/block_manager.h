/*
 * block_manager.h
 *
 *  Created on: Mar 15, 2014
 *      Author: lifeng
 */

#ifndef BLOCK_MANAGER_H_
#define BLOCK_MANAGER_H_

#include <pthread.h>

struct block_manager
{
	pthread_mutex_t mutex;

	struct block_filter *fliters;

};

void add_block_data(struct block_manager * manager, char *data, int length,
		int timeout);
struct block_filter * add_block_filter(struct block_manager *manager,
		int (*match)(char*, int), int block_size, int block_num);
void remove_block_filter(struct block_manager *manager,
		struct block_filter * filter);
#endif /* BLOCK_MANAGER_H_ */
