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
#include "helios_os.h"
#include "audio_queue.h"
#include "helios_audio.h"
#include "helios_debug.h"
#define HELIOS_AUDIO_QUEUE_LOG(fmt, ...) custom_log(audio_audio, fmt, ##__VA_ARGS__)

AUDIO_T audio = {0};

extern int audio_play_callback(char *ptr, size_t lens, Helios_EnumAudPlayerState state);


/*=============================================================================*/
/* FUNCTION: audio_queue_init                                                  */
/*=============================================================================*/
/*!@brief 		: queue init.
 * @param[in] 	: the pointer to queue
 * @param[out] 	: 
 * @return		:
 *        -  0--success
 *        - -1--error
 */
/*=============================================================================*/

int audio_queue_init(AUDIO_QUEUE_T *Q)
{
	if (Q == NULL)
	{
		return -1;
	}
	memset(Q, 0, sizeof(AUDIO_QUEUE_T));
	Q->front = Q->rear = QUEUE_SIZE - 1;
	return 0;
}

/*=============================================================================*/
/* FUNCTION: audio_queue_is_empty                                              */
/*=============================================================================*/
/*!@brief 		: Determine if the queue is empty.
 * @param[in] 	: the pointer to queue
 * @param[out] 	: 
 * @return		:
 *        -  0--the queue is not empty
 *        -  1--the queue is empty
 *        - -1--error
 */
/*=============================================================================*/
int audio_queue_is_empty(AUDIO_QUEUE_T *Q)
{
	if (Q == NULL)
	{
		return -1;
	}
	if (Q->front == Q->rear)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/*=============================================================================*/
/* FUNCTION: audio_queue_is_full                                               */
/*=============================================================================*/
/*!@brief 		: Determine if the queue is full.
 * @param[in] 	: the pointer to queue
 * @param[out] 	: 
 * @return		:
 *        -  0--the queue is not full
 *        -  1--the queue is full
 *        - -1--error
 */
/*=============================================================================*/

int audio_queue_is_full(AUDIO_QUEUE_T *Q)
{
	if (Q == NULL)
	{
		return -1;
	}
	if ((Q->rear + 1) % QUEUE_SIZE == Q->front)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/*=============================================================================*/
/* FUNCTION: ql_audio_play_queue                                               */
/*=============================================================================*/
/*!@brief 		: Fetch queue element to playing.
 * @param[in] 	: 
 * @param[out] 	: 
 * @return		:
 * @modify      : 
 				----------  ---------   ---------------------------------------------------
				2020/10/16  Jayceon.Fu	Added support for audio file playing 
 */
/*=============================================================================*/
void helios_audio_play_queue(void)
{
	int i = 0, ret = 0;
	HELIOS_AUDIO_QUEUE_LOG("---- enter queue. ----\r\n");
	for (i=QUEUE_NUMS-1; i>=0; i--)
	{
		if (!audio_queue_is_empty(&audio.audio_queue[i]))
		{
			Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
			uint8_t front = (audio.audio_queue[i].front + 1) % QUEUE_SIZE;
			uint8_t audio_type = audio.audio_queue[i].audio_data[front].audio_type;
			int   mode = audio.audio_queue[i].audio_data[front].mode;
			char *data = audio.audio_queue[i].audio_data[front].data;
			audio.cur_priority = audio.audio_queue[i].audio_data[front].priority;
			audio.cur_breakin  = audio.audio_queue[i].audio_data[front].breakin;
			audio.cur_type     = audio.audio_queue[i].audio_data[front].audio_type;
			audio.audio_queue[i].front = front;
			audio.total_nums--;
			uint8_t nums = audio.total_nums;
			audio.audio_state = AUDIO_PLAYING;
			Helios_Mutex_Unlock(audio.queue_mutex);
			
			HELIOS_AUDIO_QUEUE_LOG("###total nums : %d, pri=%d, data: %s\r\n", nums, audio.cur_priority, data);
			
			if (audio_type == AUDIO_TTS)
			{
			#if defined(PLAT_ASR)
				ret = Helios_TTS_Start(mode, data, strlen(data));
			#endif
			}
			else if (audio_type == AUDIO_FILE)
			{
				ret = Helios_Audio_FilePlayStart(data, HELIOS_AUDIO_PLAY_TYPE_LOCAL, audio_play_callback);
				HELIOS_AUDIO_QUEUE_LOG("Helios_Audio_FilePlayStart, ret = %d\r\n", ret);
			}

			if (ret == -1)
			{
				Helios_Mutex_Lock(audio.queue_mutex, HELIOS_WAIT_FOREVER);
				audio.audio_state = AUDIO_IDLE;
				Helios_Mutex_Unlock(audio.queue_mutex);
				i=QUEUE_NUMS;
				continue;
			}
			else
			{
				break;
			}
		}
		else
		{
			audio_queue_init(&audio.audio_queue[i]);
		}
	}
}

void helios_audio_queue_play_task(void *argv)
{
	HELIOS_AUDIO_QUEUE_LOG("helios_audio_queue_play_task start...\r\n");
	uint8_t audio_msg = 0;
	int ret = 0;
	while (1)
	{
		HELIOS_AUDIO_QUEUE_LOG("wait audio play msg...\r\n");
		ret = Helios_MsgQ_Get(audio.queue_msg, (void *)&audio_msg, sizeof(uint8_t), HELIOS_WAIT_FOREVER);
		HELIOS_AUDIO_QUEUE_LOG("wait msg ok, msg=%d...\r\n", audio_msg);
		if (ret == 0)
		{
			switch (audio_msg)
			{
				case AUDIO_START_EVENT:
					break;
				case AUDIO_STOP_EVENT:
					HELIOS_AUDIO_QUEUE_LOG("----AUDIO_STOP_EVENT---\r\n");
					break;
				case AUDIO_FINISH_EVENT:
					HELIOS_AUDIO_QUEUE_LOG("----AUDIO_FINISH_EVENT---\r\n");
					//HELIOS_AUDIO_QUEUE_LOG("start queue paly...\r\n");
					helios_audio_play_queue();
					break;
				default:
					break;
			}
		}
	}
}


