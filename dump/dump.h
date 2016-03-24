/*
 * dump.h
 *
 *  Created on: Mar 15, 2014
 *      Author: lifeng
 */

#ifndef DUMP_H_
#define DUMP_H_

#define DUMP_SIZE	(512*1024)

enum dump_type {
	DUMP_STATUS_ACK = 0,
	DUMP_STATUS_DATA = 1,
	DUMP_STATUS_FINISHED = 2,
	DUMP_SERIAL_ACK = 3,
	DUMP_SERIAL_DATA = 4,
	DUMP_SERIAL_FINISHED = 5,
	DUMP_WAVE_ACK = 6,
	DUMP_WAVE_DATA = 7,
	DUMP_WAVE_FINISHED = 8,
};

struct dump {

	int type;
	int size; //最大数据的大小
	int length; //实际数据大小
	int data_num; //数据编号
	char data[DUMP_SIZE];

};

#endif /* DUMP_H_ */
