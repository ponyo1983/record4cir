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

	struct led *next;
};


void init_led();
void light_on(int no);
void change_led_mode (enum led_mode mode);

#endif /* LED_H_ */
