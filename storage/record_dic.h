/*
 * record_dic.h
 *
 *  Created on: Mar 12, 2014
 *      Author: lifeng
 */

#ifndef RECORD_DIC_H_
#define RECORD_DIC_H_

# include <bits/types.h>

struct record_dic {
	//64-0
	char tag[16];
	__int64_t sec_size;
	__int64_t sec_num;
	char r0[32];
	struct {
		char tag[16];
		__int64_t date_offset;
		__int64_t date_total;
		__int64_t date_next_off;
		__int64_t date_size;
		__int64_t offset;
		__int64_t total;
		__int64_t next_off; //下一个写入的偏移地址
		__int64_t size; //保存的数据的长度
		__int64_t last_next_off;
		__int64_t last_size;
	} sections[3];
	__int64_t last_wav[5]; //最近5条的录音记录
	char reserved[512 - 16 * 4 - 16 * 6 * 3 - 40 - 2];
	unsigned short crc;

}__attribute__((packed));

int record_dic_valid(struct record_dic *dic);
void init_record_dic(struct record_dic *dic, int sec_size, int sec_num);

#endif /* RECORD_DIC_H_ */
