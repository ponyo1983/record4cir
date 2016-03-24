/*
 * record_dic.c
 *
 *  Created on: Mar 12, 2014
 *      Author: lifeng
 */

#include <string.h>

#include "record_dic.h"
#include "../lib/crc16.h"

#define STATS_STORE_SIZE	(2*1024*1024)

#define SIZE_1M	(1024*1024)

const char tag[] = "CIR.RECORD";
const char tag_sts[] = "STATUS";
const char tag_ser[] = "SERIAL";
const char tag_wav[] = "WAVE";

int record_dic_valid(struct record_dic *dic) {


	int ret = strncmp(dic->tag, tag, strlen(tag));
	if (ret != 0)
		return 1;

	ret = strncmp(dic->sections[0].tag, tag_sts, strlen(tag_sts));
	if (ret != 0)
		return 1;
	ret = strncmp(dic->sections[1].tag, tag_ser, strlen(tag_ser));
	if (ret != 0)
		return 1;
	ret = strncmp(dic->sections[2].tag, tag_wav, strlen(tag_wav));
	if (ret != 0)
		return 1;

	ret = compute_crc16(dic, 510);
	if (dic->crc != ret)
		return 1;

	return 0;

}

void init_record_dic(struct record_dic *dic, int sec_size, int sec_num) {

	long total_size = (sec_size * sec_num - 10 * SIZE_1M);

	long serial_size = (total_size / 3) / SIZE_1M * SIZE_1M; //alignment 1M byte
	long wav_size = (total_size - serial_size) / SIZE_1M * SIZE_1M; //alignment 1M byte
	long offset=512*6;
	memset(dic, 0, 512);
	strcpy(dic->tag, tag);
	dic->sec_size = sec_size;
	dic->sec_num = sec_num;
	strcpy(dic->sections[0].tag, tag_sts);
	dic->sections[0].date_offset=512*2;
	dic->sections[0].date_total=512;
	dic->sections[0].offset = offset;
	dic->sections[0].total = STATS_STORE_SIZE;
	strcpy(dic->sections[1].tag, tag_ser);
	dic->sections[1].date_offset=512*3;
	dic->sections[1].date_total=512;
	dic->sections[1].offset = offset + STATS_STORE_SIZE;
	dic->sections[1].total = serial_size;
	strcpy(dic->sections[2].tag, tag_wav);
	dic->sections[2].date_offset=512*4;
	dic->sections[2].date_total=512;
	dic->sections[2].offset = offset + STATS_STORE_SIZE+serial_size;
	dic->sections[2].total=wav_size;
	dic->last_wav[0]=-1;
	dic->last_wav[1]=-1;
	dic->last_wav[2]=-1;
	dic->last_wav[3]=-1;
	dic->last_wav[4]=-1;
	dic->crc=compute_crc16(dic, 510);

}

