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

#include "audio_queue.h"
#include "qpy_audio.h"

AUDIO_T audio = {0};

extern int qpy_audio_playFileStart(char *file_name, QPY_AUD_PLAYER_TYPE type,qpy_cb_on_player usr_cb);
extern int qpy_audio_playFileStop();
extern int audio_play_callback(char *p_data, int len, QPY_AUD_PLAYER_STATE state);


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
void ql_audio_play_queue(void)
{
	int i = 0, ret = 0;
	
	for (i=QUEUE_NUMS-1; i>=0; i--)
	{
		if (!audio_queue_is_empty(&audio.audio_queue[i]))
		{
			ql_rtos_mutex_lock(audio.queue_mutex, QL_WAIT_FOREVER);
			uint8_t front = (audio.audio_queue[i].front + 1) % QUEUE_SIZE;
			uint8_t audio_type = audio.audio_queue[i].audio_data[front].audio_type;
			//int   mode = audio.audio_queue[i].audio_data[front].mode;
			//char *data = audio.audio_queue[i].audio_data[front].data;
			audio.cur_priority = audio.audio_queue[i].audio_data[front].priority;
			audio.cur_breakin  = audio.audio_queue[i].audio_data[front].breakin;
			audio.cur_type     = audio.audio_queue[i].audio_data[front].audio_type;
			audio.audio_queue[i].front = front;
			ql_rtos_mutex_unlock(audio.queue_mutex);
		
			if (audio_type == AUDIO_TTS)
			{
				//ret = ql_tts_play(mode, audio.audio_queue[i].audio_data[front].data);
			}
			else if (audio_type == AUDIO_FILE)
			{
				//ret = ql_audio_file_play(audio.audio_queue[i].audio_data[front].data);
				QPY_AUD_PLAYER_TYPE type = 1;
				ret = qpy_audio_playFileStart(audio.audio_queue[i].audio_data[front].data, type, audio_play_callback);
			}

			if (ret == -1)
			{
				audio.audio_state = AUDIO_IDLE;
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



