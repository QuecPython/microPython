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

#include "zbar.h"
#include "image.h"

#include "helios_debug.h"

#define SCANDE_LOG(msg, ...)      custom_log("scandecode", msg, ##__VA_ARGS__)



#define OPEN_PREVIEW_AND_DECODE 1
const mp_obj_type_t camera_scandecode_type;

static mp_obj_t callback_cur = NULL;

static Helios_CAMConfig camconfig = {0};


static char camera_decode_switch = 0;

typedef enum {
	DECODE_OPEN,
	DECODE_CLOSE
}Camera_decode_switch;

typedef struct _preview_obj_t {
    mp_obj_base_t base;
	unsigned short cam_w;
	unsigned short cam_h;
	unsigned short lcd_w;
	unsigned short lcd_h;
	unsigned char prebufcnt;
	unsigned char perview;
	unsigned char decbufcnt;
	mp_obj_t callback;
	//int  inited;
} preview_obj_t;

zbar_image_scanner_t *scanner = NULL;

 void camDecode(unsigned char* raw, int width, int height) {
 	if(camera_decode_switch == DECODE_OPEN) {
		/* create a reader */
		scanner = zbar_image_scanner_create();
	
		/* configure the reader */
		zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);
	
		/* obtain image data */
		//int width = 0, height = 0;
		//void *raw = NULL;
		//get_data("barcode.png", &width, &height, &raw);
	
		/* wrap image data */
		zbar_image_t *image = zbar_image_create();
		zbar_image_set_format(image, *(int*)"Y800");
		zbar_image_set_size(image, width, height);
		zbar_image_set_data(image, raw, width * height, zbar_image_free_data);
	
		/* scan the image for barcodes */
		int n = zbar_scan_image(scanner, image);
		if(n == 0) {
			goto lop;
		}
		/* extract results */
		const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
		
		for(; symbol; symbol = zbar_symbol_next(symbol)) {
			/* do something useful with results */
			zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
			const char *data = zbar_symbol_get_data(symbol);
			SCANDE_LOG("decoded %s symbol \"%s\"\r\n",
				   zbar_get_symbol_name(typ), data);
			
			/*SCANDE_LOG("len = %d\r\n",strlen(data));
			for(int k=0; k<strlen(data); k++)
			{
				SCANDE_LOG("%X ", (uint8_t)data[k]);
			}*/
	
		   if(callback_cur != NULL) {
				mp_obj_t decode_cb[2] = {
				   mp_obj_new_int(0),
				   mp_obj_new_str((char*)data,strlen((char*)data)),
				};
			   if(mp_obj_is_callable(callback_cur)){
				   mp_sched_schedule(callback_cur, MP_OBJ_FROM_PTR(mp_obj_new_list(2, decode_cb)));
			   }
		   }
			
		}
	
	lop:
		/* clean up */
		zbar_image_destroy(image);
		zbar_image_scanner_destroy(scanner);
	}
 
 }


static void camera_switch(char value) {
	camera_decode_switch = value;
}


STATIC mp_obj_t scandecode_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) 
{
	enum { ARG_model, ARG_decode_level, ARG_cam_w, ARG_cam_h, ARG_perview_level, ARG_lcd_w, ARG_lcd_h };
	
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_model, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
			{ MP_QSTR_decode_level, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
			{ MP_QSTR_cam_w, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 640} },
			{ MP_QSTR_cam_h, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 480} },
			{ MP_QSTR_perview_level, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 1} },
			{ MP_QSTR_lcd_w, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 176} },
			{ MP_QSTR_lcd_h, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 220} },
			
		};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
	int model = args[ARG_model].u_int;
	unsigned short camwidth = (unsigned short)args[ARG_cam_w].u_int;
	unsigned short camheight = (unsigned short)args[ARG_cam_h].u_int;
	unsigned short lcdprewidth = (unsigned short)args[ARG_lcd_w].u_int;
	unsigned short lcdpreheight = (unsigned short)args[ARG_lcd_h].u_int;
	unsigned char prebufcnt = (unsigned char)args[ARG_perview_level].u_int;
	unsigned char decbufcnt = (unsigned char)args[ARG_decode_level].u_int;

	if ( decbufcnt != 1 && decbufcnt != 2) {
       mp_raise_ValueError("decode_level must be [1,2]");
    }

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

	SCANDE_LOG("data camwidth etc = %d,%d,%d,%d,%d\n",camwidth,camheight,lcdprewidth,lcdpreheight,prebufcnt);

	
    preview_obj_t *self = m_new_obj(preview_obj_t);
	self->base.type = &camera_scandecode_type;
	self->cam_h = camheight;
	self->cam_w = camwidth;
	self->lcd_h = lcdpreheight;
	self->lcd_w = lcdprewidth;
	self->prebufcnt = prebufcnt;
	self->decbufcnt = decbufcnt;
	self->callback = NULL;

	camconfig.camheight = camheight;
	camconfig.camwidth = camwidth;
	camconfig.lcdpreheight = lcdpreheight;
	camconfig.lcdprewidth = lcdprewidth;
	camconfig.prebufcnt = prebufcnt;
	camconfig.decbufcnt = decbufcnt;
	camconfig.preview = 1;
	camconfig.model = model;
	camconfig.decode_pro = camDecode;

	camera_switch(DECODE_CLOSE);
	
	if(prebufcnt == 0) {
		camconfig.preview = 0;
	}
 
    return MP_OBJ_FROM_PTR(self);
}






STATIC mp_obj_t camera_open(mp_obj_t self_in)
{
	int error =  Helios_camera_open(&camconfig);//ql_start_preview(data);

	return mp_obj_new_int(error);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_scandecode_open_obj, camera_open);

STATIC mp_obj_t camera_close(mp_obj_t self_in)
{

	int error =  Helios_camera_close();
	camera_switch(DECODE_CLOSE);

	return mp_obj_new_int(error);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_scandecode_close_obj, camera_close);

STATIC mp_obj_t camera_scandecode_start(mp_obj_t self_in)
{
	camera_switch(DECODE_OPEN);
	int error =  Helios_camera_scandecode_start();

	return mp_obj_new_int(error);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_scandecode_start_obj, camera_scandecode_start);

STATIC mp_obj_t camera_scandecode_pause(mp_obj_t self_in)
{
	camera_switch(DECODE_CLOSE);

	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_scandecode_pause_obj, camera_scandecode_pause);

STATIC mp_obj_t camera_scandecode_resume(mp_obj_t self_in)
{
	camera_switch(DECODE_OPEN);

	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_scandecode_resume_obj, camera_scandecode_resume);

STATIC mp_obj_t camera_scandecode_stop(mp_obj_t self_in)
{
	
	camera_switch(DECODE_CLOSE);
	int error =  Helios_camera_scandecode_stop();

	return mp_obj_new_int(error);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_scandecode_stop_obj, camera_scandecode_stop);



STATIC mp_obj_t camera_scandecode_callback(mp_obj_t self_in, mp_obj_t callback)
{
	preview_obj_t *self = MP_OBJ_TO_PTR(self_in);
	self->callback = callback;
	callback_cur = callback;
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(camera_scandecode_callback_obj, camera_scandecode_callback);


STATIC const mp_rom_map_elem_t scandecode_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&camera_scandecode_open_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&camera_scandecode_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&camera_scandecode_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_pause), MP_ROM_PTR(&camera_scandecode_pause_obj) },
    { MP_ROM_QSTR(MP_QSTR_resume), MP_ROM_PTR(&camera_scandecode_resume_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&camera_scandecode_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&camera_scandecode_callback_obj) },
    

};
STATIC MP_DEFINE_CONST_DICT(camScandecode_locals_dict, scandecode_locals_dict_table);

const mp_obj_type_t camera_scandecode_type = {
    { &mp_type_type },
    .name = MP_QSTR_camScandecode,
    .make_new = scandecode_make_new,
    .locals_dict = (mp_obj_dict_t *)&camScandecode_locals_dict,
};
