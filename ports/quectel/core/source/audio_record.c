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

#include "py/binary.h"
#include "py/gc.h"
#include "py/misc.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/obj.h"
#include "py/objarray.h"
#include "py/qstr.h"
#include "py/runtime.h"

#include <stdio.h>
#include "stdlib.h"
#include "string.h"	
#include "linklist.h"
#include "Helios_audio.h"
#include "Helios_debug.h"
#include "helios_fs.h"
#include "helios_os.h"

#define audio_record_printf(fmt, ...) custom_log(audio_record, fmt, ##__VA_ARGS__)

static char record_state = 0;

const mp_obj_type_t audio_record_type;

typedef struct _audio_record_obj_t {
    mp_obj_base_t base;
	char file_name[128];
	mp_obj_t callback;
	int  isbusy;
	int record_timer;
	struct Node *filenameListHead;
	struct Node *filenameListEnd;
} audio_record_obj_t;

audio_record_obj_t *self_cur = NULL;



static mp_obj_t callback_cur;

extern int device;
Helios_OSTimer_t rec_timer = 0;

STATIC mp_obj_t audio_record_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) 
{
    mp_arg_check_num(n_args, n_kw, 0, 0, true);

	audio_record_obj_t *self = m_new_obj_with_finaliser(audio_record_obj_t);
	self->base.type = &audio_record_type;

	self->isbusy = 0;
	self->callback = NULL;
	self->filenameListHead = NULL;
	self->filenameListEnd = NULL;

	rec_timer = Helios_OSTimer_Create();
	if(rec_timer == 0)
	{
		return mp_obj_new_int(-1);
	}

	if(device == HELIOS_OUTPUT_RECEIVER||device == HELIOS_OUTPUT_SPEAKER||device == HELIOS_OUTPUT_HEADPHONE){
		Helios_Audio_SetAudioChannle(device);
	}
	else{
		device = HELIOS_OUTPUT_RECEIVER;			//default replay path
		Helios_Audio_SetAudioChannle(device);
	}

    return MP_OBJ_FROM_PTR(self);
}

static int helios_file_record_cb(char *p_data, int len, Helios_EnumAudRecordState res)
{
	audio_record_printf("[carola] entern helios_file_record_cb res = %d\n", res);
	audio_record_printf("helios_file_record_cb len %d\n",len);
	audio_record_printf("helios_file_record_cb p_data %s\n",p_data);
	return 0;
}


static void helios_record_timer_cb(void *argv) {
	audio_record_obj_t *self = (audio_record_obj_t *)argv;
	Helios_Audio_FileRecordStop();
	self->isbusy = 0;
	record_state = 0;
}


enum {
	FORMAT_WAV,
	FORMAT_AMR,
	FORMAT_MP3,
};


static int check_format(char *path) {
	if(path == NULL) return -1;
	
	if(strlen(path) < 1) {
		return -2;
	}
	
	char *st_old = NULL;
	char *st_now = NULL;
	
	int is_first = 1;
	while(1) {
		if(is_first == 1) {
			is_first = 0;
			st_now = strchr(path, '.');
		} else {
			st_now = strchr(st_now+1, '.');
		}
		if(st_now != NULL){
			st_old = st_now+1;
		} else {
			break;
		}
	}
	if(st_old == NULL) return FORMAT_AMR;
	
	if(strncmp(st_old, "wav", 3)==0){
		return FORMAT_WAV;
	}  else if(strncmp(st_old, "mp3", 3)==0) {
		return FORMAT_MP3;
	} else {
		return FORMAT_AMR;
	}
}


STATIC mp_obj_t helios_audio_record_start(mp_obj_t self_in, mp_obj_t path, mp_obj_t time_out)
{
	int time = 0;
	int ret = -1;
	int format = 0;
	audio_record_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->isbusy == 1 || record_state == 1) {
		return mp_obj_new_int(-3);
	}

	memset(self->file_name, 0, 128);
	char *file_name = (char*)mp_obj_str_get_str(path);
	audio_record_printf("char *file_name = %s\r\n",file_name);
#ifdef PLAT_ASR
	sprintf(self->file_name, "U:/%s",file_name);
#endif
#ifdef PLAT_Unisoc
	sprintf(self->file_name, "UFS:%s",file_name);
#endif

#if 0
	int channel = mp_obj_get_int(args[2]);
	if(channel != 1 && channel != 2) {
		return mp_obj_new_int(-4);
	}
#endif
//判断文件名称和时长
	format = check_format(file_name);

	if(format < 0 ) {
		return mp_obj_new_int(-6);
	}
	
	callback_cur = self->callback;
	time = mp_obj_get_int(time_out);

	if(time < 1) {
		return mp_obj_new_int(-5);
	}

	HeliosFILE *fileID = NULL;
	fileID = Helios_fopen(self->file_name, "r");	
	if(fileID)
	{	
		audio_record_printf("fileID = %d\n",fileID);
		Helios_fclose(fileID);
		if(FindNode(self->filenameListHead,self->file_name) == NULL) {
			audio_record_printf("FindNode = null\n");
			//return mp_obj_new_int(-7);
		}
		Helios_remove(self->file_name);
	} else audio_record_printf("fileID = null\n");


    Helios_OSTimerAttr OSTimerAttr = {
        .ms = (uint32_t)time*1000,
        .cycle_enable = 0,
        .cb = helios_record_timer_cb,
        .argv = (void *)self
    };
	ret = Helios_OSTimer_Start(rec_timer, &OSTimerAttr);
	if(ret)
	{
		audio_record_printf("Timer starte failed\r\n");
	}
	audio_record_printf("Timer started\r\n");
	self->isbusy = 1;
	record_state = 1;
#if 1 
	AddListTill(&(self->filenameListHead),&(self->filenameListEnd),self->file_name);
	self_cur = self;
#endif
	Helios_AudRecordConfig record_conf;
	record_conf.channels = 1;
	record_conf.samplerate = 8000;

	if(format==FORMAT_WAV){
		Helios_AUDRecordInitStruct record_info;
		record_info.record_para = &record_conf;
		record_info.format = HELIOS_AUDIO_FORMAT_WAVPCM;
		record_info.type = HELIOS_REC_TYPE_MIC;
		record_info.cb = (void *)helios_file_record_cb;
		ret = Helios_Audio_FileRecordStart(self->file_name, &record_info);
	}
	else if(format==FORMAT_AMR){

	}
	else{
		return mp_obj_new_int(-6);
	}

	if(ret != 0) {
		Helios_Audio_FileRecordStop();
		self->isbusy = 0;
		record_state = 0;

		if(rec_timer != 0) {
			ret = Helios_OSTimer_Stop(rec_timer);
		}
		ret = -6;

	}
	
	return mp_obj_new_int(ret);
}


STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_helios_record_file_start_obj, helios_audio_record_start);



STATIC mp_obj_t helios_audio_record_stop(mp_obj_t self_in)
{
    int ret = 0;
	audio_record_obj_t *self = MP_OBJ_TO_PTR(self_in);
	ret = Helios_Audio_FileRecordStop();
	self->isbusy = 0;
	record_state = 0;

	if(rec_timer!= 0) 
		ret = Helios_OSTimer_Stop(rec_timer);

	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_helios_record_file_stop_obj, helios_audio_record_stop);

STATIC mp_obj_t helios_audio_record_getFilePath(mp_obj_t self_in,mp_obj_t path)
{
	audio_record_obj_t *self = MP_OBJ_TO_PTR(self_in);
	char name[256] = {0};
	char *file_name = (char*)mp_obj_str_get_str(path);
#ifdef PLAT_ASR
	sprintf(name, "U:/%s",file_name);
#endif
#ifdef PLAT_Unisoc
	sprintf(name, "UFS:%s",file_name);
#endif

	audio_record_printf("name = %s\n",name);

	HeliosFILE *fileID = NULL;
	fileID = Helios_fopen(name, "r");	
	if(fileID)
	{	 
		Helios_fclose(fileID);
		if(FindNode(self->filenameListHead,name) == NULL) {
			return mp_obj_new_int(-2);
		}
		return mp_obj_new_str(name,strlen(name));
		
	}

	return mp_obj_new_int(-1);
	
	//return mp_obj_new_str(file_name_str,strlen(file_name_str));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_helios_record_file_getFilePath_obj, helios_audio_record_getFilePath);

STATIC mp_obj_t helios_audio_record_getData(size_t n_args, const mp_obj_t *args)
{
	audio_record_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int ret = 0;
	static unsigned char *audio_buf = NULL;
	int offset_get = mp_obj_get_int(args[2]);
	int len_get = mp_obj_get_int(args[3]);
	

	HeliosFILE *fileID = NULL;
	if(audio_buf != NULL) {
		free(audio_buf);
		audio_buf = NULL;
	}

	
	
	char name[256] = {0};
	char *file_name = (char*)mp_obj_str_get_str(args[1]);
#ifdef PLAT_ASR
	sprintf(name, "U:/%s",file_name);
#endif
#ifdef PLAT_Unisoc
	sprintf(name, "UFS:%s",file_name);
#endif
	audio_record_printf("name = %s\n",name);

	
	fileID = Helios_fopen(name, "r");
	if(!(fileID)){
		return mp_obj_new_int(-2);
	}

	
	if(FindNode(self->filenameListHead,name) == NULL) {
		ret = -8;
		goto loop_getData;
	}
	if(self->isbusy == 1) {
		ret = -4;
		goto loop_getData;
	}


	if(len_get > 10240 || len_get < 0) {
		ret = -6;
		goto loop_getData;
	}

    if(fileID) {
		int file_size = Helios_fsize(fileID);
		if(offset_get + len_get > file_size) {
			ret = -5;
			goto loop_getData;
		}
	
		ret = Helios_fseek(fileID, offset_get, SEEK_SET);
		if (ret < 0) {
			audio_record_printf("helios_audio_record_getData helios_fseek end ret = %d\n", ret);
			ret = -3;
			goto loop_getData;
		}
		audio_buf = (unsigned char *)calloc(len_get,sizeof(unsigned char));
		if(audio_buf == NULL) {
			ret = -7;
			goto loop_getData;
		}
		ret = Helios_fread(audio_buf, len_get, 1, fileID);
		if ( ret <= 0 ) {
			ret = -1;
			goto loop_getData;
        }
		Helios_fclose(fileID);
		return mp_obj_new_bytes(audio_buf, len_get);
    }
	return mp_obj_new_int(-2);

loop_getData: 
	Helios_fclose(fileID);
	return mp_obj_new_int(ret);
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_helios_record_file_getData_obj,4,4, helios_audio_record_getData);

STATIC mp_obj_t helios_audio_record_getSize(mp_obj_t self_in,mp_obj_t path)
{
	audio_record_obj_t *self = MP_OBJ_TO_PTR(self_in);
	HeliosFILE *fileID = NULL;
	int file_size = 0;
	if(self->isbusy == 1) {
		return mp_obj_new_int(-3);
	}

	char name[256] = {0};
	char *file_name = (char*)mp_obj_str_get_str(path);
#ifdef PLAT_ASR
	sprintf(name, "U:/%s",file_name);
#endif
#ifdef PLAT_Unisoc
	sprintf(name, "UFS:%s",file_name);
#endif
	audio_record_printf("name = %s\n",name);
	if(FindNode(self->filenameListHead,name) == NULL) {
			return mp_obj_new_int(-4);
		}
	
	fileID = Helios_fopen(name, "r");
	if(fileID) {
		file_size = Helios_fsize(fileID);
		Helios_fclose(fileID);
		if(file_size <= 0)
		{
			return mp_obj_new_int(-1);
		}
		return mp_obj_new_int(file_size);
	}
	return mp_obj_new_int(-2);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_helios_record_file_getSize_obj, helios_audio_record_getSize);

STATIC mp_obj_t helios_audio_record_delete(size_t n_args, const mp_obj_t *args)	//(mp_obj_t self_in, mp_obj_t path)
{
	audio_record_printf("n_args = %d\n",n_args);
	audio_record_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	if(self->isbusy == 1) {
		return mp_obj_new_int(-2);
	}
	
	if(n_args == 1) {	
		FreeList(&(self->filenameListHead),&(self->filenameListEnd));
		return mp_obj_new_int(0);
	} 

	char name[256] = {0};
	char *file_name = (char*)mp_obj_str_get_str(args[1]);
#ifdef PLAT_ASR
	sprintf(name, "U:/%s",file_name);
#endif
#ifdef PLAT_Unisoc
	sprintf(name, "UFS:%s",file_name);
#endif
	audio_record_printf("name = %s\n",name);
	if(FindNode(self->filenameListHead,name) == NULL) {
			return mp_obj_new_int(-3);
	}

	HeliosFILE *fileID = NULL;
	fileID = Helios_fopen(name, "r");	 
	if(fileID)
	{	 
		Helios_fclose(fileID);
		Helios_remove(name);
		DeleteListRand(&(self->filenameListHead),&(self->filenameListEnd),name);
		return mp_obj_new_int(0);
	}
	
	return mp_obj_new_int(-1);
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_helios_record_file_delete_obj,    1,2, helios_audio_record_delete);

STATIC mp_obj_t helios_audio_record_exists(mp_obj_t self_in,mp_obj_t path)
{
	audio_record_obj_t *self = MP_OBJ_TO_PTR(self_in);
    HeliosFILE *fileID = NULL;

	char name[256] = {0};
	char *file_name = (char*)mp_obj_str_get_str(path);
#ifdef PLAT_ASR
	sprintf(name, "U:/%s",file_name);
#endif
#ifdef PLAT_Unisoc
	sprintf(name, "UFS:%s",file_name);
#endif
	audio_record_printf("name = %s\n",name);
	if(FindNode(self->filenameListHead,name) == NULL) {
			return mp_obj_new_int(-1);
		}

    fileID = Helios_fopen(name, "r");    
    if(fileID)
	{    
		Helios_fclose(fileID);

		return mp_obj_new_bool(1);
    }
    
    return mp_obj_new_bool(0); 
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_helios_record_file_exists_obj, helios_audio_record_exists);

STATIC mp_obj_t helios_audio_record_isBusy(mp_obj_t self_in)
{
	audio_record_obj_t *self = MP_OBJ_TO_PTR(self_in);
	
    return mp_obj_new_int(self->isbusy); 
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_helios_record_file_isBusy_obj, helios_audio_record_isBusy);

STATIC mp_obj_t helios_set_record_callback(mp_obj_t self_in, mp_obj_t callback)
{
	audio_record_obj_t *self = MP_OBJ_TO_PTR(self_in);
	self->callback = callback;
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_helios_record_file_end_callback_obj, helios_set_record_callback);

STATIC mp_obj_t helios_audio_record_list_file(mp_obj_t self_in)
{
	audio_record_obj_t *self = MP_OBJ_TO_PTR(self_in);
	char *file_name_str = ScanList(self->filenameListHead);
	
	return mp_obj_new_str(file_name_str,strlen(file_name_str));
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_helios_record_file_list_file_obj, helios_audio_record_list_file);



STATIC mp_obj_t helios_audio_record_gain(size_t n_args, const mp_obj_t *args)
{
	//audio_record_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	int codec_gain = mp_obj_get_int(args[1]);
	int dsp_gain = mp_obj_get_int(args[2]);

	
    if (!(codec_gain >= 0 && codec_gain <= 4)) {
		mp_raise_ValueError("invalid codec_gain value, must be in [0,4]");
	}
	if (!(dsp_gain >= -36 && dsp_gain <= 12)) {
		mp_raise_ValueError("invalid dsp_gain value, must be in [-36,12]");
	}

	audio_record_printf("codec_gain = %d\n",codec_gain);
	//qpy_set_txcodecgain(codec_gain); //gain:0~4
	
	audio_record_printf("dsp_gain = %d\n",dsp_gain);
	//qpy_set_txdspgain(dsp_gain); //gain: -36~12


	return mp_obj_new_int(0);

	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_helios_record_file_gain_obj,3,3, helios_audio_record_gain);

static int play_callback(char *p_data, int len, Helios_EnumAudPlayerState state)
{
	if(state == HELIOS_AUD_PLAYER_START)
	{
		audio_record_printf("player start run");
	}
	else if(state == HELIOS_AUD_PLAYER_FINISHED)
	{
		audio_record_printf("player stop run");
	}
	else
	{
		audio_record_printf("type is %d", state);
	}

	return 0;
}

STATIC mp_obj_t helios_audio_record_play(mp_obj_t self_in,mp_obj_t path, mp_obj_t audio_path)
{
	
	audio_record_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->isbusy == 1) {
		return mp_obj_new_int(-1);
	}
#if 0
	int ql_audio_path = mp_obj_get_int(audio_path);

	if(ql_audio_path != HELIOS_OUTPUT_RECEIVER && ql_audio_path != HELIOS_OUTPUT_HEADPHONE && ql_audio_path != HELIOS_OUTPUT_SPEAKER) {
		mp_raise_ValueError("invalid audio path, must be in {0 or 1 or 2}");
	}

	if(ql_audio_path == HELIOS_OUTPUT_RECEIVER) ql_set_audio_path_receiver();
	else if (ql_audio_path == HELIOS_OUTPUT_HEADPHONE) ql_set_audio_path_earphone();
	else ql_set_audio_path_speaker();
#endif
#if 1
	char name[256] = {0};
	char *file_name = (char*)mp_obj_str_get_str(path);
#ifdef PLAT_ASR
	sprintf(name, "U:/%s",file_name);
#endif
#ifdef PLAT_Unisoc
	sprintf(name, "UFS:%s",file_name);
#endif
	audio_record_printf("name = %s\n",name);
	
	if(Helios_Audio_FilePlayStart(name, 0, 0, HELIOS_AUDIO_PLAY_TYPE_LOCAL, device,(void *)play_callback))
	{
		audio_record_printf("play failed");
		return mp_obj_new_int(-1);;
	}
#endif
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_helios_record_file_play_obj, helios_audio_record_play);


STATIC const mp_rom_map_elem_t audio_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_start), 		MP_ROM_PTR(&mp_helios_record_file_start_obj) 	},
    { MP_ROM_QSTR(MP_QSTR_stop), 		MP_ROM_PTR(&mp_helios_record_file_stop_obj) 	},
    { MP_ROM_QSTR(MP_QSTR_getFilePath), MP_ROM_PTR(&mp_helios_record_file_getFilePath_obj) 	},
    { MP_ROM_QSTR(MP_QSTR_getData),		MP_ROM_PTR(&mp_helios_record_file_getData_obj) 	},
    { MP_ROM_QSTR(MP_QSTR_getSize),		MP_ROM_PTR(&mp_helios_record_file_getSize_obj) 	},
    { MP_ROM_QSTR(MP_QSTR_Delete),		MP_ROM_PTR(&mp_helios_record_file_delete_obj) 	},
    { MP_ROM_QSTR(MP_QSTR_exists),		MP_ROM_PTR(&mp_helios_record_file_exists_obj) 	},
    { MP_ROM_QSTR(MP_QSTR_isBusy),		MP_ROM_PTR(&mp_helios_record_file_isBusy_obj) 	},
    { MP_ROM_QSTR(MP_QSTR_end_callback),		MP_ROM_PTR(&mp_helios_record_file_end_callback_obj) 	},
    { MP_ROM_QSTR(MP_QSTR_list_file),		MP_ROM_PTR(&mp_helios_record_file_list_file_obj) 	},
    { MP_ROM_QSTR(MP_QSTR_gain),		MP_ROM_PTR(&mp_helios_record_file_gain_obj) 	},
    { MP_ROM_QSTR(MP_QSTR_play),		MP_ROM_PTR(&mp_helios_record_file_play_obj) 	},
};
STATIC MP_DEFINE_CONST_DICT(record_locals_dict, audio_locals_dict_table);

const mp_obj_type_t audio_record_type = {
    { &mp_type_type },
    .name = MP_QSTR_Record,
    .make_new = audio_record_make_new,
    .locals_dict = (mp_obj_dict_t *)&record_locals_dict,
};
