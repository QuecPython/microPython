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

/**************************************************************************
 @file	camera_preview.c
 @brief	Add API to camera preview.

 DESCRIPTION
 This module provide the camera API of micropython.

 INITIALIZATION AND SEQUENCING REQUIREMENTS


 ===========================================================================
 Copyright (c) 2018 Quectel Wireless Solution, Co., Ltd.All Rights Reserved.
 Quectel Wireless Solution Proprietary and Confidential.
 ===========================================================================

						EDIT HISTORY FOR FILE
This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

WHEN		WHO			WHAT,WHERE,WHY
----------  ---------   ---------------------------------------------------
27/01/2021  felix.ye	Create.						
**************************************************************************/
#include <stdio.h>
#include "runtime.h"
	
#include "helios_camera.h"

#include "jpeglib.h"
#include "jpeg_operation.h"


#include "helios_debug.h"

#define CAPTURE_LOG(msg, ...)      custom_log("capture", msg, ##__VA_ARGS__)



#define OPEN_PREVIEW_AND_DECODE 1
const mp_obj_type_t camera_capture_type;

static c_callback_t *callback_cur = NULL;

typedef struct _preview_obj_t {
    mp_obj_base_t base;
	unsigned short cam_w;
	unsigned short cam_h;
	unsigned short lcd_w;
	unsigned short lcd_h;
	unsigned char prebufcnt;
	unsigned char perview;
	unsigned char decbufcnt;
	c_callback_t *callback;
	//int  inited;
} preview_obj_t;

static preview_obj_t *capture_obj = NULL;

static Helios_CAMConfig camconfig = {0};


static unsigned short *g_lcd_buffer = NULL;

#define __align(x) __attribute__ ((aligned (x)))
#define CAM_W 640
#define CAM_H 480
#define LCD_W 160
#define LCD_H 128
#define DECODE_BUF_CNT 0
#define PREVIEW_BUF_CNT 2

#define CAMERA_PREVIEW_TASK_PRIORITY	218 
#define CAMERA_DECODE_TASK_PRIORITY		227

typedef struct img_info_struct{
	int img_width;
	int img_height;
	unsigned char image_name[128];
}img_info_s;

static img_info_s image_info = {0};

void Helios_SaveImage(unsigned char* pYUVBuffer, int width, int height) {
	int result = -1;
	char name[256] = {0};

	
	sprintf(name, "U:/%s.jpeg",image_info.image_name);
	CAPTURE_LOG("SaveImage %d %d %d %d\n", width, height, image_info.img_width, image_info.img_height);

	if(image_info.img_width <= width && image_info.img_height <= height && pYUVBuffer != NULL) {
		
#if defined(PLAT_ASR)
		if(0 != yuv420_NV12_to_jpg(name, image_info.img_width, image_info.img_height, pYUVBuffer, width, height))
#elif defined(PLAT_Unisoc)
		if(0 != yuv422_UYVY_to_jpg(name, image_info.img_width, image_info.img_height, pYUVBuffer, width, height))
#else
		error("Helios_SaveImage:Uncertain picture format");
#endif
		{
			CAPTURE_LOG("Helios_SaveImage fail\n");
		} else {
			result = 0;
			CAPTURE_LOG("Helios_SaveImage success\n");
		}
	}
	
	if(callback_cur != NULL) {
		 mp_obj_t save_cb[2] = {
			mp_obj_new_int(result),
			mp_obj_new_str((char*)name,strlen((char*)name)),
		 };

	    mp_sched_schedule_ex(callback_cur, MP_OBJ_FROM_PTR(mp_obj_new_list(2, save_cb)));

	}
}

STATIC mp_obj_t caputre_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) 
{
	enum { ARG_model, ARG_cam_w, ARG_cam_h, ARG_perview_level, ARG_lcd_w, ARG_lcd_h};
	
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_model, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
			{ MP_QSTR_cam_w, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 640} },
			{ MP_QSTR_cam_h, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 480} },
			{ MP_QSTR_perview_level, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 1} },
			{ MP_QSTR_lcd_w, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 160} },
			{ MP_QSTR_lcd_h, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 128} },
			
		};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
	int model = args[ARG_model].u_int;
	unsigned short camwidth = (unsigned short)args[ARG_cam_w].u_int;
	unsigned short camheight = (unsigned short)args[ARG_cam_h].u_int;
	unsigned short lcdprewidth = (unsigned short)args[ARG_lcd_w].u_int;
	unsigned short lcdpreheight = (unsigned short)args[ARG_lcd_h].u_int;
	unsigned char prebufcnt = (unsigned char)args[ARG_perview_level].u_int;

	if ( prebufcnt != 1 && prebufcnt != 2 && prebufcnt != 0) {
       mp_raise_ValueError("perview_level must be [0,2]");
    }

	if ( lcdpreheight > 500) {
       mp_raise_ValueError("lcd_h must be (0,500)");
    }
	if ( lcdprewidth > 500) {
       mp_raise_ValueError("lcd_w must be (0,500)");
    }
	if ( camwidth > 640) {
       mp_raise_ValueError("camwidth must be (0,640)");
    }
	if ( camheight > 640) {
       mp_raise_ValueError("camheight must be (0,640)");
    }

	CAPTURE_LOG("data camwidth etc = %d,%d,%d,%d,%d\n",camwidth,camheight,lcdprewidth,lcdpreheight,prebufcnt);

	if(capture_obj == NULL) 
	{
		capture_obj = m_new_obj_with_finaliser(preview_obj_t);
	}
    preview_obj_t *self = capture_obj;
	self->base.type = &camera_capture_type;
	self->cam_h = camheight;
	self->cam_w = camwidth;
	self->lcd_h = lcdpreheight;
	self->lcd_w = lcdprewidth;
	self->prebufcnt = prebufcnt;

	camconfig.model = model;
	camconfig.camheight = camheight;
	camconfig.camwidth = camwidth;
	camconfig.lcdpreheight = lcdpreheight;
	camconfig.lcdprewidth = lcdprewidth;
	camconfig.prebufcnt = prebufcnt;
	camconfig.saveimg_pro = Helios_SaveImage;
	camconfig.decbufcnt = 1;
	camconfig.preview = 1;
	
	if(camconfig.prebufcnt == 0) {
		camconfig.preview = 0;
	}

 
    return MP_OBJ_FROM_PTR(self);
}


STATIC mp_obj_t camera_open(mp_obj_t self_in)
{
	CAPTURE_LOG("open %d,%d,%d,%d,%d\n",camconfig.camheight,camconfig.camwidth,camconfig.lcdpreheight,camconfig.lcdprewidth,camconfig.prebufcnt);
	int error =  Helios_camera_open(&camconfig);//ql_start_preview(data);

	return mp_obj_new_int(error);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_capture_open_obj, camera_open);

STATIC mp_obj_t camera_close(mp_obj_t self_in)
{

	int error =  Helios_camera_close();

	return mp_obj_new_int(error);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_capture_close_obj, camera_close);

STATIC const mp_arg_t capture_open_allowed_args[] = {
	{ MP_QSTR_image_w, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 640} },
	{ MP_QSTR_image_h, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 480} },
	{ MP_QSTR_picture_name, MP_ARG_REQUIRED | MP_ARG_OBJ,{.u_obj = MP_OBJ_NULL} },
};

STATIC mp_obj_t camera_capture_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
	
	enum { ARG_image_w, ARG_image_h, ARG_picture_name};
	
	mp_arg_val_t args[MP_ARRAY_SIZE(capture_open_allowed_args)];
	
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(capture_open_allowed_args), capture_open_allowed_args, args);

	char *picname = NULL;
	if (mp_obj_is_str(args[ARG_picture_name].u_obj)) {
		picname = (char*)mp_obj_str_get_str(args[ARG_picture_name].u_obj);
	}
	int image_w = (int)args[ARG_image_w].u_int;
	int image_h = (int)args[ARG_image_h].u_int;

	if(picname == NULL) {
		mp_raise_ValueError("image nane is error");
	}

	
	if ( image_w < 0 || image_w > camconfig.camwidth) {
       mp_raise_ValueError("image_w is less than cam_w");
    }

	if ( image_h > camconfig.camheight || image_h < 0) {
       mp_raise_ValueError("image_h is less tha cam_h");
    }

	memset(&image_info, 0, sizeof(img_info_s));
	image_info.img_height = image_h;
	image_info.img_width = image_w;
	memcpy(image_info.image_name, picname, strlen(picname));

	CAPTURE_LOG("capture para %d,%d,%s\n",image_w,image_h,picname);

	int ret = Helios_camera_capture();
	return mp_obj_new_int(ret);
    
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(camera_capture_start_obj,1, camera_capture_start);

STATIC mp_obj_t camera_save_callback(mp_obj_t self_in, mp_obj_t callback)
{
	preview_obj_t *self = MP_OBJ_TO_PTR(self_in);

	static c_callback_t cb = {0};
    memset(&cb, 0, sizeof(c_callback_t));
	callback_cur = &cb;
	mp_sched_schedule_callback_register(callback_cur, callback);
	
	self->callback = callback_cur;
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(camera_capture_callback_obj, camera_save_callback);


STATIC mp_obj_t camera_capture_deinit(mp_obj_t self_in)
{
	preview_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int ret = -1;
	ret = Helios_camera_close();
	callback_cur = NULL;
	capture_obj = NULL;
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_capture__del__obj, camera_capture_deinit);


STATIC const mp_rom_map_elem_t Preview_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), 	MP_ROM_PTR(&camera_capture__del__obj) },
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&camera_capture_open_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&camera_capture_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&camera_capture_start_obj) },
	{ MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&camera_capture_callback_obj) },
};
STATIC MP_DEFINE_CONST_DICT(camCapture_locals_dict, Preview_locals_dict_table);

const mp_obj_type_t camera_capture_type = {
    { &mp_type_type },
    .name = MP_QSTR_camCapture,
    .make_new = caputre_make_new,
    .locals_dict = (mp_obj_dict_t *)&camCapture_locals_dict,
};
