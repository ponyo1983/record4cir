
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "battery.h"

#define AD_NAME	("/sys/devices/platform/at91_adc/chan0")

#define VREF	(3300)
#define AD_MAX	(1024)
static int fd_ad=-1;
static char ad_val[32];




int get_battery_volt()
{
	if(fd_ad<0)
	{
		fd_ad=open(AD_NAME,O_RDONLY);
		if(fd_ad>=0)
		{
			read(fd_ad,ad_val,32);
			usleep(100000);
		}
	}
	if(fd_ad>=0)
	{
		int size=read(fd_ad,ad_val,32);
		if(size>0)
		{
			int val=atoi(ad_val);
			return val*VREF/AD_MAX;
		}
	}
	return -1;
}
