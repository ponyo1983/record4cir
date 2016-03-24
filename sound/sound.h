/*
 * sound.h
 *
 *  Created on: Mar 20, 2014
 *      Author: lifeng
 */

#ifndef SOUND_H_
#define SOUND_H_

#include <pthread.h>
#include <alsa/asoundlib.h>

#include "../lib/block_manager.h"

#define CAPTURE_BUFFER_SIZE	(1024*1024)

enum sound_state
{
	SOUND_IDLE=0,
	SOUND_CAPTURE=1,
	SOUND_PLAY=2,
};
struct sound
{
	struct block_manager manager;
	pthread_mutex_t mutex;
	pthread_t thread;
	pthread_t thread_key;
	int ppt;
	enum sound_state state;
	snd_pcm_t *handle;
	u_char *audiobuf;
	int chunk_size;
	int bits_per_frame;
	int request_port;

};



void start_sound();

void start_capture();
void stop_capture();


void start_play(int port);
void stop_play();



#endif /* SOUND_H_ */
