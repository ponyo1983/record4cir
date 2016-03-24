/*
 * sound.c
 *
 *  Created on: Mar 20, 2014
 *      Author: lifeng
 */

#include <stdlib.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <linux/input.h>
#include <fcntl.h>

#include <speex/speex.h>

#include "sound.h"

#include "../lib/block_manager.h"
#include "../lib/block_filter.h"
#include "../lib/block.h"

#include "../storage/record_manager.h"

#include "../storage/record.h"

#include "../config/config.h"

#include "../serial/frame_manager.h"

#include "../global.h"

#include "g726.h"

#define DEFAULT_FORMAT		SND_PCM_FORMAT_S16_LE
#define DEFAULT_SPEED 		8000
#define SOUND_NAME	("default")

#define FRAME_SIZE	(160)

#define S16_MAX_VALUE	(32767)
#define S16_MIN_VALUE	(-32768)

#ifdef __x86_64
#define INPUT_NAME	("/dev/input/event7")
#define KEY_CODE	KEY_ESC

const char* selem_name[2] = { "Master", "Capture" };
#else
#define INPUT_NAME	("/dev/input/event0")
#define KEY_CODE	(257)
const char* selem_name[2] = {"Digital", "Line Input"};
#endif

static struct sound* pgsound = NULL;

void start_capture() {
	if (pgsound != NULL) {
		pgsound->state = SOUND_CAPTURE;
	}
}

void stop_capture() {
	if (pgsound != NULL) {
		pgsound->state = SOUND_IDLE;
	}
}

void start_play(int port) {
	if (pgsound != NULL) {
		pgsound->state = SOUND_PLAY;
		pgsound->request_port = port;
	}
}

void stop_play() {
	if (pgsound != NULL) {
		pgsound->state = SOUND_IDLE;
	}
}

/*
 * The parameter volume is to be given in the range [0, 100]
 */
void set_volume(long volume, int dir) {
	long min, max;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	dir = dir > 0 ? 1 : 0;
	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, card);
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name[dir]);
	snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

	if (dir == 0) {
		snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
		snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);
	} else {
		snd_mixer_selem_get_capture_volume_range(elem, &min, &max);
		snd_mixer_selem_set_capture_volume_all(elem, volume * max / 100);
	}

	snd_mixer_close(handle);
}

static ssize_t pcm_read(struct sound* psound, u_char *data, size_t rcount) {
	ssize_t r;
	size_t result = 0;
	size_t count = rcount;
	snd_pcm_t * handle = psound->handle;
	if (count > psound->chunk_size) {
		count = psound->chunk_size;
	}

	while (count > 0) {

		r = snd_pcm_readi(handle, data, count);

		if (r == -EAGAIN || (r >= 0 && (size_t) r < count)) {

			snd_pcm_wait(handle, 100);
		} else if (r == -EPIPE) {
			//xrun();
		} else if (r == -ESTRPIPE) {
			//suspend();
		} else if (r < 0) {
			//error(_("read error: %s"), snd_strerror(r));
			//prg_exit(EXIT_FAILURE);
		}
		if (r > 0) {

			result += r;
			count -= r;
			data += r * (psound->bits_per_frame) / 8;
		}
	}
	return rcount;
}

static void set_params(struct sound *psound) {
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_uframes_t buffer_size;
	int err;

	unsigned int rate;
	unsigned buffer_time = 0;

	unsigned period_time = 0;
	snd_pcm_uframes_t period_frames = 0;
	snd_pcm_uframes_t buffer_frames = 0;
	int bits_per_sample, bits_per_frame;
	snd_pcm_uframes_t chunk_size = 0;
	snd_pcm_t *handle = psound->handle;
	int channels = 1;
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_sw_params_alloca(&swparams);
	err = snd_pcm_hw_params_any(handle, params);
	if (err < 0)
		return;

	err = snd_pcm_hw_params_set_access(handle, params,
			SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0)
		return;
	err = snd_pcm_hw_params_set_format(handle, params, DEFAULT_FORMAT);
	if (err < 0)
		return;

	err = snd_pcm_hw_params_set_channels(handle, params, channels);
	if (err < 0)
		return;

	rate = DEFAULT_SPEED;
	err = snd_pcm_hw_params_set_rate_near(handle, params, &rate, 0);
	assert(err >= 0);

	if (buffer_time == 0 && buffer_frames == 0) {
		err = snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, NULL);
		assert(err >= 0);
		if (buffer_time > 500000)
			buffer_time = 500000;
	}
	if (period_time == 0 && period_frames == 0) {
		if (buffer_time > 0)
			period_time = buffer_time / 4;
		else
			period_frames = buffer_frames / 4;
	}
	if (period_time > 0)
		err = snd_pcm_hw_params_set_period_time_near(handle, params,
				&period_time, NULL);
	else
		err = snd_pcm_hw_params_set_period_size_near(handle, params,
				&period_frames, 0);
	assert(err >= 0);
	if (buffer_time > 0) {
		err = snd_pcm_hw_params_set_buffer_time_near(handle, params,
				&buffer_time, 0);
	} else {
		err = snd_pcm_hw_params_set_buffer_size_near(handle, params,
				&buffer_frames);
	}
	assert(err >= 0);

	err = snd_pcm_hw_params(handle, params);
	if (err < 0) {
		return;
	}
	snd_pcm_hw_params_get_period_size(params, &chunk_size, NULL);
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
	psound->chunk_size = chunk_size;
	if (chunk_size == buffer_size) {
		return;
	}
	snd_pcm_sw_params_current(handle, swparams);

	if (snd_pcm_sw_params(handle, swparams) < 0) {
		return;
	}

	bits_per_sample = snd_pcm_format_physical_width(DEFAULT_FORMAT);
	bits_per_frame = bits_per_sample * channels;
	psound->bits_per_frame = bits_per_frame;
	int chunk_bytes = chunk_size * bits_per_frame / 8;
	if (psound->audiobuf == NULL) {
		psound->audiobuf = malloc(chunk_bytes);
	}
	//snd_pcm_hw_params_free(params);
	//snd_pcm_sw_params_free(swparams);
}

static void playback_loop(struct sound* psound, u_char *wav_buffer, int length) {

	int index = 0;
	int i;
	float tmp;
	int frame_size;
	float output[FRAME_SIZE];
	short out[FRAME_SIZE];
	int enhancement = 1;
	/*Holds the state of the decoder*/
	void *state;
	/*Holds bits so they can be read and written to by the Speex routines*/
	SpeexBits bits;

	float rate_vol=get_play_rate();
	/*Create a new decoder state in narrowband mode*/
	state = speex_decoder_init(&speex_nb_mode);

	/*Set the perceptual enhancement on*/
	speex_decoder_ctl(state, SPEEX_SET_ENH, &enhancement);

	/*Initialization of the structure that holds the bits*/
	speex_bits_init(&bits);

	index=0;
	while ((index < length) && (psound->state == SOUND_PLAY)) {

		frame_size = wav_buffer[index]+(wav_buffer[index+1]<<8);
		index += 2;
		if(frame_size!=38)
		{
			printf("leng!=38\n");
		}
		if (frame_size < 256 ) {

			/*Copy the data into the bit-stream struct*/
			speex_bits_read_from(&bits, (char*)(wav_buffer + index), frame_size);

			/*Decode the data*/
			speex_decode(state, &bits, output);

			/*Copy from float to short (16 bits) for output*/
			for (i = 0; i < FRAME_SIZE; i++) {

				tmp = output[i]*rate_vol; //在此更该放大倍数
				if (tmp > S16_MAX_VALUE) {
					out[i] = S16_MAX_VALUE;
				} else if (tmp < S16_MIN_VALUE) {
					out[i] = S16_MIN_VALUE;
				} else {
					out[i] = tmp;
				}
			}
			if (snd_pcm_writei(psound->handle, out, FRAME_SIZE) <= 0)
						break;

			index+=frame_size;

		} else {

		}
	}

	/*Destroy the decoder state*/
	speex_decoder_destroy(state);
	/*Destroy the bit-stream truct*/
	speex_bits_destroy(&bits);
	//snd_pcm_nonblock(psound->handle, 0);
	//snd_pcm_drain(psound->handle);

}

/*
 * 按照G726 rate=2 G726_encode()函数的格式要求存储数据
 */
void capture_loop(struct sound*psound) {

	short *audiobuf = (short*) psound->audiobuf;
	int *enc_num;
	struct block * pblock = NULL;
	int length = 0;
	int i;
	char cbits[256];
	int nbBytes;
	struct record_manager * record_manager = get_record_manager();

	float input[FRAME_SIZE];

	int quality = 8;
	/*Holds the state of the encoder*/
	void *state;
	/*Holds bits so they can be read and written to by the Speex routines*/
	SpeexBits bits;

	/*Create a new encoder state in narrowband mode*/
	state = speex_encoder_init(&speex_nb_mode);

	/*Set the quality to 8 (15 kbps)*/
	speex_encoder_ctl(state, SPEEX_SET_QUALITY, &quality);
	/*Initialization of the structure that holds the bits*/
	speex_bits_init(&bits);

	while (1) {

		if (pblock == NULL) {
			pblock = get_block(psound->manager.fliters, 0, BLOCK_EMPTY);
			if (pblock != NULL) {
				pblock->data_length = 0;
				init_wave_block(pblock); //???
				length = 16; //record_header
			}
			if (psound->state != SOUND_CAPTURE) {
				break;
			}
		}
		if (pcm_read(psound, (u_char*) audiobuf, FRAME_SIZE) != FRAME_SIZE) {
			printf("capture err\n");
			break;
		} else {
			if (pblock != NULL) {
				//开始Speex编码
				for (i = 0; i < FRAME_SIZE; i++) {
					input[i] = audiobuf[i];
				}
				/*Flush all the bits in the struct so we can encode a new frame*/
				speex_bits_reset(&bits);

				/*Encode the frame*/
				speex_encode(state, input, &bits);
				/*Copy the bits to an array of char that can be written*/
				nbBytes = speex_bits_write(&bits, cbits, 256);

				pblock->data[length]=(nbBytes&0xff);
				pblock->data[length+1]=((nbBytes>>8)&0xff);
				enc_num = (int*) (pblock->data + length);
				*enc_num = nbBytes;

				for (i = 0; i < nbBytes; i++) {
					pblock->data[length + 2 + i] = cbits[i];
				}

				length += (2 + nbBytes);
				pblock->data_length = pblock->data_length + 2 + nbBytes;
				if (psound->state != SOUND_CAPTURE) {
					printf("store:%d\n", length);
					set_wave_block_length(pblock, length - 16);
					store_wave_data(record_manager, pblock);
					pblock = NULL;
					break;
				}
				if (length + FRAME_SIZE * 2 > pblock->block_size) {
					set_wave_block_length(pblock, (length - 16));
					store_wave_data(record_manager, pblock);
					pblock = NULL;
					printf("OK:%d\n", length);
				}

			}

		}

	}

	/*Destroy the encoder state*/
	speex_encoder_destroy(state);
	/*Destroy the bit-packing struct*/
	speex_bits_destroy(&bits);
}

static void playback(struct sound * psound) {

	struct record_manager* record_manager = get_record_manager();
	int i, sound_num;
	struct block * pblock;
	struct block * bkblock;

	int err;
	struct record_header * header;
	int open_mode = 0;
	char buffer[3] = { 0, 0, 0 };

	for (sound_num = 0; sound_num < 5; sound_num++) {
		printf("play num:%d\n", sound_num);
		if (psound->state != SOUND_PLAY)
			break;

		pblock = get_block(psound->manager.fliters, 0, BLOCK_EMPTY);
		if (pblock != NULL) {
			request_playback(record_manager, sound_num, pblock);

			bkblock = get_block(psound->manager.fliters, 5000, BLOCK_FULL);
			if (bkblock != NULL) {

				header = (struct record_header*) bkblock->data;
				printf("start play wave size:%d\n", header->wave_size);
				if (header->wave_size > 0) {

					err = snd_pcm_open(&(psound->handle), SOUND_NAME,
							SND_PCM_STREAM_PLAYBACK, open_mode);
					if (err < 0)
						break;
					set_params(psound);
					err = snd_pcm_prepare(psound->handle);
					if (err < 0)
						break;
					playback_loop(psound, bkblock->data + 16,
							header->wave_size);

					snd_pcm_close(psound->handle);
				}
			}

			put_block(bkblock, BLOCK_EMPTY);
		} else {
			put_block(pblock, BLOCK_EMPTY);

		}

	}

	struct frame_manager *manager_control = get_frame_manager(CONTROL_MANAGER);
	send_frame(manager_control, RECORD_ADDRESS, psound->request_port, 1,
			(char) 0x92, buffer, 3);

}
static void captue(struct sound * psound) {
	int err;
	int open_mode = 0;
	err = snd_pcm_open(&(psound->handle), SOUND_NAME, SND_PCM_STREAM_CAPTURE,
			open_mode);
	if (err < 0)
		return;

	set_params(psound);
	capture_loop(psound);
	snd_pcm_close(psound->handle);
}

static void update_state(struct sound * psound) {
	if (psound->ppt == 1) {
		if (psound->state != SOUND_PLAY) {
			psound->state = SOUND_CAPTURE;
		}
	} else if (psound->ppt == 0) {
		if (psound->state != SOUND_PLAY) {
			psound->state = SOUND_IDLE;
		}
	}
}

void * sound_proc(void * args) {
	struct sound * psound = (struct sound*) args;
	int vol = get_playback_volume();
	set_volume(vol, 0);
	vol = get_capture_volume();
	set_volume(vol, 1);
	while (1) {
		usleep(100000);
		update_state(psound);
		if (psound->state == SOUND_CAPTURE) {
			captue(psound);
			psound->state = SOUND_IDLE;

		} else if (psound->state == SOUND_PLAY) {
			playback(psound);
			psound->state = SOUND_IDLE;
		}

	}
}

void * key_check(void *args) {
	struct sound * psound = (struct sound*) args;
	struct input_event key_event;
	int fd = open(INPUT_NAME, O_RDWR);
	if (fd > 0) {
		while (1) {
			if (read(fd, &key_event, sizeof(key_event)) == sizeof(key_event)) {
				if (key_event.type == EV_KEY) {

					if (key_event.code == KEY_CODE) {
						psound->ppt = key_event.value > 0 ? 1 : 0;
						update_state(psound);
					}

				}

			}

		}
		close(fd);
	}

	return NULL;
}

void start_sound() {
	if (pgsound == NULL) {
		pgsound = malloc(sizeof(struct sound));
		if (pgsound != NULL) {
			memset(pgsound, 0, sizeof(struct sound));
			pgsound->state = SOUND_IDLE;
			add_block_filter(&(pgsound->manager), NULL, CAPTURE_BUFFER_SIZE, 5);
			pthread_create(&(pgsound->thread), NULL, sound_proc, pgsound);
			pthread_create(&(pgsound->thread_key), NULL, key_check, pgsound);
		}
	}
}
