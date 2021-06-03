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


#define OPEN_PREVIEW_AND_DECODE 1
const mp_obj_type_t camera_preview_type;


typedef struct _preview_obj_t {
    mp_obj_base_t base;
	unsigned short cam_w;
	unsigned short cam_h;
	unsigned short lcd_w;
	unsigned short lcd_h;
	unsigned char prebufcnt;
	//int  inited;
} preview_obj_t;

static Helios_CAMConfig camconfig = {0};


STATIC mp_obj_t preview_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) 
{
	enum { ARG_model, ARG_cam_w, ARG_cam_h, ARG_lcd_w, ARG_lcd_h, ARG_perview_level};
	
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_model, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
			{ MP_QSTR_cam_w, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 640} },
			{ MP_QSTR_cam_h, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 480} },
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

	if ( prebufcnt != 1 && prebufcnt != 2) {
       mp_raise_ValueError("perview_level must be [1,2]");
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

	printf("data camwidth etc = %d,%d,%d,%d,%d\n",camwidth,camheight,lcdprewidth,lcdpreheight,prebufcnt);
	
    preview_obj_t *self = m_new_obj(preview_obj_t);
	self->base.type = &camera_preview_type;
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

 
    return MP_OBJ_FROM_PTR(self);
}






STATIC mp_obj_t camera_open(mp_obj_t self_in)
{
	camconfig.preview = 1;
	printf("open %d,%d,%d,%d,%d\n",camconfig.camheight,camconfig.camwidth,camconfig.lcdpreheight,camconfig.lcdprewidth,camconfig.prebufcnt);
	int error =  Helios_camera_open(&camconfig);//ql_start_preview(data);

	return mp_obj_new_int(error);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_preview_open_obj, camera_open);

STATIC mp_obj_t camera_close(mp_obj_t self_in)
{

	int error =  Helios_camera_close();

	return mp_obj_new_int(error);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(camera_preview_close_obj, camera_close);




STATIC const mp_rom_map_elem_t Preview_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&camera_preview_open_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&camera_preview_close_obj) },

};
STATIC MP_DEFINE_CONST_DICT(camPreview_locals_dict, Preview_locals_dict_table);

const mp_obj_type_t camera_preview_type = {
    { &mp_type_type },
    .name = MP_QSTR_camPreview,
    .make_new = preview_make_new,
    .locals_dict = (mp_obj_dict_t *)&camPreview_locals_dict,
};
