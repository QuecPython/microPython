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
 * @file    machine_lcd.c
 * @author  xxx
 * @version V1.0.0
 * @date    2019/04/16
 * @brief   xxx
 ******************************************************************************
 */
#include <stdio.h>
#include <string.h>

#include "runtime.h"
#include "gc.h"
#include "mphal.h"
#include "mperrno.h"


#include "modmachine.h"


#include "helios_lcd.h"
#include "helios_fs.h"

#ifdef CONFIG_JPEG  
#include "jpeglib.h"
#include "jpeg_operation.h"
#endif

#include "helios_debug.h"

#define LCD_LOG(msg, ...)      custom_log("lcd", msg, ##__VA_ARGS__)


#if 1

//******************invaild data predefine*******************//

const mp_obj_type_t machine_lcd_type;



//******************invaild data predefine end*******************//



#define SPI_LCD_WIDTH	128
#define SPI_LCD_HEIGTH	160


typedef struct machine_lcd_obj_struct {
    mp_obj_base_t base;
    mp_buffer_info_t lcd_init_data;
    unsigned char *fb;
    unsigned short bg_color;
	unsigned short width;
	unsigned short height;
	mp_buffer_info_t quec_lcd_invalid_data;
	mp_buffer_info_t quec_lcd_display_on_data;
	mp_buffer_info_t quec_lcd_display_off_data;
	mp_buffer_info_t quec_lcd_set_brightness_data;
}machine_lcd_obj_t;




static machine_lcd_obj_t *self_lcd;


STATIC mp_obj_t machine_lcd_clear(mp_obj_t self_in, mp_obj_t data)
{
	
    machine_lcd_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->bg_color = mp_obj_get_int(data);
	int ret = Helios_LCD_Clear(self->bg_color);
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_lcd_clear_obj, machine_lcd_clear);

STATIC mp_obj_t machine_lcd_brightness(mp_obj_t self_in, mp_obj_t data)
{
	int ret = 0; 
	int bright_level = mp_obj_get_int(data);
	if(bright_level > 5 || bright_level < 0) {
		return mp_obj_new_int(-1);
	}
	ret = Helios_LCD_Brightness(bright_level);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_lcd_brightness_obj, machine_lcd_brightness);

STATIC mp_obj_t machine_lcd_write(size_t n_args, const mp_obj_t *args)
{	

	if(6 != n_args) {
		printf("n_args error = %d\n",n_args);
		return mp_const_false;
	}
	unsigned short start_x = mp_obj_get_int(args[2]);
	unsigned short start_y = mp_obj_get_int(args[3]);
	unsigned short end_x = mp_obj_get_int(args[4]);
	unsigned short end_y = mp_obj_get_int(args[5]);


	mp_buffer_info_t lcd_write_data = {0};

	
	mp_get_buffer_raise(args[1], &lcd_write_data, MP_BUFFER_READ);
	

	if(lcd_write_data.buf == NULL || lcd_write_data.len == 0) {
		return mp_obj_new_int(-3);
	}


	int ret = Helios_LCD_Write(lcd_write_data.buf, start_x, start_y, end_x, end_y);
	
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_lcd_write_obj, 1, 6, machine_lcd_write);

STATIC mp_obj_t machine_lcd_display_on(mp_obj_t self_in)
{
	printf("machine_lcd_display_on\n");
    int ret = Helios_LCD_Display_on();
    //lcd_display_on_to_python();
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_lcd_display_on_obj, machine_lcd_display_on);

STATIC mp_obj_t machine_lcd_display_off(mp_obj_t self_in)
{
	printf("machine_lcd_display_off\n");
    int ret = Helios_LCD_Display_off();
    //lcd_display_off_to_python();
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_lcd_display_off_obj, machine_lcd_display_off);

STATIC mp_obj_t machine_lcd_write_cmd(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	//machine_lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	int cmd = mp_obj_get_int(args[1]);
	int data_len = mp_obj_get_int(args[2]);
	printf("cmd = %d, data_len = %d\n",cmd, data_len);

	ret = Helios_LCD_WriteCmd(cmd,data_len);
	
    return mp_obj_new_int(ret);;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_lcd_write_cmd_obj, 3, 3, machine_lcd_write_cmd);

STATIC mp_obj_t machine_lcd_write_data(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	//machine_lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	int data = mp_obj_get_int(args[1]);
	int data_len = mp_obj_get_int(args[2]);
	printf("data = %d, data_len = %d\n",data, data_len);
	ret = Helios_LCD_WriteData(data,data_len);
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_lcd_write_data_obj, 3, 3, machine_lcd_write_data);





STATIC mp_obj_t machine_lcd_init_helper(machine_lcd_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	enum { ARG_initbuf, ARG_width, ARG_hight, ARG_clk, ARG_dataline, ARG_linenum, ARG_type, ARG_invaildbuf, ARG_displayonbuf, ARG_displayoffbuf, ARG_lightbuf };
	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_initbuf,	   MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_width,	MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_hight,	MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_clk,	  MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_dataline,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_linenum,	  MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_type,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_invaildbuf,	   MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },	
		{ MP_QSTR_displayonbuf,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_displayoffbuf,	   MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_lightbuf,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
	};

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	
	Helios_LCDInitStruct init_data = {0};
	
	mp_get_buffer_raise(args[ARG_initbuf].u_obj, &(self->lcd_init_data), MP_BUFFER_READ);	
	if(self->lcd_init_data.len <= 0 || self->lcd_init_data.buf == NULL || self->lcd_init_data.len > 1000) {
		mp_raise_ValueError("invalid init data");
	}
	
	init_data.init_data = self->lcd_init_data.buf;
	init_data.init_data_len = self->lcd_init_data.len;

	self_lcd->width = args[ARG_width].u_int;
	self_lcd->height = args[ARG_hight].u_int;

	init_data.width = args[ARG_width].u_int;
	init_data.hight = args[ARG_hight].u_int;
	init_data.dataline = args[ARG_dataline].u_int;
	init_data.linenum = args[ARG_linenum].u_int;
	init_data.lcdtype = args[ARG_type].u_int;

    switch (args[ARG_clk].u_int) {
        case 0:
            break;
        case HELIOS_SPI_LCD_CLK_6_5M:
		case HELIOS_SPI_LCD_CLK_13M:
		case HELIOS_SPI_LCD_CLK_26M:
		case HELIOS_SPI_LCD_CLK_52M:
            init_data.clk = args[ARG_clk].u_int;
            break;
        default:
            mp_raise_ValueError("invalid clk");
            break;
    }

	if(init_data.width > 500 || init_data.hight > 500|| 
		(init_data.clk != HELIOS_SPI_LCD_CLK_6_5M &&
		init_data.clk != HELIOS_SPI_LCD_CLK_13M &&
		init_data.clk != HELIOS_SPI_LCD_CLK_26M &&
		init_data.clk != HELIOS_SPI_LCD_CLK_52M) || 
		(init_data.dataline != 1 && init_data.dataline != 2) ||
		(init_data.linenum != 4 && init_data.linenum != 3) ||
		(init_data.lcdtype != 0 && init_data.lcdtype != 1 )) {
		mp_raise_ValueError("Initialization parameter error(clk or dataline or linenum or lcdtype)");
		return mp_obj_new_int(-5);
	}

	
	mp_get_buffer_raise(args[ARG_invaildbuf].u_obj, &(self->quec_lcd_invalid_data), MP_BUFFER_READ);
	if(self->quec_lcd_invalid_data.len <= 0 || self->quec_lcd_invalid_data.buf == NULL || self->quec_lcd_invalid_data.len > 1000) {
		mp_raise_ValueError("invalid invaild data");
	}
	init_data.invaild_data = self->quec_lcd_invalid_data.buf;
	init_data.invaild_data_len = self->quec_lcd_invalid_data.len;


	if(args[ARG_displayonbuf].u_obj != mp_const_none) {
		mp_get_buffer_raise(args[ARG_displayonbuf].u_obj, &(self->quec_lcd_display_on_data), MP_BUFFER_READ);
		if(self->quec_lcd_display_on_data.len <= 0 || self->quec_lcd_display_on_data.buf == NULL || self->quec_lcd_display_on_data.len > 1000) {
			mp_raise_ValueError("invalid display on data");
		}
		init_data.displayon_data = self->quec_lcd_display_on_data.buf;
		init_data.displayon_data_len = self->quec_lcd_display_on_data.len;
	}

	
	if(args[ARG_displayoffbuf].u_obj != mp_const_none) {
		mp_get_buffer_raise(args[ARG_displayoffbuf].u_obj, &(self->quec_lcd_display_off_data), MP_BUFFER_READ);
		if(self->quec_lcd_display_off_data.len <= 0 || self->quec_lcd_display_off_data.buf == NULL || self->quec_lcd_display_off_data.len > 1000) {
			mp_raise_ValueError("invalid display off data");
		}
		init_data.displayoff_data = self->quec_lcd_display_off_data.buf;
		init_data.displayoff_data_len = self->quec_lcd_display_off_data.len;
	}

	if(args[ARG_lightbuf].u_obj != mp_const_none) {
		mp_get_buffer_raise(args[ARG_lightbuf].u_obj, &(self->quec_lcd_set_brightness_data), MP_BUFFER_READ);
		if(self->quec_lcd_set_brightness_data.len <= 0 || self->quec_lcd_set_brightness_data.buf == NULL || self->quec_lcd_set_brightness_data.len > 1000) {
			mp_raise_ValueError("invalid brightness  data");
		}
		init_data.light_level_data = self->quec_lcd_set_brightness_data.buf;
		init_data.light_level_data_len = self->quec_lcd_set_brightness_data.len;
		
	}
	

	int ret = Helios_LCD_Init(&init_data);
	if(ret != 0) {
		mp_raise_ValueError("init fail");
	}
	return mp_obj_new_int(ret);
}

STATIC mp_obj_t machine_lcd_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    machine_lcd_init_helper(args[0], n_args - 1, args + 1, kw_args);
	return mp_obj_new_int(0);
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_lcd_init_obj, 1, machine_lcd_init);



STATIC mp_obj_t machine_lcd_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{

   // mp_arg_check_num(n_args, n_kw, 0, 0, true);
	if(self_lcd != NULL) {
		m_del_obj(machine_lcd_obj_t,self_lcd);
		self_lcd = NULL;
	}
    self_lcd = m_new_obj(machine_lcd_obj_t);
	memset(self_lcd,0,sizeof(machine_lcd_obj_t));
	
    self_lcd->base.type = &machine_lcd_type;
    self_lcd->bg_color = 0;

	if(n_args > 0) {	
		mp_map_t kw_args;
	    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
		machine_lcd_init_helper(self_lcd, n_args, args, &kw_args);
	}
	
    return MP_OBJ_FROM_PTR(self_lcd);
}

static int Helios_lcd_write_data_by_file(char* file_name, unsigned int start_x, unsigned int start_y) {
	
	HeliosFILE *fileID = NULL;
	unsigned char head_data[8] = {0};
	unsigned int width = 0;
	unsigned int hight = 0;
	int ret = 0;

	
	char name[256] = {0};
	sprintf(name, "U:/%s",file_name);
	
	fileID = Helios_fopen(name, "r");
	if(!(fileID)){
		return (-1);
	}

	
	
	ret = Helios_fread(head_data, 8, 1, fileID);
	if(head_data[0] == 0x10) {
		width = (head_data[2] << 8) | head_data[3];
		hight = (head_data[4] << 8) | head_data[5];
	} else {
		width = (head_data[3] << 8) | head_data[2];
		hight = (head_data[5] << 8) | head_data[4];
	}

	unsigned char *data = (unsigned char *)calloc(width*hight*2,sizeof(unsigned char));
	if(data == NULL) {
		return (-2);
	}
	
	ret = Helios_fread(data, width*hight*2, 1, fileID);
	if ( ret <= 0 ) {
		Helios_fclose(fileID);
		if(data != NULL) {
			free(data);
			data = NULL;
		}
		return (-3);
	}

	Helios_fclose(fileID);
	
	ret = Helios_LCD_Write(data, start_x, start_y, width+start_x-1, hight+start_y-1);

	if(data != NULL) {
		free(data);
		data = NULL;
	}

	return ret;
}


static int Helios_lcd_write_data_by_file_wh(char* file_name, unsigned int start_x, unsigned int start_y,unsigned int width, unsigned int hight) {
	
	HeliosFILE *fileID = NULL;
	int ret = 0;

	char name[256] = {0};
	sprintf(name, "U:/%s",file_name);
	
	fileID = Helios_fopen(name, "r");
	if(!(fileID)){
		return (-1);
	}
	
	if(width > self_lcd->width || hight > self_lcd->height) return -2;

	unsigned char *data = (unsigned char *)calloc(width*hight*2,sizeof(unsigned char));
	if(data == NULL) {
		return (-3);
	}
	
	ret = Helios_fread(data, width*hight*2, 1, fileID);
	if ( ret <= 0 ) {
		Helios_fclose(fileID);
		if(data != NULL) {
			free(data);
			data = NULL;
		}
		return (-3);
	}
	
	ret = Helios_LCD_Write(data, start_x, start_y, width+start_x-1, hight+start_y-1);

	if(data != NULL) {
		free(data);
		data = NULL;
	}

	return ret;
}

STATIC mp_obj_t machine_lcd_show_data(size_t n_args, const mp_obj_t *args)
{

#if 1
	int ret = 0;
	int start_x = 0;
	int start_y = 0;
	int width = 0;
	int hight = 0;
	//machine_lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	char *file_name = (char*)mp_obj_str_get_str(args[1]);
	start_x = mp_obj_get_int(args[2]);
	start_y = mp_obj_get_int(args[3]);
	if(n_args == 4) {
		ret = Helios_lcd_write_data_by_file(file_name,start_x,start_y);
	} else if(n_args == 6) {
		width = mp_obj_get_int(args[4]);
		hight = mp_obj_get_int(args[5]);
		ret = Helios_lcd_write_data_by_file_wh(file_name,start_x,start_y,width,hight);
	} else {
		mp_raise_ValueError("wrong number of parameters");
	}

	if(ret != 0) {
		return mp_obj_new_int(-1);
	}
#endif
    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_lcd_show_obj, 4, 6, machine_lcd_show_data);


#ifdef CONFIG_JPEG
STATIC mp_obj_t machine_lcd_show_jpeg(size_t n_args, const mp_obj_t *args)
{
	
	int ret = 0;
	int start_x = 0;
	int start_y = 0;
	int end_x = 0;
	int end_y = 0;

	start_x = mp_obj_get_int(args[2]);
	start_y = mp_obj_get_int(args[3]);

	//machine_lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	char *file_name = (char*)mp_obj_str_get_str(args[1]);
	
	char name[256] = {0};
	sprintf(name, "U:/%s",file_name);

	rgb_struct lcd_data = {0};

	ret = JPEG2RGB565(name,&lcd_data);

	LCD_LOG("JPEG2RGB565 ret = %d\n",ret);

	if(ret != 0) return mp_obj_new_int(ret);

	end_x = start_x + lcd_data.width-1;
	end_y = start_y + lcd_data.height -1;
	
	ret = Helios_LCD_Write(lcd_data.buf, start_x, start_y, end_x, end_y);

	if(lcd_data.buf) {
		free(lcd_data.buf);
	}
	
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_lcd_show_jpeg_obj, 4, 4, machine_lcd_show_jpeg);
#endif


STATIC const mp_rom_map_elem_t lcd_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_lcd_init), MP_ROM_PTR(&machine_lcd_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_lcd_clear), MP_ROM_PTR(&machine_lcd_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_lcd_show), MP_ROM_PTR(&machine_lcd_show_obj) },
    { MP_ROM_QSTR(MP_QSTR_lcd_brightness), MP_ROM_PTR(&machine_lcd_brightness_obj) },
    { MP_ROM_QSTR(MP_QSTR_lcd_write), MP_ROM_PTR(&machine_lcd_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_lcd_display_on), MP_ROM_PTR(&machine_lcd_display_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_lcd_display_off), MP_ROM_PTR(&machine_lcd_display_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_lcd_write_cmd), MP_ROM_PTR(&machine_lcd_write_cmd_obj) },
    { MP_ROM_QSTR(MP_QSTR_lcd_write_data), MP_ROM_PTR(&machine_lcd_write_data_obj) },
#ifdef CONFIG_JPEG
    { MP_ROM_QSTR(MP_QSTR_lcd_show_jpg), MP_ROM_PTR(&machine_lcd_show_jpeg_obj) },
#endif

};

STATIC MP_DEFINE_CONST_DICT(lcd_locals_dict, lcd_locals_dict_table);

STATIC mp_obj_t machine_lcd_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
const mp_obj_type_t machine_lcd_type = {
    { &mp_type_type },
    .name = MP_QSTR_LCD,
    .make_new = machine_lcd_make_new,
    .locals_dict = (mp_obj_t)&lcd_locals_dict,
};
#endif

