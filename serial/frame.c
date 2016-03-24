/*
 * frame.c
 *
 *  Created on: Mar 10, 2014
 *      Author: lifeng
 */
#include "frame.h"

char destination_of_frame(struct frame * pframe)
{
	char *data=pframe->data;
	return data[6+data[5]];
}

char operation_of_frame(struct frame * pframe)
{
	char *data=pframe->data;
	int src_addr_len=data[5];
	int dest_addr_len=data[7+data[5]];
	return data[8+src_addr_len+dest_addr_len];

}

char command_of_frame(struct frame * pframe)
{
	char *data=pframe->data;
	int src_addr_len=data[5];
	int dest_addr_len=data[7+data[5]];
	return data[9+src_addr_len+dest_addr_len];
}
