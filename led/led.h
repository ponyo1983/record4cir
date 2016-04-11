/*
 * led.h
 *
 *  Created on: Mar 19, 2014
 *      Author: lifeng
 */

#ifndef LED_H_
#define LED_H_


enum led_mode
{
	LED_NORMAL=0,
	LED_DUMP=1,
	LED_DUMP_FINISHED=2,
};

struct led
{
	int led_no;//编号
	int fd;
	int time_out;
	int time_cur;
	int time_max;
	struct led *next;
};


void init_led();
void light_on(int no);
void light_on_always(int no);
void change_led_mode (enum led_mode mode);


void light_gps_alarm(int state);
void light_main_alarm(int state);
void light_battery_alarm(int state);
#endif /* LED_H_ */
