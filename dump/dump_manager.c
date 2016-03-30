/*
 * dump_manager.c
 *
 *  Created on: Mar 14, 2014
 *      Author: lifeng
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mount.h>

#include "dump.h"
#include "dump_manager.h"

#include "../storage/record_manager.h"

#include "../lib/block_manager.h"
#include "../lib/block_filter.h"

#include "../led/led.h"
#include "../config/config.h"

#include "../lib/hwclock.h"

#ifdef __x86_64
const char NAME_PREFIX[] = "sdb"; //模拟机
#else
const char NAME_PREFIX[] = "sd"; //实际ARM开发板
#endif

const char MOUNT_POINT[] = "/mnt";

const char KEY_BEGINTIME[] = "BeginTime=";
const char KEY_ENDTIME[] = "EndTime=";
const char COPY_ALL[] = "CopyAll=y";
static struct dump_manager* gpdump_manager = NULL;

static void * get_extern_device() {

	static char device_name[50];

	char buffer[200];
	char name[20];
	int length;
	int i, j;
	int next_space;
	int segment_index = 0;
	int major = 0;
	//int minor = 0;

	memset(name, 0, sizeof(name));
	FILE* file = fopen("/proc/partitions", "r");
	if (file == NULL)
		return NULL;

	while (fgets(buffer, sizeof(buffer), file)) {

		length = strlen(buffer);
		if (length <= 5)
			continue;
		major = 0;
		//minor = 0;
		segment_index = 0;
		for (i = 0; i < length; i++) {
			if (isspace(buffer[i]))
				continue;
			next_space = i + 1;
			for (j = i + 1; j < length; j++) {
				if (isspace(buffer[j])) {
					buffer[j] = '\0';
					break;
				}
				next_space = j + 1;
			}
			switch (segment_index) {
			case 0:
				major = atoi(buffer + i);
				break;
			case 1:
				//minor = atoi(buffer + i);
				break;
			case 2:
				//#block num
				break;
			case 3:
				if (strncmp(buffer + i, NAME_PREFIX, sizeof(NAME_PREFIX) - 1)
						== 0) {
					strncpy(name, buffer + i, next_space - i);
				}

				break;
			}
			if (major != 8)
				break;
			i = next_space;
			segment_index++;
		}

	}

	fclose(file);

	length = strlen(name);
	if (length < 3) {
		return NULL;
	}
	sprintf(device_name, "/dev/%s", name);

	return device_name;
}

static void wait_usb_plugout(struct dump_manager *manager) {
	char *device = NULL;

	while (1) {

		device = get_extern_device();
		if (device == NULL)
			break;
		sleep(1);
	}
}

static void wait_usb_plugin(struct dump_manager *manager) {
	char *device = NULL;

	while (1) {

		device = get_extern_device();
		if (device == NULL) {
			memset(manager->device_name, 0, 50);
			manager->device_exit_time = 0;
		} else {
			if (strcmp(manager->device_name, device) == 0) {
				manager->device_exit_time = manager->device_exit_time + 1;
			} else {
				manager->device_exit_time = 1;
				strcpy(manager->device_name, device);
			}

		}
		if (manager->device_exit_time == 3) {
			break;
		}
		sleep(1);
	}
}

static int mount_usb(struct dump_manager *manager) {
	char * device = manager->device_name;
	//返回0。失败返回-1
	int ret = mount(device, MOUNT_POINT, "vfat", 0, NULL);
	return ret;
}

static void unmount_usb(struct dump_manager *manager) {

	umount(MOUNT_POINT);
	umount(MOUNT_POINT);
}

static int exe_update(struct dump_manager * manager) {
	char file_update[50];

	sprintf(file_update, "%s/update", MOUNT_POINT);
	if (access(file_update, F_OK | X_OK) != 0)
		return -1;
	system(file_update);
	return 0;
}

static int get_config_info(struct dump_manager *manager) {
	FILE * file = NULL;
	char file_name[50];
	char buffer[100];

	int year, month, day;
	manager->acordding_time = 0;
	manager->copy_all=0;
	manager->begin_time = 0;
	manager->end_time = (99<<16)|(12<<8)|(31); //2099-12-31
	sprintf(file_name, "%s/config.ini", MOUNT_POINT);
	file = fopen(file_name, "r");
	if (file != NULL) {
		manager->acordding_time = 1; //根据时间拷贝数据
		while (fgets(buffer, sizeof(buffer), file)) {
			year = -1;
			month = -1;
			day = -1;
			if (strncmp(buffer, KEY_BEGINTIME, strlen(KEY_BEGINTIME)) == 0) { //开始时间
				sscanf(buffer, "BeginTime=%d-%d-%d", &year, &month, &day);
				if (year >= 2000 && month > 0 && day > 0) {
					manager->begin_time = ((year - 2000) << 16) | (month<<8) | (day);
				}
			} else if (strncmp(buffer, KEY_ENDTIME, strlen(KEY_ENDTIME)) == 0) { //结束时间
				sscanf(buffer, "EndTime=%d-%d-%d", &year, &month, &day);
				if (year >= 2000 && month > 0 && day > 0) {
					manager->end_time = ((year - 2000) << 16) | (month<<8) | (day);
				}
			}
			else if(strncmp(buffer, COPY_ALL, strlen(COPY_ALL)) == 0)
			{
				manager->copy_all=1;
			}

		}

		fclose(file);
	}
	return 0;
}

static int mk_dir(char *dir) {
	DIR *mydir = NULL;
	if ((mydir = opendir(dir)) == NULL) {
		int ret = mkdir(dir, 0755);
		if (ret != 0) {
			return -1;
		}
	} else {
		closedir(mydir);
	}
	return 0;
}

static void dump_data(struct dump_manager *manager, int section) {
	char buffer[256];
	char buffer1[256];
	FILE * file = NULL;
	struct dump* pdump = NULL;
	struct block * pblock = NULL;
	int i;
	int ack = 0;
	struct record_manager * record_manager = get_record_manager();
	struct block_filter * filter = manager->manager.fliters;
	if (section < 0 || section > 2)
		return;
	sprintf(buffer, "%s/%s/", MOUNT_POINT, manager->ID);

	if (mk_dir(buffer) == 0) {
		struct tm *tNow;
		tNow=&(manager->time_export);
		sprintf(buffer1, "%s/%4d-%2d-%2d %2d,%2d,%2d/",buffer, tNow->tm_year+1900, tNow->tm_mon+1, tNow->tm_mday, tNow->tm_hour, tNow->tm_min, tNow->tm_sec);
		if (mk_dir(buffer1) == 0) {

			switch (section) {
						case 0:
							sprintf(buffer, "%s/Menu.bin", buffer1);
							break;
						case 1:
							sprintf(buffer, "%s/Data.bin", buffer1);
							break;
						case 2:
							sprintf(buffer, "%s/Wave.bin", buffer1);
							break;
						}
		} else {
			switch (section) {
			case 0:
				sprintf(buffer, "%s/%s/Menu.bin", MOUNT_POINT, manager->ID);
				break;
			case 1:
				sprintf(buffer, "%s/%s/Data.bin", MOUNT_POINT, manager->ID);
				break;
			case 2:
				sprintf(buffer, "%s/%s/Wave.bin", MOUNT_POINT, manager->ID);
				break;
			}
		}

	} else {
		switch (section) {
		case 0:
			sprintf(buffer, "%s/Menu.bin", MOUNT_POINT);
			break;
		case 1:
			sprintf(buffer, "%s/Data.bin", MOUNT_POINT);
			break;
		case 2:
			sprintf(buffer, "%s/Wave.bin", MOUNT_POINT);
			break;
		}
	}

	for (i = 0; i < 3; i++) { //尝试3次
		request_data(record_manager, section, (char*) manager,
				sizeof(struct dump_manager));
		pblock = get_block(filter, 1000, BLOCK_FULL);
		if (pblock != NULL) {
			pdump = (struct dump*) pblock->data;
			if (pdump->type == section * 3) {
				ack = 1;
				put_block(pblock, BLOCK_EMPTY);
				break;
			} else {
				printf("ack type%d!\n", (char) pdump->type);
			}
			put_block(pblock, BLOCK_EMPTY);
		} else {
			printf("ack err%d!\n", i);
		}

	}
	if (ack == 0) {
		printf("ack err!\n");
		return;
	}

	file = fopen(buffer, "w+");

	if (file == NULL)
		return;
	while (1) {
		pblock = get_block(filter, 1000, BLOCK_FULL);
		if (pblock != NULL) {
			pdump = (struct dump*) pblock->data;
			if (pdump->type == (section * 3 + 2)) { //finished
				put_block(pblock, BLOCK_EMPTY);
				break;
			} else if (pdump->type == (section * 3 + 1)) {

				fwrite(pdump->data, pdump->length, 1, file);
				fflush(file);
			}
			put_block(pblock, BLOCK_EMPTY);
		} else {
			//printf("dump is null\n");
		}
	}

	fclose(file);

}
static void dump_status(struct dump_manager *manager) {
	dump_data(manager, 0);
}
static void dump_serial(struct dump_manager *manager) {
	dump_data(manager, 1);
}
static void dump_wave(struct dump_manager *manager) {
	dump_data(manager, 2);
}

static void get_record_id(struct dump_manager *manager) {
	char *id = get_id();
	memcpy(manager->ID, id, 32);
}
void * dump_proc(void * args) {

	struct dump_manager *manager = (struct dump_manager*) args;
	while (1) {
		wait_usb_plugin(manager);
		unmount_usb(manager);
		if (mount_usb(manager) == 0) {

			get_record_id(manager);
			change_led_mode(LED_DUMP);
			if (exe_update(manager) != 0) {
				get_config_info(manager);

				get_time(&(manager->time_export));
				dump_status(manager);
				dump_serial(manager);
				dump_wave(manager);
			}
			unmount_usb(manager);

			change_led_mode(LED_DUMP_FINISHED);
			wait_usb_plugout(manager);

			change_led_mode(LED_NORMAL);
		}

	}

	return NULL;
}

struct dump_manager* get_dump_manager() {
	if (gpdump_manager == NULL) {
		gpdump_manager = malloc(sizeof(struct dump_manager));
		if (gpdump_manager != NULL) {
			memset(gpdump_manager, 0, sizeof(struct dump_manager));

			add_block_filter(&(gpdump_manager->manager), NULL,
					sizeof(struct dump), 8);

			get_record_id(gpdump_manager);
			pthread_create(&(gpdump_manager->thread_dump), NULL, dump_proc,
					gpdump_manager);
		}
	}
	return gpdump_manager;
}

void start_dump() {

	get_dump_manager();

}

void send_dump(struct dump_manager *manager, int type, int num, char *data,
		int length) {
	struct block_filter * filter = manager->manager.fliters;

	struct block *pblock = get_block(filter, 5000, BLOCK_EMPTY); //usb存储慢得等下
	struct dump* dump = NULL;
	if (pblock != NULL) {
		dump = (struct dump*) pblock->data;
		dump->type = type;
		dump->length = length;
		dump->data_num = num;
		memcpy(dump->data, data, length);
		put_block(pblock, BLOCK_FULL);
	} else {
		printf("send_dump err\n");
	}

}
