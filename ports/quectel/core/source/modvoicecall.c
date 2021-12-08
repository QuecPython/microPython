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
#include "helios_audio.h"
#include "helios_ringbuf.h"
#include "callbackdeal.h"

#define QPY_MODVOICECALL_LOG(msg, ...)      custom_log("VOICECALL", msg, ##__VA_ARGS__)

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


STATIC mp_obj_t qpy_voicecall_set_channel(mp_obj_t device)
{
	int channel = mp_obj_get_int(device);

	if ((channel < 0) || (channel > 2)) 
	{
		mp_raise_ValueError("invalid device index, the value of device should be in[0,2]");
	}

	if (!audio_channel_check(channel))
	{
		mp_raise_msg_varg(&mp_type_ValueError, "The current platform does not support device %d.", channel);
	}

	int ret = Helios_Audio_SetAudioChannle(channel);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_voicecall_set_channel_obj, qpy_voicecall_set_channel);


STATIC mp_obj_t qpy_voicecall_set_volume(mp_obj_t volume)
{
	int vol = mp_obj_get_int(volume);
	if (vol < 0 || vol > 11)
	{
		mp_raise_ValueError("invalid value, the value of volume should be array between [0,11]");
		return mp_obj_new_int(-1);
	}
	
	Helios_AudPlayerType type = 2;
	int ret = Helios_Audio_SetVolume(type, vol);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_voicecall_set_volume_obj, qpy_voicecall_set_volume);


STATIC mp_obj_t qpy_voicecall_get_volume(void)
{
	Helios_AudPlayerType type = 2;
	int vol = Helios_Audio_GetVolume(type);
	return mp_obj_new_int(vol);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_voicecall_get_volume_obj, qpy_voicecall_get_volume);


#if defined(PLAT_ASR) || defined(PLAT_Unisoc)
STATIC mp_obj_t qpy_voice_call_set_auto_record(size_t n_args, const mp_obj_t *args)
{
    int enable = mp_obj_get_int(args[0]);
    int record_type = mp_obj_get_int(args[1]);
    int record_mode = mp_obj_get_int(args[2]);
	char *filename = (char *)mp_obj_str_get_str(args[3]);

    if ((enable != 0) && (enable != 1))
        mp_raise_ValueError("invalid value.<enable> should be [0 or 1]");
    if (record_type < 0 || record_type > 1)
        mp_raise_ValueError("invalid value.<record_type> should be [0 or 1]");
    if (record_mode < 0 || record_mode > 2)
        mp_raise_ValueError("invalid value.<record_mode> should be [0 ~ 2]");
    if (filename == NULL)
        mp_raise_ValueError("invalid value.<filename> must not be NULL");
    
    Helios_Voicecall_Record_Process_t param = {0};
    param.record_type = record_type;
    param.record_mode = record_mode;
    strncpy(param.filename, filename, strlen(filename));
	HELIOS_VC_ERROR_CODE ret = Helios_Voicecall_Set_Auto_Record(enable, &param);
	if (ret == HELIOS_VC_SUCCESS)
        return mp_obj_new_int(0);
    else if (ret == HELIOS_VC_FAILURE)
        return mp_obj_new_int(-1);
    else if (ret == HELIOS_VC_NOT_SUPPORT)
        return mp_obj_new_str("NOT SUPPORT",strlen("NOT SUPPORT"));
    else
        return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_voice_call_set_auto_record_obj, 4, 4, qpy_voice_call_set_auto_record);

STATIC mp_obj_t qpy_voice_call_start_record(mp_obj_t mp_obj_recordtype, mp_obj_t mp_obj_recordmode, mp_obj_t mp_obj_filename)
{
    int record_type = mp_obj_get_int(mp_obj_recordtype);
    int record_mode = mp_obj_get_int(mp_obj_recordmode);
	char *filename = (char *)mp_obj_str_get_str(mp_obj_filename);

    if (record_type < 0 || record_type > 1)
        mp_raise_ValueError("invalid value.<record_type> should be [0 or 1]");
    if (record_mode < 0 || record_mode > 2)
        mp_raise_ValueError("invalid value.<record_mode> should be [0 ~ 2]");
    if (filename == NULL)
        mp_raise_ValueError("invalid value.<filename> must not be NULL");
    
    Helios_Voicecall_Record_Process_t param = {0};
    param.record_type = record_type;
    param.record_mode = record_mode;
    strncpy(param.filename, filename, strlen(filename));
    
	HELIOS_VC_ERROR_CODE ret = Helios_VoiceCall_Start_Record(&param);
    if (ret == HELIOS_VC_SUCCESS)
        return mp_obj_new_int(0);
    else if (ret == HELIOS_VC_FAILURE)
        return mp_obj_new_int(-1);
    else if (ret == HELIOS_VC_NOT_SUPPORT)
        return mp_obj_new_str("NOT SUPPORT",strlen("NOT SUPPORT"));
    else
        return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(qpy_voice_call_start_record_obj, qpy_voice_call_start_record);

STATIC mp_obj_t qpy_voice_call_stop_record()
{   
	HELIOS_VC_ERROR_CODE ret = Helios_VoiceCall_Stop_Record();
	if (ret == HELIOS_VC_SUCCESS)
        return mp_obj_new_int(0);
    else if (ret == HELIOS_VC_FAILURE)
        return mp_obj_new_int(-1);
    else if (ret == HELIOS_VC_NOT_SUPPORT)
        return mp_obj_new_str("NOT SUPPORT",strlen("NOT SUPPORT"));
    else
        return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_voice_call_stop_record_obj, qpy_voice_call_stop_record);
#endif

static c_callback_t *g_record_callback = NULL;

#if defined(PLAT_ASR)// || defined(PLAT_Unisoc)
helios_ring_buf_t *VC_stream_ring_buf = NULL;
#define RECORD_BUFFER_MAX   (uint)10*1024

static int helios_stream_record_cb(char *p_data, int len, HELIOS_VC_AUD_REC_STATE res)
{
	/*if(res == HELIOS_VC_AUD_REC_START || res == HELIOS_VC_AUD_REC_FINISHED || res == HELIOS_VC_AUD_REC_DATA) {
		
		if(g_record_callback == NULL) {
			return -1;
		}
        
		mp_obj_t audio_cb[3] = {
			mp_obj_new_str(p_data,strlen(p_data)),
			mp_obj_new_int(len),
			mp_obj_new_int(res),
		};

		mp_sched_schedule_ex(g_record_callback, MP_OBJ_FROM_PTR(mp_obj_new_list(3, audio_cb)));
	}

	return 0;*/
	unsigned int send_len = 0;
	static int pcm_data_size = 0;

	
	if(res == HELIOS_VC_AUD_REC_START)
	{
		//audio_record_printf("recorder start run");
		pcm_data_size = 0;
	}
	else if(res == HELIOS_VC_AUD_REC_FINISHED)
	{
		//audio_record_printf("recorder stop run");		
		
	}
	else if(res == HELIOS_VC_AUD_REC_DATA)
	{
		if(len <= 0 || VC_stream_ring_buf == NULL)
			return -1;

		send_len = helios_rb_write(VC_stream_ring_buf, (unsigned char*)p_data, len);

		pcm_data_size += send_len;
	}

	if(res == HELIOS_VC_AUD_REC_START || res == HELIOS_VC_AUD_REC_FINISHED || res == HELIOS_VC_AUD_REC_DATA) {
		
		if(g_record_callback == NULL) {
			return -1;
		}
		
		if(res == HELIOS_VC_AUD_REC_FINISHED) send_len = pcm_data_size;
    #if MICROPY_ENABLE_CALLBACK_DEAL
		st_CallBack_AudioRecord *record = malloc(sizeof(st_CallBack_AudioRecord));
	    if(NULL != record) {
	        record->record_type = RECORE_TYPE_STREAM;
    	    record->p_data = "stream";
    	    record->len = send_len;
    	    record->res = res;
    	    record->callback = *g_record_callback;
    	    qpy_send_msg_to_callback_deal_thread(CALLBACK_TYPE_ID_VOICECALL_RECORD, record);
    	}
    #else
        char* p_data_buf = "stream";
		mp_obj_t audio_cb[3] = {
			mp_obj_new_str(p_data_buf,strlen(p_data_buf)),
			mp_obj_new_int(send_len),
			mp_obj_new_int(res),
		};

		mp_sched_schedule_ex(g_record_callback, MP_OBJ_FROM_PTR(mp_obj_new_list(3, audio_cb)));
    #endif
	}

	return 0;
}

STATIC mp_obj_t qpy_voice_call_start_record_stream(mp_obj_t mp_obj_recordtype, mp_obj_t mp_obj_recordmode, mp_obj_t mp_obj_cb)
{
    int record_type = mp_obj_get_int(mp_obj_recordtype);
    int record_mode = mp_obj_get_int(mp_obj_recordmode);

    if (record_type < 0 || record_type > 1)
        mp_raise_ValueError("invalid value.<record_type> should be [0 or 1]");
    if (record_mode < 0 || record_mode > 2)
        mp_raise_ValueError("invalid value.<record_mode> should be [0 ~ 2]");
    
    Helios_Voicecall_Record_Process_t param = {0};
    param.record_type = record_type;
    param.record_mode = record_mode;

	static c_callback_t cb = {0};
	memset(&cb, 0, sizeof(c_callback_t));
	g_record_callback = &cb;
	mp_sched_schedule_callback_register(g_record_callback, mp_obj_cb);

    if(VC_stream_ring_buf == NULL) {
		VC_stream_ring_buf = helios_rb_create(RECORD_BUFFER_MAX);
	}
    
	HELIOS_VC_ERROR_CODE ret = Helios_VoiceCall_Start_Record_Stream(&param, (helios_vc_cb_on_record)helios_stream_record_cb);
    if (ret == HELIOS_VC_SUCCESS)
        return mp_obj_new_int(0);
    else if (ret == HELIOS_VC_FAILURE)
        return mp_obj_new_int(-1);
    else if (ret == HELIOS_VC_NOT_SUPPORT)
        return mp_obj_new_str("NOT SUPPORT",strlen("NOT SUPPORT"));
    else
        return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(qpy_voice_call_start_record_stream_obj, qpy_voice_call_start_record_stream);

STATIC mp_obj_t qpy_voice_call_record_stream_read(mp_obj_t read_buf, mp_obj_t len)
{
	int read_actual_len = 0;
	//audio_record_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(read_buf, &bufinfo, MP_BUFFER_WRITE);
	
	int read_len = mp_obj_get_int(len);
	if(read_len > 0 && VC_stream_ring_buf) {
		read_actual_len = helios_rb_read(VC_stream_ring_buf, bufinfo.buf, read_len);
		return mp_obj_new_int(read_actual_len);
	}
	return mp_obj_new_int(-1);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_voice_call_record_stream_read_obj, qpy_voice_call_record_stream_read);
#endif

static c_callback_t *g_voice_call_callback = NULL;

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
        			mp_sched_schedule_ex(g_voice_call_callback, mp_obj_new_tuple(3, tuple));
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
        			mp_sched_schedule_ex(g_voice_call_callback, mp_obj_new_tuple(3, tuple));
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
        			mp_sched_schedule_ex(g_voice_call_callback, mp_obj_new_tuple(3, tuple));
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
        			mp_sched_schedule_ex(g_voice_call_callback, mp_obj_new_tuple(5, tuple));
        		}
            }
        }
        break;
        
        case HELIOS_VC_DIALING_IND:
        {
    		if (g_voice_call_callback)
    		{
    			mp_sched_schedule_ex(g_voice_call_callback, mp_obj_new_int(event_id));
    		}
        }
        break;

	    case HELIOS_VC_MO_FAILED_IND:
        {
            Helios_call_mo_failed *vc_info = (Helios_call_mo_failed *)ctx;
            if (ctx)
            {
                mp_obj_t tuple[4] =
                {
                    mp_obj_new_int(event_id),
                    mp_obj_new_int(vc_info->CallId),
                    mp_obj_new_int(vc_info->Cause),
                    mp_obj_new_int(vc_info->InBandTones)
                };
        		if (g_voice_call_callback)
        		{
        			mp_sched_schedule_ex(g_voice_call_callback, mp_obj_new_tuple(4, tuple));
        		}
            }
        }
        break;

        case HELIOS_VC_HOLDING_IND:
        {
            Helios_call_holding *vc_info = (Helios_call_holding *)ctx;
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
        			mp_sched_schedule_ex(g_voice_call_callback, mp_obj_new_tuple(3, tuple));
        		}
            }
        }
        break;

        case HELIOS_VC_RING_VOLTE_IND:
	    case HELIOS_VC_CONNECT_VOLTE_IND:
	    case HELIOS_VC_NOCARRIER_VOLTE_IND:
	    case HELIOS_VC_CCWA_VOLTE_IND:
        case HELIOS_VC_DIALING_VOLTE_IND:
	    case HELIOS_VC_ALERTING_VOLTE_IND:
	    case HELIOS_VC_HOLDING_VOLTE_IND:
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
        			mp_sched_schedule_ex(g_voice_call_callback, mp_obj_new_tuple(9, tuple));
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
	static c_callback_t cb = {0};
	memset(&cb, 0, sizeof(c_callback_t));
	info.user_cb = ql_voice_call_EventHandler;
	//g_voice_call_callback = handler;
	
	g_voice_call_callback = &cb;
	mp_sched_schedule_callback_register(g_voice_call_callback, handler);
	Helios_VoiceCall_Register(&info);
	
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_voicecall_add_event_handler_obj, qpy_voicecall_add_event_handler);

STATIC mp_obj_t qpy_module_voicecall_deinit(void)
{
#if defined(PLAT_ASR) || defined(PLAT_Unisoc)
    Helios_VoiceCall_Stop_Record();
#endif
    Helios_VoiceCall_End(0);

	g_voice_call_callback = NULL;
    g_record_callback = NULL;
    
	Helios_VoiceCallInitStruct info = {0};
	info.user_cb = NULL;
	Helios_VoiceCall_Register(&info);
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_module_voicecall_deinit_obj, qpy_module_voicecall_deinit);


STATIC const mp_rom_map_elem_t mp_module_voicecall_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),                MP_ROM_QSTR(MP_QSTR_voiceCall) },
    { MP_ROM_QSTR(MP_QSTR___qpy_module_deinit__),   MP_ROM_PTR(&qpy_module_voicecall_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setAutoAnswer),           MP_ROM_PTR(&qpy_voice_auto_answer_obj) },
    { MP_ROM_QSTR(MP_QSTR_callStart),               MP_ROM_PTR(&qpy_voice_call_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_callAnswer),              MP_ROM_PTR(&qpy_voice_call_answer_obj) },
    { MP_ROM_QSTR(MP_QSTR_callEnd),                 MP_ROM_PTR(&qpy_voice_call_end_obj) },
    { MP_ROM_QSTR(MP_QSTR_startDtmf),               MP_ROM_PTR(&qpy_voice_call_start_dtmf_obj) },
    { MP_ROM_QSTR(MP_QSTR_setFw),                   MP_ROM_PTR(&qpy_voice_call_fw_set_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setCallback),             MP_ROM_PTR(&qpy_voicecall_add_event_handler_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setChannel),              MP_ROM_PTR(&qpy_voicecall_set_channel_obj) },
	{ MP_ROM_QSTR(MP_QSTR_setVolume), 				MP_ROM_PTR(&qpy_voicecall_set_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_getVolume), 				MP_ROM_PTR(&qpy_voicecall_get_volume_obj) },
#if defined(PLAT_ASR) || defined(PLAT_Unisoc)
	{ MP_ROM_QSTR(MP_QSTR_setAutoRecord),           MP_ROM_PTR(&qpy_voice_call_set_auto_record_obj) },
	{ MP_ROM_QSTR(MP_QSTR_startRecord),             MP_ROM_PTR(&qpy_voice_call_start_record_obj) },
	{ MP_ROM_QSTR(MP_QSTR_stopRecord),              MP_ROM_PTR(&qpy_voice_call_stop_record_obj) },
#endif
#if defined(PLAT_ASR)// || defined(PLAT_Unisoc)
	{ MP_ROM_QSTR(MP_QSTR_startRecordStream),       MP_ROM_PTR(&qpy_voice_call_start_record_stream_obj) },
	{ MP_ROM_QSTR(MP_QSTR_readRecordStream),       MP_ROM_PTR(&qpy_voice_call_record_stream_read_obj) },
#endif

};
STATIC MP_DEFINE_CONST_DICT(mp_module_voicecall_globals, mp_module_voicecall_globals_table);


const mp_obj_module_t mp_module_voicecall = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_voicecall_globals,
};





