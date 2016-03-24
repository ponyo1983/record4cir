/*
 * block.c
 *
 *  Created on: Mar 15, 2014
 *      Author: lifeng
 */
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "block.h"

#include "../storage/record.h"

void init_wave_block(struct block *pblock) {
	struct timeval tv;
	struct tm* ptm;
	long milliseconds;
	if (pblock != NULL) {
		/* 获得日期时间，并转化为 struct tm。 */
		gettimeofday(&tv, NULL);
		ptm = localtime(&tv.tv_sec);
		milliseconds = tv.tv_usec / 1000;

		struct record_header *header = (struct record_header *) pblock->data;

		header->tag[0] = 'W';
		header->tag[1] = 'A';
		header->tag[2] = 'V';
		header->type = RECORD_WAVE;

		header->year = ptm->tm_year - 100;
		header->month = ptm->tm_mon + 1;
		header->day = ptm->tm_mday;
		header->hour = ptm->tm_hour;
		header->minute = ptm->tm_min;
		header->second = ptm->tm_sec;
		header->millsec = milliseconds;

	}
}

void set_wave_block_length(struct block *pblock, int length) {
	if (pblock != NULL) {

		struct record_header *header = (struct record_header *) pblock->data;
		header->wave_size=length;

	}
}
