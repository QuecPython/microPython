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

/**
 ******************************************************************************
 * @file    callbackdeal.c
 * @author  freddy.li
 * @version V1.0.0
 * @date    2021/11/25
 * @brief   xxx
 ******************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include "helios_os.h"
#include "stackctrl.h"
#include "py/runtime.h"
#include "callbackdeal.h"

#if MICROPY_ENABLE_CALLBACK_DEAL

#define QPY_CALLBACK_DEAL_MSG_MAX_NUM (2 * MICROPY_SCHEDULER_DEPTH)
#define QPY_CALLBACK_DEAL_THREAD_STACK_SIZE (16 *1024)
static Helios_MsgQ_t qpy_callback_deal_queue = 0;
Helios_Thread_t qpy_callback_deal_task_ref = 0;

typedef struct _callback_deal_thread_entry_args_t {
    mp_obj_dict_t *dict_locals;
    mp_obj_dict_t *dict_globals;
} callback_deal_thread_entry_args_t;


/*
 * Create a task&queue that handles callback tasks exclusively
 */
void qpy_callback_deal_thread(void * args_in);
void qpy_callback_deal_init(void)
{
	qpy_callback_deal_queue = Helios_MsgQ_Create(QPY_CALLBACK_DEAL_MSG_MAX_NUM, sizeof(st_CallBack_Deal));
	// create thread
	callback_deal_thread_entry_args_t *th_args;
	th_args = m_new_obj(callback_deal_thread_entry_args_t);
	// pass our locals and globals into the new thread
    th_args->dict_locals = mp_locals_get();
    th_args->dict_globals = mp_globals_get();
    
    Helios_ThreadAttr ThreadAttr = {
        .name = "cb_deal",
        .stack_size = QPY_CALLBACK_DEAL_THREAD_STACK_SIZE,
        .priority = (MP_THREAD_PRIORITY - 1),
        .entry = (void*)qpy_callback_deal_thread,
        .argv = (void*)th_args
    };
    qpy_callback_deal_task_ref = Helios_Thread_Create(&ThreadAttr);
    //add thread to python_thread link list, the thread is automatically deleted after the VM restarts.
    mp_new_thread_add((uint32_t)qpy_callback_deal_task_ref, QPY_CALLBACK_DEAL_THREAD_STACK_SIZE);
}

/*
 * delete callback_deal task&queue
 */
void qpy_callback_deal_deinit(void)
{
	Helios_MsgQ_Delete(qpy_callback_deal_queue); qpy_callback_deal_queue = (Helios_MsgQ_t)0;
	//Just set it to null and the thread will be deleted automatically when the vm restarts
	qpy_callback_deal_task_ref = (Helios_Thread_t)0;
}


//Added by Freddy @20211124 发送消息至deal task的queue
void qpy_send_msg_to_callback_deal_thread(uint8_t id, void *msg)
{
    if((Helios_MsgQ_t)0 != qpy_callback_deal_queue)
    {
        st_CallBack_Deal cb_msg = {0};
        cb_msg.callback_type_id = id;
        cb_msg.msg = msg;
        Helios_MsgQ_Put(qpy_callback_deal_queue, (void*)(&cb_msg), sizeof(st_CallBack_Deal), HELIOS_NO_WAIT);
    }
}

/*
 * Resolve the crash issue when non-Python threads apply for GC memory and throw exceptions when memory application fails
 */
void qpy_callback_deal_thread(void * args_in)
{
    callback_deal_thread_entry_args_t *args = (callback_deal_thread_entry_args_t *)args_in;
    mp_state_thread_t ts;
    mp_thread_set_state(&ts);
    st_CallBack_Deal msg = {0};

    mp_stack_set_top(&ts + 1); // need to include ts in root-pointer scan
    mp_stack_set_limit(QPY_CALLBACK_DEAL_THREAD_STACK_SIZE);

    #if MICROPY_ENABLE_PYSTACK
    // TODO threading and pystack is not fully supported, for now just make a small stack
    mp_obj_t mini_pystack[128];
    mp_pystack_init(mini_pystack, &mini_pystack[128]);
    #endif
    
    // set locals and globals from the calling context
    mp_locals_set(args->dict_locals);
    mp_globals_set(args->dict_globals);
    m_del_obj(callback_deal_thread_entry_args_t, args);
    
    // signal that we are set up and running
    mp_thread_start();
    
    MP_THREAD_GIL_ENTER();
    
    while(1)
    {
        nlr_buf_t nlr;
        mp_obj_t arg = NULL;
        if (nlr_push(&nlr) == 0)
        {
            MP_THREAD_GIL_EXIT();
            //Wait for an underlying callback to process the message
    		int ret = Helios_MsgQ_Get(qpy_callback_deal_queue, (void*)&msg, sizeof(msg), HELIOS_WAIT_FOREVER);
    		MP_THREAD_GIL_ENTER();
            if(ret != 0)
    		{
    		    nlr_pop();
    			continue;
    		}
    		//Each is processed according to the callback ID
            switch(msg.callback_type_id)
            {
                case CALLBACK_TYPE_ID_EXTINT:
                {
                    st_CallBack_Extint *cb_msg = (st_CallBack_Extint *)msg.msg;
                    mp_obj_t extint_list[2] = {
				        mp_obj_new_int(cb_msg->pin_no),
				        mp_obj_new_int(cb_msg->edge),
			        };
			        arg = mp_obj_new_list(2, extint_list);
		            mp_sched_schedule_ex(&cb_msg->callback,  arg);
                }
                break;
                case CALLBACK_TYPE_ID_AUDIO_RECORD:
                {
                    st_CallBack_AudioRecord *cb_msg = (st_CallBack_AudioRecord *)msg.msg;
                    mp_obj_t audio_cb[3] = {
            			mp_obj_new_str(cb_msg->p_data,strlen(cb_msg->p_data)),
            			mp_obj_new_int(cb_msg->len),
            			mp_obj_new_int(cb_msg->res),
            		};
            		if(RECORE_TYPE_FILE == cb_msg->record_type && NULL != cb_msg->p_data) {
            		    free(cb_msg->p_data);
            		}
            		arg = mp_obj_new_list(3, audio_cb);
                    mp_sched_schedule_ex(&cb_msg->callback,  arg);
				#if defined(PLAT_Unisoc)
					if (RECORE_TYPE_STREAM == cb_msg->record_type)
					{
						Helios_msleep(10);
					}
				#endif
                }
                break;
                case CALLBACK_TYPE_ID_VOICECALL_RECORD:
                {
                    st_CallBack_AudioRecord *cb_msg = (st_CallBack_AudioRecord *)msg.msg;
                    mp_obj_t audio_cb[3] = {
            			mp_obj_new_str(cb_msg->p_data,strlen(cb_msg->p_data)),
            			mp_obj_new_int(cb_msg->len),
            			mp_obj_new_int(cb_msg->res),
            		};

            		arg = mp_obj_new_list(3, audio_cb);
                    mp_sched_schedule_ex(&cb_msg->callback,  arg);
                }
                break;
                case CALLBACK_TYPE_ID_NONE:
                default:
                break;
            }
            
            MICROPY_EVENT_POLL_HOOK

	        if(msg.msg) {
	            free(msg.msg);
	        }
            memset(&msg, 0, sizeof(st_CallBack_Deal));
    		nlr_pop();
		}
		else
		{
            mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
		}
    }
}

#endif
