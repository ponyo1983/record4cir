#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "battery.h"

#include "../state/state.h"
#include "../led/led.h"

#define AD_NAME	("/sys/devices/platform/at91_adc/chan0")

#define VREF	(3300)
#define AD_MAX	(1024)

int get_battery_volt() {

	int fd_ad = -1;
	char ad_val[32];
	int volt = -1;

	fd_ad = open(AD_NAME, O_RDONLY);
	if (fd_ad >= 0) {
		int size = read(fd_ad, ad_val, 32);

		if (size > 0) {
			int val = atoi(ad_val);
			volt = val * VREF / AD_MAX;
			if (volt < BATTERY_ALARM_VOLT) {
				set_sys_state(BIT6_BATTERY, STATE_BATTERY_FAIL);
				light_battery_alarm(1);
			} else {
				set_sys_state(BIT6_BATTERY, STATE_BATTERY_OK);
				light_battery_alarm(0);
			}
		}

		close(fd_ad);
	}

	return volt;
}
