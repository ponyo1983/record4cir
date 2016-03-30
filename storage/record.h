/*
 * record.h
 *
 *  Created on: Mar 12, 2014
 *      Author: lifeng
 */

#ifndef RECORD_H_
#define RECORD_H_

#define RECORD_DATA_SIZE	(512) //最大不能超过2^16Byte


enum record_type {
	RECORD_STATUS = 0,
	RECORD_SERIAL = 1,
	RECORD_WAVE = 2,
	RECORD_DUMP_STATUS=3, //转储状态数据请求
	RECORD_DUMP_SERIAL=4,//转储串口数据请求
	RECORD_DUMP_WAVE=5,//转储声音请求
	RECORD_PLAYBACK=6, //请求回放数据
	RECORD_FLUSH_DATA=7, //刷回数据
};

struct record_header
{
	char tag[3];
	char type;
	union
	{
		struct
		{
			unsigned char serial_len;
			char status;
			unsigned short lattery;
		};
		int wave_size;
	};

	char year;
	char month;
	char day;
	char hour;
	char minute;
	char second;
	unsigned short millsec;

}__attribute__((packed));




struct record
{
	struct record_header header;
	char data[RECORD_DATA_SIZE];
	void *wave_data;


};

#endif /* RECORD_H_ */
