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



