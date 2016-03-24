/*
 * block.h
 *
 *  Created on: Mar 15, 2014
 *      Author: lifeng
 */

#ifndef BLOCK_H_
#define BLOCK_H_


struct block
{
	struct block_filter* filter;
	struct block* next;
	struct block* prev;
	int block_no; //block 编号
	int block_size;
	int data_length;
	unsigned char * data;
};

void init_wave_block(struct block *pblock);
void set_wave_block_length(struct block *pblock, int length);
#endif /* BLOCK_H_ */
