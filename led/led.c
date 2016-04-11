/*
 * led.c
 *
 *  Created on: Mar 19, 2014
 *      Author: lifeng
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

#include "led.h"

#define TIMEOUT_MS	(100)
#define DUMP_MS	(20)

#ifdef __x86_64
const char com_name[] = "/dev/null";
const char gps_name[] = "/dev/null";
const char record_name[] = "/dev/null";
#else
const char com_name[] = "/sys/class/leds/com/brightness";
const char gps_name[] = "/sys/class/leds/gps/brightness";
const char record_name[] = "/sys/class/leds/record/brightness";
const char battery_name[] = "/sys/class/leds/alarm/brightness";
#endif

#define MAX_LED_FLASH_VAL	(10)

static void * led_proc(void *args);

static struct led* led_list = NULL;
static pthread_t thread_led;

static char alarm_led_state = 0;

static int led_mode = (int) LED_NORMAL;

static void change_light_force(int no, char state, int force, int timeout) { //timeout 取值[-1,-10] 或者 >0
	struct led* pled = led_list;
	char buffer[4];
	if ((led_mode != (int) LED_NORMAL) && (force == 0))
		return;
	while (pled != NULL) {
		if (pled->led_no == no) {

			if (pled->fd > 0) {

				buffer[0] = state;
				if(timeout==0)buffer[0]='0';
				write(pled->fd, buffer, 1);
				pled->time_out = timeout;
				if (timeout < 0) {
					pled->time_cur = timeout;
					pled->time_max = MAX_LED_FLASH_VAL + timeout;
				}

			}
			break;
		}
		pled = pled->next;
	}
}

void light_on(int no) {
	change_light_force(no, '1', 0, 3);
}

void light_on_always(int no) {
	change_light_force(no, '1', 0, -MAX_LED_FLASH_VAL);
}

void light_gps_alarm(int state) { //gps点亮200ms

	if(((alarm_led_state>>1)&0x01)==state) return;

	if (state == 0) {
		alarm_led_state = alarm_led_state & (~2);
	} else {
		alarm_led_state = alarm_led_state | (2);
	}

	change_light_force(3, '1', 0, -alarm_led_state);
}

void light_main_alarm(int state) { ////main unit点亮400ms
	if(((alarm_led_state>>2)&0x01)==state) return;
	if (state == 0) {
		alarm_led_state = alarm_led_state & (~4);
	} else {
		alarm_led_state = alarm_led_state | (4);
	}
	change_light_force(3, '1', 0, -alarm_led_state);
}

void light_battery_alarm(int state) {
	if (state == 0) {
		if(alarm_led_state==0) return;
		alarm_led_state = 0;
	} else {
		if(MAX_LED_FLASH_VAL==0) return;
		alarm_led_state = MAX_LED_FLASH_VAL;
	}
	change_light_force(3, '1', 0, -alarm_led_state);
}

void change_led_mode(enum led_mode mode) {
	led_mode = (int) mode;
	if (mode == LED_DUMP) {
		change_light_force(0, '0', 1, 3);
		change_light_force(1, '0', 1, 3);
		change_light_force(2, '0', 1, 3);
	} else if (mode == LED_DUMP_FINISHED) {
		change_light_force(2, '0', 1, 3);
	}
}

static void add_led(const char *name, int no) {
	int fd = 0;
	struct led *pled = malloc(sizeof(struct led));
	if (pled != NULL) {
		pled->time_out = 0;
		fd = open(name, O_WRONLY);
		if (fd < 0) {
			free(pled);
			return;
		}
		pled->fd = fd;
		pled->led_no = no;
		pled->next = led_list;
		led_list = pled;
	}
}

static void * led_proc(void *args) {
	struct led* pled = led_list;
	char buffer[4];

	while (1) {
		if (led_mode == (int) LED_NORMAL) {
			usleep(TIMEOUT_MS * 1000);
			pled = led_list;
			while (pled != NULL) {
				if (pled->time_out > 0) {
					pled->time_out--;
					if (pled->time_out == 0) {
						buffer[0] = '0';
						write(pled->fd, buffer, 1);

					}
				} else if (pled->time_out < 0) {
					pled->time_cur++;
					if (pled->time_cur >= pled->time_max) { //点亮
						pled->time_cur = pled->time_out;
						buffer[0] = '1';
						write(pled->fd, buffer, 1);
					}
					if (pled->time_cur == 0) { //熄灭
						buffer[0] = '0';
						write(pled->fd, buffer, 1);

					}
				}
				pled = pled->next;
			}
		} else if (led_mode == (int) LED_DUMP) {
			usleep(DUMP_MS * 1000);
			change_light_force(2, '1', 1, 3);
			usleep(DUMP_MS * 1000);
			change_light_force(2, '0', 1, 3);
		} else {
			usleep(100000);
		}

	}
	return NULL;

}
void init_led() {

	add_led(com_name, 0);
	add_led(gps_name, 1);
	add_led(record_name, 2);
	add_led(battery_name, 3);
	pthread_create(&thread_led, NULL, led_proc, NULL);
}

