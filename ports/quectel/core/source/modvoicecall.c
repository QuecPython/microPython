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
 @file	modvoicecall.h
*/
/**************************************************************************
===========================================================================
Copyright (c) 2021 Quectel Wireless Solution, Co., Ltd.All Rights Reserved.
Quectel Wireless Solution Proprietary and Confidential.
===========================================================================

						EDIT HISTORY FOR FILE
This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

WHEN		WHO			WHAT,WHERE,WHY
----------  ---------   ---------------------------------------------------
2021/07/20  Mia.zhong	Create.
**************************************************************************/

#include "obj.h"
#include "runtime.h"
#include "mphalport.h"
#include "helios_debug.h"
#include "helios_voicecall.h"


#define QPY_MODSMS_LOG(msg, ...)      custom_log("VOICECALL", msg, ##__VA_ARGS__)

//auto answer
STATIC mp_obj_t qpy_voicecall_auto_answer(mp_obj_t mp_obj_seconds)
{
	int seconds = mp_obj_get_int(mp_obj_seconds);

    if (seconds < 0 || seconds > 255)
    {
        mp_raise_ValueError("invalid value, seconds should be in [0-255].");
    }
    
	int ret = Helios_VoiceCall_Auto_Answer(0, seconds);

	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_voice_auto_answer_obj, qpy_voicecall_auto_answer);

//start call
STATIC mp_obj_t qpy_voicecall_start(mp_obj_t mp_obj_phone_num)
{
	char *phonenum = (char *)mp_obj_str_get_str(mp_obj_phone_num);

	int ret = Helios_VoiceCall_start(0, phonenum);

	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_voice_call_start_obj, qpy_voicecall_start);

//answer call
STATIC mp_obj_t qpy_voicecall_answer(void)
{
	int ret = Helios_VoiceCall_Answer(0);

	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_voice_call_answer_obj, qpy_voicecall_answer);

//end call
STATIC mp_obj_t qpy_voicecall_end(void)
{
	int ret = Helios_VoiceCall_End(0);

	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_voice_call_end_obj, qpy_voicecall_end);

STATIC mp_obj_t qpy_voicecall_start_dtmf(mp_obj_t mp_obj_dtmf, mp_obj_t mp_obj_duration)
{
    int ret = 0;
    char *dtmf = (char *)mp_obj_str_get_str(mp_obj_dtmf);
    int duration = mp_obj_get_int(mp_obj_duration);

    if (duration > 1000 || duration < 100)
    {
        mp_raise_ValueError("invalid value.duration shuould be in [100,1000]");
    }
    
    ret = Helios_VoiceCall_Start_Dtmf(0, dtmf, duration);
    
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_voice_call_start_dtmf_obj, qpy_voicecall_start_dtmf);

//fw set
STATIC mp_obj_t qpy_voicecall_fw_set(mp_obj_t mp_obj_reason, mp_obj_t mp_obj_fwmode, mp_obj_t mp_obj_phone_num)
{
    int reason = mp_obj_get_int(mp_obj_reason);
    int fwmode = mp_obj_get_int(mp_obj_fwmode);
	char *phonenum = (char *)mp_obj_str_get_str(mp_obj_phone_num);
	
	int ret = Helios_VoiceCall_Set_Fw(0, reason, fwmode, phonenum);

	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(qpy_voice_call_fw_set_obj, qpy_voicecall_fw_set);

static mp_obj_t g_voice_call_callback;

static void ql_voice_call_EventHandler(uint8_t sim_id, int32_t event_id, void *ctx)
{
    printf("voicecall event_id=%ld\n",event_id);
    
	switch(event_id)
	{
		case HELIOS_VC_RING_IND:
        {      
            Helios_call_incoming *vc_info = (Helios_call_incoming *)ctx;
            if (ctx)
            {
                char phone_num[20] = {0};
                memcpy(phone_num, vc_info->phone_num, strlen(vc_info->phone_num));
                
        		mp_obj_t tuple[3] =
                {
                    mp_obj_new_int(event_id),
                    mp_obj_new_int(vc_info->CallId),
                    mp_obj_new_str(phone_num,strlen(phone_num))
                };
        		if (g_voice_call_callback)
        		{
        			mp_sched_schedule(g_voice_call_callback, mp_obj_new_tuple(3, tuple));
        		}
            }
        }
		break;
            
		case HELIOS_VC_CONNECT_IND:
        {      
            Helios_call_connect *vc_info = (Helios_call_connect *)ctx;
			if (ctx)
            {
                char phone_num[20] = {0};
                memcpy(phone_num, vc_info->phone_num, strlen(vc_info->phone_num));
                
        		mp_obj_t tuple[3] =
                {
                    mp_obj_new_int(event_id),
                    mp_obj_new_int(vc_info->CallId),
                    mp_obj_new_str(phone_num,strlen(phone_num))
                };
        		if (g_voice_call_callback)
        		{
        			mp_sched_schedule(g_voice_call_callback, mp_obj_new_tuple(3, tuple));
        		}
            }
        }
	    break;
            
		case HELIOS_VC_NOCARRIER_IND: //no carrier reason
        {      
            Helios_call_disconnect *vc_info = (Helios_call_disconnect *)ctx;
            if (ctx)
            {
        		mp_obj_t tuple[3] =
                {
                    mp_obj_new_int(event_id),
                    mp_obj_new_int(vc_info->CallId),
                    mp_obj_new_int(vc_info->cause)
                };
        		if (g_voice_call_callback)
        		{
        			mp_sched_schedule(g_voice_call_callback, mp_obj_new_tuple(3, tuple));
        		}
            }
        }
		break;
            
        case HELIOS_VC_CCWA_IND:
        {
            Helios_call_waiting *vc_info = (Helios_call_waiting *)ctx;
            if (ctx)
            {
                char phone_num[40] = {0};
                memcpy(phone_num, vc_info->phone_num, strlen(vc_info->phone_num));
                mp_obj_t tuple[5] =
                {
                    mp_obj_new_int(event_id),
                    mp_obj_new_int(vc_info->CallId),
                    mp_obj_new_str(vc_info->phone_num, strlen(vc_info->phone_num)),
                    mp_obj_new_int(vc_info->num_type),
                    mp_obj_new_int(vc_info->CliValidity)
                };
        		if (g_voice_call_callback)
        		{
        			mp_sched_schedule(g_voice_call_callback, mp_obj_new_tuple(5, tuple));
        		}
            }
        }
        break;

        case HELIOS_VC_RING_VOLTE_IND:
	    case HELIOS_VC_CONNECT_VOLTE_IND:
	    case HELIOS_VC_NOCARRIER_VOLTE_IND:
	    case HELIOS_VC_CCWA_VOLTE_IND:
        {
            Helios_call_volte *vc_info = (Helios_call_volte *)ctx;
            if (ctx)
            {
                char phone_num[40] = {0};
                memcpy(phone_num, vc_info->phone_num, strlen(vc_info->phone_num));
                mp_obj_t tuple[9] =
                {
                    mp_obj_new_int(event_id),
                    mp_obj_new_int(vc_info->id),
                    mp_obj_new_int(vc_info->dir),
                    mp_obj_new_int(vc_info->status),
                    mp_obj_new_int(vc_info->type),
                    mp_obj_new_int(vc_info->mpty),
                    mp_obj_new_str(vc_info->phone_num, strlen(vc_info->phone_num)),
                    mp_obj_new_int(vc_info->num_type),
                    mp_obj_new_int(vc_info->release_direction),
                };
        		if (g_voice_call_callback)
        		{
        			mp_sched_schedule(g_voice_call_callback, mp_obj_new_tuple(9, tuple));
        		}
            }
        }
        break;
            
		default:
			break;
	}
}

STATIC mp_obj_t qpy_voicecall_add_event_handler(mp_obj_t handler)
{
    Helios_VoiceCallInitStruct info = {0};
	
	info.user_cb = ql_voice_call_EventHandler;
	g_voice_call_callback = handler;
	Helios_VoiceCall_Register(&info);
	
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_voicecall_add_event_handler_obj, qpy_voicecall_add_event_handler);

STATIC const mp_rom_map_elem_t mp_module_voicecall_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),           MP_ROM_QSTR(MP_QSTR_voiceCall) },
	{ MP_ROM_QSTR(MP_QSTR_setAutoAnswer),         MP_ROM_PTR(&qpy_voice_auto_answer_obj) },
    { MP_ROM_QSTR(MP_QSTR_callStart),      MP_ROM_PTR(&qpy_voice_call_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_callAnswer),       MP_ROM_PTR(&qpy_voice_call_answer_obj) },
    { MP_ROM_QSTR(MP_QSTR_callEnd),       MP_ROM_PTR(&qpy_voice_call_end_obj) },
    { MP_ROM_QSTR(MP_QSTR_startDtmf),       MP_ROM_PTR(&qpy_voice_call_start_dtmf_obj) },
    { MP_ROM_QSTR(MP_QSTR_setFw),         MP_ROM_PTR(&qpy_voice_call_fw_set_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setCallback),        MP_ROM_PTR(&qpy_voicecall_add_event_handler_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_voicecall_globals, mp_module_voicecall_globals_table);


const mp_obj_module_t mp_module_voicecall = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_voicecall_globals,
};





