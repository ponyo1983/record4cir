/*
 * config.c
 *
 *  Created on: Mar 22, 2014
 *      Author: lifeng
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char record_id[16] = "PGV5";
static int volume_playback=84;
static int volume_capture=84;

static float play_rate=3;

static void str_cpy(char *src,char *dst,int length)
{
	int i;
	for(i=0;i<length;i++)
	{
		dst[i]=src[i];
		if(dst[i]=='\r' || dst[i]=='\n')
		{
			dst[i]='\0';
		}
	}
}

void load_config() {

	char buffer[128];
	char value[32];
	FILE *file = fopen("/Config", "r");

	if (file != NULL) {
		while (fgets(buffer, 128, file) != NULL) {

			if(strncmp(buffer,"ID=",3)==0)
			{
				str_cpy(buffer+3,record_id,16);

			}
			else if(strncmp(buffer,"PlayRate=",9)==0)
			{
				str_cpy(buffer+9,value,16);
				play_rate=atoi(value);
			}

		}

		fclose(file);
	}
}

char * get_id() {
	return record_id;
}
int get_playback_volume()
{
	return volume_playback;
}

int get_capture_volume()
{
	return volume_capture;
}

float get_play_rate()
{
	return play_rate;
}
