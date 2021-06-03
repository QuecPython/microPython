/**************************************************************************
 @file	modcamera.c
 @brief	camera module interface package.

 DESCRIPTION
 This module provide the camera playback interface of micropython.

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
27/01/2021  felixe.ye	Create.
**************************************************************************/

#include <stdio.h>
#include "modcamera.h"


STATIC const mp_rom_map_elem_t mp_module_camera_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_camera) },
    { MP_ROM_QSTR(MP_QSTR_camPreview), MP_ROM_PTR(&camera_preview_type) },
  //  { MP_ROM_QSTR(MP_QSTR_camRecord), MP_ROM_PTR(&camer_record_type) },
   // { MP_ROM_QSTR(MP_QSTR_camCaputre), MP_ROM_PTR(&camera_capture_type) },
    { MP_ROM_QSTR(MP_QSTR_camScandecode), MP_ROM_PTR(&camera_scandecode_type) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_camera_globals, mp_module_camera_globals_table);


const mp_obj_module_t mp_module_camera = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_camera_globals,
};



