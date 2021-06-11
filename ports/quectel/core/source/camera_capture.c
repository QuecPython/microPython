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

#include "ql_qr_code.h"
#include "ql_camera.h"


#define OPEN_PREVIEW_AND_DECODE 1
STATIC const mp_obj_type_t camera_capture_type;


typedef struct _capture_obj_t {
    mp_obj_base_t base;
	unsigned short cam_w;
	unsigned short cam_h;
	unsigned short lcd_w;
	unsigned short lcd_h;
	unsigned char prebufcnt;
	unsigned char preview;
	//int  inited;
} capture_obj_t;

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

static CAM_DECODE_CONFIG_CTX camconfig_test =
{
	.camwidth      =CAM_W,     //
	.camheight     =CAM_H,
	.dectaskprio   =CAMERA_DECODE_TASK_PRIORITY,
	.decbufcnt     =DECODE_BUF_CNT,      //解码buffer的数量
	.decbufaddr    =NULL,   //解码buffer的首地址
	.decodecb      =NULL,   //解码成功的回调
	.preview       =1,      //是否需要进行预览
	.pretaskprio   =CAMERA_PREVIEW_TASK_PRIORITY,//预览任务的优先级
	.prebufcnt     =PREVIEW_BUF_CNT, //预览的buffer数量
	.prebufaddr    =NULL,//campreviewbuf,   //预览的buffer首地址---用于Camera预览数据（NV12）保存
	.lcdmemeryaddr =NULL,     //LCD的显存首地址-------------预览的时候,需要将预览的数据转化到此buffer
	.lcdprewidth   =LCD_W,    //预览图像的宽度
	.lcdpreheight  =LCD_H,    //预览图像的高度
};

STATIC mp_obj_t caputre_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) 
{
	enum { ARG_model, ARG_cam_w, ARG_cam_h, ARG_perview, ARG_lcd_w, ARG_lcd_h, ARG_perview_level};
	
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_model, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
			{ MP_QSTR_cam_w, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 640} },
			{ MP_QSTR_cam_h, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 480} },
			{ MP_QSTR_perview, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
			{ MP_QSTR_lcd_w, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 160} },
			{ MP_QSTR_lcd_h, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 128} },
			{ MP_QSTR_perview_level, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 1} },
		};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
	int model = args[ARG_model].u_int;
	unsigned short camwidth = (unsigned short)args[ARG_cam_w].u_int;
	unsigned short camheight = (unsigned short)args[ARG_cam_h].u_int;
	unsigned short lcdprewidth = (unsigned short)args[ARG_lcd_w].u_int;
	unsigned short lcdpreheight = (unsigned short)args[ARG_lcd_h].u_int;
	unsigned char prebufcnt = (unsigned char)args[ARG_perview_level].u_int;
	unsigned char perview = (unsigned char)args[ARG_perview].u_int;

	if ( prebufcnt != 1 && prebufcnt != 2) {
       mp_raise_ValueError("perview_level must be [1,2]");
    }

	if ( lcdpreheight > 500 || lcdpreheight < 0) {
       mp_raise_ValueError("lcd_h must be (0,500)");
    }
	if ( lcdprewidth > 500 || lcdprewidth < 0) {
       mp_raise_ValueError("lcd_w must be (0,500)");
    }
	if ( camwidth > 640 || camwidth < 0) {
       mp_raise_ValueError("camwidth must be (0,640)");
    }
	if ( camheight > 640 || camheight < 0) {
       mp_raise_ValueError("camheight must be (0,640)");
    }
	if (perview != 0 && perview != 1) {
		mp_raise_ValueError("camheight must be [0,1]");
	}

	uart_printf("data camwidth etc = %d,%d,%d,%d,%d,%d\n",camwidth,camheight,lcdprewidth,lcdpreheight,prebufcnt,perview);
	
    capture_obj_t *self = m_new_obj(capture_obj_t);
	self->base.type = &camera_capture_type;
	self->cam_h = camheight;
	self->cam_w = camwidth;
	self->lcd_h = lcdpreheight;
	self->lcd_w = lcdprewidth;
	self->prebufcnt = prebufcnt;
	self->preview = perview;

	camconfig_test.camheight = camheight;
	camconfig_test.camwidth = camwidth;
	camconfig_test.lcdpreheight = lcdpreheight;
	camconfig_test.lcdprewidth = lcdprewidth;
	camconfig_test.prebufcnt = prebufcnt;
	camconfig_test.preview = perview;

#if 0
	g_lcd_buffer = Ql_LCD_Get_Frame_Buffer_addr();
	printf("[LCD-OPEN] LCD memory addr:0x%x\r\n",g_lcd_buffer);
	
	if(g_lcd_buffer == NULL) {
		return mp_obj_new_int(-1);
	}
	camconfig_test.lcdmemeryaddr = g_lcd_buffer;
#else
	uart_printf("[LCD-OPEN] The LCD has no display memory now!\r\n");
#endif

    return MP_OBJ_FROM_PTR(self);
}






STATIC mp_obj_t ql_camera_open(mp_obj_t self_in)
{
	QLStartPreviewStruct data = {0};
	capture_obj_t *self = MP_OBJ_TO_PTR(self_in);
	data.image_height = self->lcd_h;
	data.image_width = self->lcd_w;
	uart_printf("open %d,%d,%d,%d,%d\n",camconfig_test.camheight,camconfig_test.camwidth,camconfig_test.lcdpreheight,camconfig_test.lcdprewidth,camconfig_test.prebufcnt);
	int error =  ql_qr_camera_open(&camconfig_test);//ql_start_preview(data);

	return mp_obj_new_int(error);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_capture_open_obj, ql_camera_open);

STATIC mp_obj_t ql_camera_close(mp_obj_t self_in)
{

	int error =  ql_qr_camera_close();

	return mp_obj_new_int(error);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_capture_close_obj, ql_camera_close);


STATIC mp_obj_t ql_camera_capture_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
	
	enum { ARG_image_w, ARG_image_h, ARG_picture_name};
	
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_image_w, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 640} },
			{ MP_QSTR_image_h, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 480} },
			{ MP_QSTR_picture_name, MP_ARG_REQUIRED | MP_ARG_OBJ,{.u_obj = MP_OBJ_NULL} },
		};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	char *picname = NULL;
	if (mp_obj_is_str(args[ARG_picture_name].u_obj)) {
		picname = (char*)mp_obj_str_get_str(args[ARG_picture_name].u_obj);
	}
	int image_w = (int)args[ARG_image_w].u_int;
	int image_h = (int)args[ARG_image_h].u_int;

	
	if ( image_w < 0 || image_w > camconfig_test.camwidth) {
       mp_raise_ValueError("image_w is less than cam_w");
    }

	if ( image_h > camconfig_test.camheight || image_h < 0) {
       mp_raise_ValueError("image_h is less tha cam_h");
    }

	uart_printf("capture para %d,%d,%s\n",image_w,image_h,picname);

	int ret = ql_camera_caputre_start(image_w, image_h, picname);
	return mp_obj_new_int(ret);
    
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(camera_capture_start_obj,1, ql_camera_capture_start);





STATIC const mp_rom_map_elem_t Preview_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&camera_capture_open_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&camera_capture_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&camera_capture_start_obj) },

};
STATIC MP_DEFINE_CONST_DICT(camCapture_locals_dict, Preview_locals_dict_table);

const mp_obj_type_t camera_capture_type = {
    { &mp_type_type },
    .name = MP_QSTR_camCaputre,
    .make_new = caputre_make_new,
    .locals_dict = (mp_obj_dict_t *)&camCapture_locals_dict,
};
