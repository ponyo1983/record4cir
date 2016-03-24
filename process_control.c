/*
 * process_control.c
 *
 *  Created on: Mar 11, 2014
 *      Author: lifeng
 */

#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>

#include "serial/frame_manager.h"
#include "serial/frame.h"

#include "lib/hwclock.h"
#include "lib/bcd.h"

#include "lib/block_filter.h"
#include "lib/block_manager.h"

#include "./led/led.h"
#include "./sound/sound.h"

#include "global.h"

pthread_t thread_control;

const char version[16] = "V2.1.01-16/03/11";



static void * proc_control(void *args) {
	struct frame_manager *manager = get_frame_manager(CONTROL_MANAGER);

	struct block_filter *filter = NULL;

	struct frame *pframe = NULL;
	struct block * pblock=NULL;
	char buffer[32];
	char dest_addr = 0;
	char src_addr = 0;
	char operation = 0;
	char cmd = 0;
	char tmp;

	if (manager == NULL)
		return NULL;
	filter=get_frame_filter(manager);
	while (1) {
		pblock = get_block(filter, 2000, BLOCK_FULL);
		if (pblock != NULL) {
			light_on(0);
			pframe=(struct frame*)pblock->data;
			pframe->length=pblock->data_length;
			dest_addr = destination_of_frame(pframe);
			src_addr = pframe->data[4];
			cmd = command_of_frame(pframe);

			if (dest_addr == RECORD_ADDRESS) {
				operation = operation_of_frame(pframe);
				switch (operation) {
				case 1: //维护信息
					// echo off
					tmp = 0;
					if (src_addr == 0x01) {
						tmp = 0x01;
					} else if ((src_addr == 0x03) || (src_addr == 0x04)) {
						tmp = 0x41;
					}
					send_frame(manager, RECORD_ADDRESS, src_addr, 1, tmp,
							pframe->data + 9, 2);

					switch (cmd) {
					case (char) 0x33: //播放控制
						{
							if(pframe->data[10]==(char)0xff)
							{
								start_play(src_addr);
							}
							else if(pframe->data[10]==0)
							{
								printf("stop play\n");
								stop_play();
							}
						}
						break;
					case (char) 0xfa: //问询测试
						break;
					case (char) 0x34: //查询时钟
					{
						struct tm time;
						get_time(&time);
						buffer[0] =to_bcd(time.tm_year - 100) ; //year
						buffer[1] = to_bcd(time.tm_mon + 1);
						buffer[2] = to_bcd(time.tm_mday);
						buffer[3] = to_bcd(time.tm_hour);
						buffer[4] = to_bcd(time.tm_min);
						buffer[5] = to_bcd(time.tm_sec);

						send_frame(manager, RECORD_ADDRESS, src_addr, 1,
								(char) 0x91, buffer, 6);
					}
						break;
					case (char) 0x35: //设置时钟
						break;
					case (char) 0xa5: //查询软件版本
					{

						bcopy(version, buffer, 16);
						send_frame(manager, RECORD_ADDRESS, src_addr, 1,
								(char) 0xaa, buffer, 16);
					}
						break;

					}

					break;
				case 3: //不需要应答
					break;
				}
			}
			put_block(pblock, BLOCK_EMPTY);
		} else {
			//printf("no data\n");
		}
	}

	return NULL;
}

void start_control_process() {
	pthread_create(&thread_control, NULL, proc_control, NULL);
}

