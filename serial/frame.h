/*
 * frame.h
 *
 *  Created on: Mar 10, 2014
 *      Author: lifeng
 */

#ifndef FRAME_H_
#define FRAME_H_


#define MAX_FRAME_LENGTH (256)

struct frame
{

	char data[MAX_FRAME_LENGTH];
	int length;
};

char destination_of_frame(struct frame * pframe);
char operation_of_frame(struct frame * pframe);
char command_of_frame(struct frame * pframe);
#endif /* FRAME_H_ */
