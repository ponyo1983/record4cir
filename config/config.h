/*
 * config.h
 *
 *  Created on: Mar 22, 2014
 *      Author: lifeng
 */

#ifndef CONFIG_H_
#define CONFIG_H_

void load_config();

char * get_id();
int get_playback_volume();
int get_capture_volume();
float get_play_rate();
#endif /* CONFIG_H_ */
