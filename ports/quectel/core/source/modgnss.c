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
 * @file    modgnss.c
 * @author  Pawn
 * @version V1.0.0
 * @date    2021/07/09
 * @brief   gnss
 ******************************************************************************
 */


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
 
#include "nlr.h"
#include "objlist.h"
#include "objstr.h"
#include "runtime.h"

#include "helios_debug.h"
#include "helios_gnss.h"
#include "helios_uart.h"
#include "helios_os.h"

#define MOD_GNSS_LOG(msg, ...)      custom_log(GNSS, msg, ##__VA_ARGS__)

// Helios_Event_t Helios_event;

 
 STATIC mp_obj_t mp_gnss_switch(size_t n_args, const mp_obj_t *args)
 {
	 int ret=0;
 
	 int gnss_enable = mp_obj_get_int(args[0]);
	 
	 ret = Helios_Gnss_Switch(gnss_enable);
	 
	 if(ret == HELIOS_GNSS_ALREADY_OPEN)
	 {
		MOD_GNSS_LOG("MP GNSS already open");
		return mp_obj_new_int(0);
	 }
	 return mp_obj_new_int(-1);
 }
 STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_gnss_switch_obj, 1, 2, mp_gnss_switch);
 
 
 STATIC mp_obj_t mp_get_gnss_state(void)
 {
	 int ret=0;
 
	 ret = Helios_Gnss_State_Info_Get();
	 return mp_obj_new_int(ret);
 }
 STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_gnss_state_obj, mp_get_gnss_state);
 
 STATIC int mp_gnss_device_info_request(void)
 {
	 char *send_str=NULL;
	 unsigned char checksum=0;
	 int i=1;	 
	 int send_len=0;
	 int ret=0;
	 if((Helios_Gnss_State_Info_Get()==HELIOS_GNSS_FIRMWARE_UPDATE)||(Helios_Gnss_State_Info_Get()==HELIOS_GNSS_CLOSE))
	 {
		 MOD_GNSS_LOG("gnss dev not ready\r\n");
		 return HELIOS_GNSS_EXECUTE_ERR;
	 }
	 send_str=malloc(HELIOS_GPS_CMD_LEN_MAX);
	 if(NULL==send_str)
	 {
		 MOD_GNSS_LOG("malloc err\r\n");
		 ret=HELIOS_GNSS_EXECUTE_ERR;
		 goto exit;
	 }
	 send_len=snprintf(send_str, HELIOS_GPS_CMD_LEN_MAX, "$PDTINFO,");
	 while(send_len>i)
	 {
		 checksum^=send_str[i];
		 i++;
	 }
	 send_len+=snprintf(send_str+send_len, HELIOS_GPS_CMD_LEN_MAX, "*%x\r\n",checksum);
	 if(Helios_Uart_Write(HELIOS_GNSS_UART,(unsigned char*)send_str,send_len)!=send_len)
	 {
		MOD_GNSS_LOG("gnss device info request fail\r\n");
		ret=HELIOS_GNSS_EXECUTE_ERR;
	 }
 exit:
	 if(NULL != send_str)
	 {
		 free(send_str);
		 send_str=NULL;
	 }
	 return ret;
 }
 
 STATIC mp_obj_t mp_get_gnss_init(void)
 {
	 int ret;
	 ret = Helios_Gnss_Switch(HELIOS_GNSS_ENABLE);
	 if(ret == HELIOS_GNSS_ALREADY_OPEN)
	 {
		return mp_obj_new_int(-1);
	 }
	 while(Helios_Gnss_State_Info_Get()==HELIOS_GNSS_FIRMWARE_UPDATE)
	 {
	   Helios_msleep(1000);
	 }
	 ret = Helios_Gnss_Callback_Register();
	 if(ret == HELIOS_GNSS_CB_NULL_ERR)
	 {
		 return mp_obj_new_int(-1);
	 }
	 mp_gnss_device_info_request();
 
	 return mp_obj_new_int(0);
 }
 STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_get_gnss_init_obj, mp_get_gnss_init);
 
 /*
 STATIC mp_obj_t mp_event_try_wait(void)
 {
	 int ret;
	 ret = Helios_Event_Wait(&mp_event);
	 return mp_obj_new_int(ret);
 }
 STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_event_try_wait_obj, mp_event_try_wait);
 */
 
 STATIC mp_obj_t mp_gnss_nmea_read(size_t n_args, const mp_obj_t *args)
 {
	 int gnss_msg_len;
	 int recbuff_len_max;
	 unsigned char *recbuff=NULL;
	 //QPY_PAWNGNSS_LOG("mp_gnss_nmea_read 1  \r\n");
	 recbuff_len_max = mp_obj_get_int(args[0]);
	 recbuff=calloc(1,recbuff_len_max);
	 memset(recbuff,0,recbuff_len_max);
	 //QPY_PAWNGNSS_LOG("mp_gnss_nmea_read 2  \r\n");
	 if(NULL==recbuff)
	 {
		MOD_GNSS_LOG("malloc err\r\n");
		return mp_obj_new_int(-1);
	 }
	 // QPY_PAWNGNSS_LOG("mp_gnss_nmea_read 3 len: %d\r\n", mp_event.param2);
	 
	 gnss_msg_len = Helios_Gnss_Nmea_Get(HELIOS_UART_PORT_3, recbuff, recbuff_len_max);
	 mp_obj_t tuple[2] = {
		 tuple[0] = mp_obj_new_int(strlen((char *)recbuff)),
		 // tuple[1] = mp_obj_new_str((char *)recbuff, strlen((char *)recbuff)),
		 tuple[1] = mp_obj_new_str((char *)recbuff, gnss_msg_len),
	 };
	 
	 if(NULL != recbuff)
	 {
		 free(recbuff);
		 recbuff=NULL;
	 }
	 
	 return mp_obj_new_tuple(2, tuple);
 }
 STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_gnss_nmea_read_obj, 1, 2, mp_gnss_nmea_read);

 
 STATIC const mp_rom_map_elem_t gnss_module_globals_table[] = {
	 { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_quecgnss) },
	 { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&mp_get_gnss_init_obj) },
	 { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_gnss_nmea_read_obj) },
	 // { MP_ROM_QSTR(MP_QSTR_event_wait), MP_ROM_PTR(&mp_event_try_wait_obj) },
	 { MP_ROM_QSTR(MP_QSTR_get_state), MP_ROM_PTR(&mp_get_gnss_state_obj) },
	 { MP_ROM_QSTR(MP_QSTR_gnssEnable), MP_ROM_PTR(&mp_gnss_switch_obj) },
 };
 
 STATIC MP_DEFINE_CONST_DICT(gnss_module_globals, gnss_module_globals_table);
 
 const mp_obj_module_t mp_module_quecgnss = {
	 .base = { &mp_type_module },
	 .globals = (mp_obj_dict_t *)&gnss_module_globals,
 };

