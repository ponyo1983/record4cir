/*
 * block_filter.c
 *
 *  Created on: Mar 15, 2014
 *      Author: lifeng
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <time.h>
#include <sys/time.h>

#include "block.h"
#include "block_filter.h"

struct block_filter * create_block_filter(int block_size, int block_num) {
	struct block_filter * filter = NULL;
	struct block * blocks = NULL;
	u_char *data = NULL;
	int i;

	filter = malloc(sizeof(struct block_filter));

	if (filter == NULL)
		goto OK;
	blocks = malloc(block_num * sizeof(struct block));
	if (blocks == NULL)
		goto Fail1;
	data = malloc(block_size * block_num);
	if (data == NULL)
		goto Fail2;

	memset(filter, 0, sizeof(struct block_filter));
	memset(blocks, 0, block_num * sizeof(struct block));
	filter->block_data=data;
	filter->block_size = block_size;
	filter->block_num = block_num;
	filter->blosk_list = blocks;
	filter->full_list = NULL;
	filter->empty_list = NULL;
	pthread_cond_init(&(filter->cond_full), NULL);
	pthread_mutex_init(&(filter->mutex_full), NULL);

	pthread_cond_init(&(filter->cond_empt), NULL);
	pthread_mutex_init(&(filter->mutex_empt), NULL);

	for (i = 0; i < block_num; i++) {
		blocks[i].filter=filter;
		blocks[i].block_no = i;
		blocks[i].block_size = block_size;
		blocks[i].data = data + i * block_size;

		if (filter->empty_list == NULL) {
			blocks[i].next = blocks + i;
			blocks[i].prev = blocks + i;
			filter->empty_list = blocks + i;
		} else {
			blocks[i].next = filter->empty_list;
			blocks[i].prev = filter->empty_list->prev;
			filter->empty_list->prev->next = blocks + i;
			filter->empty_list->prev = blocks + i;
		}

	}
	goto OK;

	Fail2: free(blocks);
	Fail1: free(filter);
	filter = NULL;
	OK: return filter;
}

/*
 *timeout >0 wait for (ms), =0 no wait ,<0 waitforever
 *type  frame's type
 * */
struct block *get_block(struct block_filter* filter, int timeout,
		enum block_status status) {

	struct block** pblock_head = NULL;
	struct block *pblock = NULL;
	pthread_cond_t * cond = NULL;
	pthread_mutex_t *mutex = NULL;

	struct timespec abstime;
	struct timeval now;

	if (status == BLOCK_EMPTY) {
		pblock_head = &(filter->empty_list);
		mutex = &(filter->mutex_empt);
		cond = &(filter->cond_empt);
	} else {
		pblock_head = &(filter->full_list);
		mutex = &(filter->mutex_full);
		cond = &(filter->cond_full);
	}
	pthread_mutex_lock(mutex);
	pblock = *pblock_head;
	if (pblock != NULL) {

		if (pblock->next == pblock) {
			*pblock_head = NULL;
		} else {
			pblock->prev->next = pblock->next;
			pblock->next->prev = pblock->prev;
			*pblock_head = pblock->next;
		}

	} else {
		if (timeout < 0) {
			pthread_cond_wait(cond, mutex);

		} else if (timeout > 0) {
			gettimeofday(&now, NULL);
			long nsec = now.tv_usec * 1000 + (timeout % 1000) * 1000000;
			abstime.tv_nsec = nsec % 1000000000;
			abstime.tv_sec = now.tv_sec + nsec / 1000000000 + timeout / 1000;
			pthread_cond_timedwait(cond, mutex, &abstime);
		}
		pblock = *pblock_head;
		if (pblock != NULL) {
			if (pblock->next == pblock) {
				*pblock_head = NULL;
			} else {
				pblock->prev->next = pblock->next;
				pblock->next->prev = pblock->prev;
				*pblock_head = pblock->next;
			}
		}
	}

	pthread_mutex_unlock(mutex);

	return pblock;
}

void put_block(struct block *pblock,enum block_status status) {

	struct block** pblock_head = NULL;
	pthread_cond_t * cond = NULL;
	pthread_mutex_t *mutex = NULL;

	struct block_filter* filter=pblock->filter;
	if (status == BLOCK_EMPTY) {
		pblock_head = &(filter->empty_list);
		mutex = &(filter->mutex_empt);
		cond = &(filter->cond_empt);
	} else {
		pblock_head = &(filter->full_list);
		mutex = &(filter->mutex_full);
		cond = &(filter->cond_full);
	}
	pthread_mutex_lock(mutex);
	if (*pblock_head == NULL) {
		pblock->next = pblock;
		pblock->prev = pblock;
		*pblock_head = pblock;
	} else {
		pblock->next = *pblock_head;
		pblock->prev = (*pblock_head)->prev;
		(*pblock_head)->prev->next = pblock;
		(*pblock_head)->prev = pblock;
	}

	pthread_cond_signal(cond);
	pthread_mutex_unlock(mutex);

}

