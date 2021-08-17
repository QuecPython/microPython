/*
 * Copyright (c) Quectel Wireless Solution, Co., Ltd.All Rights Reserved.
 *  
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *  
 *     http://www.apache.org/licenses/LICENSE-2.0
 *  
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __AUDIO_QUEUE_H__
#define __AUDIO_QUEUE_H__

#include <stdint.h>
#include <string.h>
#include "helios_os.h"

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
	uint8_t total_nums;
	uint8_t user_call_stop;
	Helios_Mutex_t queue_mutex;
	Helios_MsgQ_t queue_msg;
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
	AUDIO_ERROR_EVENT = -1,
    AUDIO_START_EVENT,
    AUDIO_FINISH_EVENT,
    AUDIO_STOP_EVENT,
}AUDIO_TTS_ENVENT_E;


extern AUDIO_T audio;

int audio_queue_init(AUDIO_QUEUE_T *Q);
int audio_queue_is_empty(AUDIO_QUEUE_T *Q);
int audio_queue_is_full(AUDIO_QUEUE_T *Q);
void helios_audio_play_queue(void);
void helios_audio_queue_play_task(void *argv);

#endif