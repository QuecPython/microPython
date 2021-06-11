/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2021, QUECTEL  
 * Copyright (c) Quectel Wireless Solution, Co., Ltd.All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __AUDIO_QUEUE_H__
#define __AUDIO_QUEUE_H__

#include <stdint.h>
#include "ql_api_osi.h"

#define QUEUE_SIZE 11
#define QUEUE_NUMS 5


typedef struct
{
	uint8_t audio_type; //1 - tts; 2 - audio file(mp3,amr)
	uint8_t priority;
	uint8_t breakin;
	int mode;
	char data[500];
}ADUIO_DATA_T;

typedef struct
{
	ADUIO_DATA_T audio_data[QUEUE_SIZE];
	uint8_t front;
	uint8_t rear;
}AUDIO_QUEUE_T;

typedef struct
{
	AUDIO_QUEUE_T audio_queue[QUEUE_NUMS];
	uint8_t cur_priority;
	uint8_t cur_breakin;
	uint8_t cur_type;
	uint8_t audio_state;  //1 - now playing; 2 - idle
	uint8_t inited;
	ql_mutex_t queue_mutex;
}AUDIO_T;

typedef enum
{
	AUDIO_TTS = 1,
	AUDIO_FILE,
}AUDIO_TYPE_E;

typedef enum
{
	AUDIO_PLAYING = 1,
	AUDIO_IDLE,
}AUDIO_STATE_E;

typedef enum 
{
    QL_TTS_EVT_INIT,
    QL_TTS_EVT_DEINIT,
    QL_TTS_EVT_PLAY_START,
    QL_TTS_EVT_PLAY_STOP,
    QL_TTS_EVT_PLAY_FINISH,
    QL_TTS_EVT_PLAY_FAILED,
} QL_TTS_ENVENT_E;

#if 0
typedef enum
{
	AUD_PLAYER_ERROR 	= -1,
	AUD_PLAYER_START 	= 0,
	AUD_PLAYER_PAUSE 	= 1,
	AUD_PLAYER_RECOVER	= 2,
	AUD_PLAYER_PLAYING	= 3,
	AUD_PLAYER_NODATA	= 4,
    AUD_PLAYER_LESSDATA	= 5,
    AUD_PLAYER_MOREDATA	= 6,
	AUD_PLAYER_FINISHED	= 7,
}enum_aud_player_state;
#endif

extern AUDIO_T audio;

int audio_queue_init(AUDIO_QUEUE_T *Q);
int audio_queue_is_empty(AUDIO_QUEUE_T *Q);
int audio_queue_is_full(AUDIO_QUEUE_T *Q);
void ql_audio_play_queue(void);

#endif