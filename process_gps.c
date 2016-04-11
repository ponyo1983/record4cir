/*
 * process_gps.c
 *
 *  Created on: Mar 19, 2014
 *      Author: lifeng
 */

#include <stdio.h>

#include <pthread.h>

#include "./serial/frame_manager.h"
#include "./lib/block_manager.h"
#include "./lib/block_filter.h"
#include "./lib/bcd.h"
#include "./led/led.h"

#include "./serial/frame.h"
#include "./lib/hwclock.h"

#include "./state/state.h"


static pthread_t thread_gps;

static void * proc_gps(void *args) {
	struct frame_manager *manager = get_frame_manager(GPS_MANAGER);

	struct block_filter *filter = NULL;

	struct block * pblock = NULL;

	struct frame *pframe = NULL;

	char firstGPS = 1;
	char gpsSate = 0;
	char year, month, day, hour, minute, second;

	if (manager == NULL)
		return NULL;
	filter = get_frame_filter(manager);
	while (1) {

		pblock = get_block(filter, TIMEOUT_GPS, BLOCK_FULL);
		if (pblock != NULL) {
			set_sys_state(BIT3_GPS,STATE_GPS_OK);
			light_on(1);
			light_gps_alarm(0);
			pframe = (struct frame*) (pblock->data);
			pframe->length = pblock->data_length;

			gpsSate = pframe->data[12];
			if (gpsSate == (char) 0x41) { //gps time valid

				set_sys_state(BIT4_GPS_VALID,STATE_GPS_VALID);
				year = from_bcd(pframe->data[54]);
				month = from_bcd(pframe->data[55]);
				day = from_bcd(pframe->data[56]);
				hour = from_bcd(pframe->data[57]);
				minute = from_bcd(pframe->data[58]);
				second = from_bcd(pframe->data[59]);
				if ((year < 100) && (month < 13) && (day < 32) && (hour < 24)
						&& (minute < 60) && (second < 60)) {
					struct tm tGPS;
					tGPS.tm_year = (2000 - 1900) + year;
					tGPS.tm_mon = month - 1;
					tGPS.tm_mday = day;
					tGPS.tm_hour = hour;
					tGPS.tm_min = minute;
					tGPS.tm_sec = second;
					if (firstGPS) {

						set_time(&tGPS);
						set_time2(&tGPS);
						firstGPS = 0;
					} else {
						struct tm tSys;
						get_time(&tSys);
						if ((tGPS.tm_year != tSys.tm_year)
								|| (tGPS.tm_mon != tSys.tm_mon)
								|| (tGPS.tm_mday != tSys.tm_mday)) {
							set_time(&tGPS);
							set_time2(&tGPS);
						} else {
							int s1 = hour * 60 * 60 + minute * 60 + second;
							int s2 = tSys.tm_hour * 60 * 60 + tSys.tm_min * 60
									+ tSys.tm_sec;
							int diff = s1 < s2 ? (s2 - s1) : (s1 - s2);
							if (diff > 30) {
								set_time(&tGPS);
								set_time2(&tGPS);
							}
						}
					}

				}

			} else if (gpsSate == (char) 0x56) {
				set_sys_state(BIT4_GPS_VALID,STATE_GPS_INVALID);
			}

			put_block(pblock, BLOCK_EMPTY);

		} else { //超时
			set_sys_state(BIT3_GPS,STATE_GPS_FAIL);
			set_sys_state(BIT4_GPS_VALID,STATE_GPS_INVALID);
			light_gps_alarm(1);
		}
	}
	return NULL;
}

void start_gps_process() {
	pthread_create(&thread_gps, NULL, proc_gps, NULL);
}
