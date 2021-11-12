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
 * @file    modqrcode.c
 * @author  felix.ye
 * @version V1.0.0
 * @date    2021/11/4
 * @brief   xxx
 ******************************************************************************
 */
#include <stdio.h>
#include <string.h>

#include "runtime.h"
#include "gc.h"
#include "mphal.h"
#include "mperrno.h"

#include "helios_os.h"
#include "helios_lcd.h"
#include "QR_Encode.h"



unsigned short *qrcode_data = NULL;
static Helios_Mutex_t  qrcode_MutexRef = 0;
static int first_show = 1;


static int qrcode_amplification(unsigned int qrcode_size, unsigned int magnification,
		unsigned short Background_color, unsigned short Foreground_color) {

	unsigned int i = 0, j = 0, k = 0, l = 0;

	//unsigned short * qrcode = NULL;
	if(qrcode_data != NULL) {
		free(qrcode_data);
		qrcode_data = NULL;
	}
	
	qrcode_data = calloc(qrcode_size*qrcode_size*magnification*magnification,sizeof(unsigned short));
	if(qrcode_data == NULL) return -1;

	for(i = 0; i < qrcode_size; i++) {
		for(j = 0; j < qrcode_size ; j++) {
			if(m_byModuleData[i][j] == 1) {
				for(k = 0; k < magnification; k++) {
					for(l = 0; l < magnification; l++) {
						qrcode_data[(magnification * i+k)*qrcode_size * magnification + (magnification * j+l)] = Foreground_color;
					}
				}
			}
			else {
				for(k = 0; k < magnification; k++) {
					for(l = 0; l < magnification; l++) {
						qrcode_data[(magnification * i+k)*qrcode_size * magnification + (magnification * j+l)] = Background_color;
					}
				}
			}

		}
	}

	return 0;
	
}


int qpy_qrcode_show(char *qrcode_str, unsigned int magnification, 
	unsigned int start_x, unsigned start_y,unsigned short Background_color, unsigned short Foreground_color) {

	int ret = 0;
	
	if(first_show == 1) {
		qrcode_MutexRef = Helios_Mutex_Create();
		first_show = 0;
	}

	int ret1 = Helios_Mutex_Lock(qrcode_MutexRef, HELIOS_WAIT_FOREVER);
	memset(m_byModuleData,0,sizeof(m_byModuleData));

	int qrcode_size = EncodeData(qrcode_str);

	if(qrcode_size < 0) {
		ret = -1;
		goto lopo;
	}

	if(qrcode_amplification(qrcode_size, magnification,Background_color,Foreground_color) < 0) {
		ret = -2;
		goto lopo;
	}
	if(Helios_LCD_Write((unsigned char*)qrcode_data, start_x, start_y, start_x+qrcode_size*magnification-1, start_y+qrcode_size*magnification-1) != 0) {
		ret = -3;
		goto lopo;
	}
	

lopo:
	if(qrcode_data != NULL) {
		free(qrcode_data);
		qrcode_data = NULL;
	}
	if(!ret1)
		Helios_Mutex_Unlock(qrcode_MutexRef);
	return ret;
}



STATIC mp_obj_t qrcode_show(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	const char* qrcode_str =  mp_obj_str_get_str(args[0]);
	if(strlen(qrcode_str) > 192) mp_raise_ValueError("String length cannot exceed 192");

	int magnification = mp_obj_get_int(args[1]);
	if(magnification < 1 || magnification > 6) mp_raise_ValueError("Wrong magnification");

	unsigned int start_x = mp_obj_get_int(args[2]);
	unsigned int start_y = mp_obj_get_int(args[3]);

	unsigned short Background_color = 0xffff;
	unsigned short Foreground_color = 0x0000;
	

	if(n_args == 6) {
		Background_color = (unsigned short)mp_obj_get_int(args[4]);
		Foreground_color = (unsigned short)mp_obj_get_int(args[5]);
	} 

			
 	ret = qpy_qrcode_show((char*)qrcode_str, magnification, start_x,start_y,Background_color,Foreground_color);
 	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qrcode_show_obj, 4, 6, qrcode_show);


STATIC const mp_rom_map_elem_t mp_module_qrcode_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sim) },
    { MP_ROM_QSTR(MP_QSTR_show), MP_ROM_PTR(&qrcode_show_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mp_module_qrcode_globals, mp_module_qrcode_globals_table);


const mp_obj_module_t mp_module_qrcode = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_qrcode_globals,
};


