/*
 * hwclock.h
 *
 *  Created on: Mar 11, 2014
 *      Author: lifeng
 */

#ifndef HWCLOCK_H_
#define HWCLOCK_H_

#include <time.h>

void get_time(struct tm *ptm) ;
void set_time(struct tm tm);
#endif /* HWCLOCK_H_ */
