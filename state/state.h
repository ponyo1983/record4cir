/*
 * state.h
 *
 *  Created on: Apr 8, 2016
 *      Author: lifeng
 */

#ifndef STATE_STATE_H_
#define STATE_STATE_H_


#define TIMEOUT_GPS	(5000)
#define TIMEOUT_MAIN_UNIT	(9000)

#define BATTERY_ALARM_VOLT	(1400)

#define BIT0_POWER	(0)
#define BIT1_RECORD	(1)
#define BIT2_RESET	(2)
#define BIT3_GPS	(3)
#define BIT4_GPS_VALID	(4)
#define BIT5_MAIN_UNIT	(5)
#define BIT6_BATTERY	(6)



#define STATE_POWER_ON	(1)
#define STATE_POWER_OFF	(0)

#define STATE_RECORD_ON		(0)
#define STATE_RECORD_OFF	(1)



#define STATE_RESET_DOWN	(0)
#define STATE_RESET_UP	(1)


#define STATE_GPS_OK	(1)
#define STATE_GPS_FAIL	(0)

#define STATE_GPS_VALID	(0)
#define STATE_GPS_INVALID	(1)

#define STATE_MAINUNIT_OK	(0)
#define STATE_MAINUNIT_FAIL	(1)

#define STATE_BATTERY_OK	(0)
#define STATE_BATTERY_FAIL	(1)


extern char get_sys_state() ;

extern void set_sys_state(char bit, char state);



#endif /* STATE_STATE_H_ */
