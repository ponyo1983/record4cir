/*
 * block_manager.c
 *
 *  Created on: Mar 15, 2014
 *      Author: lifeng
 */

#include <stdio.h>
#include <string.h>

#include <pthread.h>

#include "block_filter.h"
#include "block_manager.h"

void add_block_data(struct block_manager * manager, char *data, int length,
		int timeout) {

	struct block_filter*filter = manager->fliters;
	struct block * pblock = NULL;
	pthread_mutex_lock(&(manager->mutex));

	while (filter != NULL) {
		if ((filter->match != NULL) && !(filter->match(data, length)))
			continue;

		pblock = get_block(filter, timeout, BLOCK_EMPTY);
		if (pblock != NULL) {

			memcpy(pblock->data, data, length);
			pblock->data_length = length;
			put_block(pblock, BLOCK_FULL);
		} else {
			printf("get block err\n");
		}
		filter = filter->next;

	}

	pthread_mutex_unlock(&(manager->mutex));
}

struct block_filter * add_block_filter(struct block_manager *manager,
		int (*match)(char*, int), int block_size, int block_num) {

	struct block_filter * filter = create_block_filter(block_size, block_num);

	if (filter != NULL) {
		filter->match = match;
		pthread_mutex_lock(&(manager->mutex));

		filter->next = manager->fliters;
		manager->fliters = filter;

		pthread_mutex_unlock(&(manager->mutex));

	}

	return filter;
}

void remove_block_filter(struct block_manager *manager,
		struct block_filter * filter) {

	pthread_mutex_lock(&(manager->mutex));
	struct block_filter * filter_cur = manager->fliters;
	struct block_filter * filter_prev=NULL;
	while (filter_cur != NULL) {
		if (filter_cur == filter) {

			if(filter_prev==NULL)
			{
				manager->fliters=filter_cur->next;
			}
			else
			{
				filter_prev->next=filter_cur->next;
			}
			break;
		}
		else
		{
			filter_prev=filter_cur;
		}
	}

	pthread_mutex_unlock(&(manager->mutex));

}

