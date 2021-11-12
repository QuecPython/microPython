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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "nlr.h"
#include "objlist.h"
#include "objstr.h"
#include "runtime.h"
#include "mperrno.h"
#include "helios_debug.h"
#include "helios_ble.h"

#define MOD_BLE_LOG(msg, ...)      custom_log(modble, msg, ##__VA_ARGS__)

static c_callback_t *g_ble_server_callback = NULL;
static c_callback_t *g_ble_client_callback = NULL;


static void qpy_ble_data_print(size_t len, void *data)
{
	unsigned short int i = 0;
	unsigned short int datalen = (unsigned short int)len;
	char src[20] = {0};
	char dest[50] = {0};
	uint8_t *pdata = (uint8_t *)data;

	for (i=0; i<datalen; i++)
	{
		if (i%6 == 0)
		{
			sprintf(src, "[%d-%d]0x%02x", i, i+5, pdata[i]);
			if (i != 0)
			{
				MOD_BLE_LOG("%s", dest);
				memset((void *)dest, 0, sizeof(dest));
			}
		}
		else
		{
			sprintf(src, ",0x%02x", pdata[i]);
		}
		strcat(dest, src);
		memset((void *)src, 0, sizeof(src));
	}
	MOD_BLE_LOG("%s", dest);
}


static void qpy_ble_server_user_callback(void *ind_msg_buf, void *ctx)
{
	Helios_EventId *event = (Helios_EventId *)ind_msg_buf;
	uint32_t event_id = event->id;
	uint32_t status = event->param1;
	uint32_t param2 = event->param2;
	uint32_t param3 = event->param3;

	switch (event_id)
	{
		case HELIOS_BT_START_STATUS_IND:
		case HELIOS_BT_STOP_STATUS_IND:
		case HELIOS_BLE_GATT_SEND_END:
		{
			mp_obj_t tuple[2] = 
			{
				mp_obj_new_int(event_id),
				mp_obj_new_int(status),
			};
			
			if (g_ble_server_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule_ex(g_ble_server_callback, mp_obj_new_tuple(2, tuple));
				MOD_BLE_LOG("[ble] callback end.\r\n");
			}
			break;
		}
		case HELIOS_BLE_CONNECT_IND:
		case HELIOS_BLE_DISCONNECT_IND:
		{
			Helios_BtBleAddr *p = (Helios_BtBleAddr *)param2;
			int conn_id = param3;
			char addr_buf[20] = {0};

			if (p->addr != NULL)
			{
				sprintf(addr_buf, "%02x:%02x:%02x:%02x:%02x:%02x", p->addr[0], p->addr[1], p->addr[2], p->addr[3], p->addr[4], p->addr[5]);
				MOD_BLE_LOG("conn_id:%d\r\n", conn_id);
				MOD_BLE_LOG("addr:%s\r\n", addr_buf);
			}
			else
			{
				MOD_BLE_LOG("addr is NULL.\r\n");
			}

			mp_obj_t tuple[4] = 
			{
				mp_obj_new_int(event_id),
				mp_obj_new_int(status),
				mp_obj_new_int(conn_id),
				mp_obj_new_bytes(p->addr, HELIOS_BT_MAC_ADDRESS_SIZE),
				//mp_obj_new_str(addr_buf, strlen(addr_buf)),
			};
		
			if (g_ble_server_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule_ex(g_ble_server_callback, mp_obj_new_tuple(4, tuple));
				MOD_BLE_LOG("[ble] callback end.\r\n");
			}
			
			break;
		}
		case HELIOS_BLE_UPDATE_CONN_PARAM_IND:
		{
			Helios_BleUpdateConnInfos *conn_param = (Helios_BleUpdateConnInfos *)param2;
			mp_obj_t tuple[7] = 
			{
				mp_obj_new_int(event_id),
				mp_obj_new_int(status),
				mp_obj_new_int(conn_param->conn_id),
				mp_obj_new_int(conn_param->max_interval),
				mp_obj_new_int(conn_param->min_interval),
				mp_obj_new_int(conn_param->latency),
				mp_obj_new_int(conn_param->timeout)
			};
			
			if (g_ble_server_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule_ex(g_ble_server_callback, mp_obj_new_tuple(7, tuple));
				MOD_BLE_LOG("[ble] callback end.\r\n");
			}
			break;
		}
		case HELIOS_BLE_GATT_MTU:
		{
			unsigned short handle = param2;
			unsigned short ble_mtu = param3;

			MOD_BLE_LOG("connect mtu successful: handle=%d, mtu=%d\r\n", param2, param3);
			mp_obj_t tuple[4] = 
			{
				mp_obj_new_int(event_id),
				mp_obj_new_int(status),
				mp_obj_new_int(handle),
				mp_obj_new_int(ble_mtu),
			};
			
			if (g_ble_server_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule_ex(g_ble_server_callback, mp_obj_new_tuple(4, tuple));
				MOD_BLE_LOG("[ble] callback end.\r\n");
			}
			
			break;
		}
		case HELIOS_BLE_GATT_RECV_IND:
		case HELIOS_BLE_GATT_RECV_READ_IND:	
		{
			Helios_BleGattData *ble_data = (Helios_BleGattData *)param2;
			
			mp_obj_t tuple[7] = 
			{
				mp_obj_new_int(event_id),
				mp_obj_new_int(status),
				mp_obj_new_int(ble_data->len),
				mp_obj_new_bytes(ble_data->data, ble_data->len),
				mp_obj_new_int(ble_data->att_handle),
				mp_obj_new_int(ble_data->uuid_s),
				mp_obj_new_bytes(ble_data->uuid_l, sizeof(ble_data->uuid_l))	
			};
			
			if (g_ble_server_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule_ex(g_ble_server_callback, mp_obj_new_tuple(7, tuple));
				MOD_BLE_LOG("[ble] callback end.\r\n");
			}
			
			break;
		}
		default:
			break;
		
	}
}


STATIC mp_obj_t qpy_ble_gatt_start(void)
{
	int ret = 0;
	ret = Helios_BLE_GattStart();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_ble_gatt_start_obj, qpy_ble_gatt_start);


STATIC mp_obj_t qpy_ble_gatt_stop(void)
{
	int ret = 0;
	ret = Helios_BLE_GattStop();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_ble_gatt_stop_obj, qpy_ble_gatt_stop);


STATIC mp_obj_t qpy_ble_get_status(void)
{
	int ret = 0;
	Helios_BtBle_Status status = 0;
	ret = Helios_BLE_GetStatus(&status);
	if (ret == 0)
	{
		return mp_obj_new_int(status);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_ble_get_status_obj, qpy_ble_get_status);

STATIC mp_obj_t qpy_ble_get_public_addr(void)
{
	int ret = 0;
	Helios_BtBleAddr ble_addr = {0};
	ret = Helios_BLE_GetPublic_addr(&ble_addr);
	if (ret == 0)
	{
		return mp_obj_new_bytes(ble_addr.addr, HELIOS_BT_MAC_ADDRESS_SIZE);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_ble_get_public_addr_obj, qpy_ble_get_public_addr);


#if 0
STATIC mp_obj_t qpy_ble_get_connect_status(mp_obj_t addr)
{
	int ret = 0;
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(addr, &datainfo, MP_BUFFER_READ);
	Helios_BtBleAddr bleaddr = {0};

	if (datainfo.len != 6)
	{
		mp_raise_ValueError("invalid value, addr should be 6 bytes.");
	}
	
	memcpy((void *)bleaddr.addr, (const void *)datainfo.buf, datainfo.len);
	Helios_BleConnectionStatus status = 0;
	ret = Helios_BLE_GetConnectionStatus(&bleaddr, &status);
	if (ret == 0)
	{
		mp_obj_new_int(status);
	}
	return mp_obj_new_int(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ble_get_conn_status_obj, qpy_ble_get_connect_status);
#endif


STATIC mp_obj_t qpy_ble_gatt_set_local_name(mp_obj_t code, mp_obj_t name)
{
	int ret = 0;
	int code_type = mp_obj_get_int(code);
	Helios_BtBleLocalName name_info = {0};
	mp_buffer_info_t nameinfo = {0};
	mp_get_buffer_raise(name, &nameinfo, MP_BUFFER_READ);

	if ((code_type != 0) && (code_type != 1))
	{
		mp_raise_ValueError("invalid value, code should be 0-UTF8 or 1-GBK.");
	}
	if ((nameinfo.len < 1) || (nameinfo.len > 29))
	{
		mp_raise_ValueError("invalid value, the length of name should be in [1,29] bytes.");
	}
	name_info.code_type = code_type;
	strncpy((char *)name_info.name, (const char *)nameinfo.buf, nameinfo.len);
	
	ret = Helios_BLE_GattSetLocalName(&name_info);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_ble_gatt_set_local_name_obj, qpy_ble_gatt_set_local_name);


STATIC mp_obj_t qpy_ble_gatt_server_init(mp_obj_t callback)
{
	Helios_BLEInitStruct info = {0};
	static c_callback_t cb = {0};
    memset(&cb, 0, sizeof(c_callback_t));
	g_ble_server_callback = &cb;
	mp_sched_schedule_callback_register(g_ble_server_callback, callback);
	
	info.ble_user_cb = qpy_ble_server_user_callback;
	int ret = Helios_BLE_GattServerInit(&info);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ble_gatt_server_init_obj, qpy_ble_gatt_server_init);


STATIC mp_obj_t qpy_ble_gatt_server_release(void)
{
	int ret = 0;
	g_ble_server_callback = NULL;
	ret = Helios_BLE_GattServerRelease();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_ble_gatt_server_release_obj, qpy_ble_gatt_server_release);


STATIC mp_obj_t qpy_ble_set_adv_param(size_t n_args, const mp_obj_t *args)
{
	int min_adv = mp_obj_get_int(args[0]);
	int max_adv = mp_obj_get_int(args[1]);
	int adv_type = mp_obj_get_int(args[2]);
	int addr_type = mp_obj_get_int(args[3]);
	int channel_map = mp_obj_get_int(args[4]);
	int filter = mp_obj_get_int(args[5]);
	int discov_mode = mp_obj_get_int(args[6]);
	int no_br_edr = mp_obj_get_int(args[7]);
	int enable_adv = mp_obj_get_int(args[8]);
	Helios_BleAdvParam adv_param = {0};

	if ((min_adv < 0x0020) || (min_adv > 0x4000) || (max_adv < 0x0020) || (max_adv > 0x4000))
	{
		mp_raise_ValueError("invalid value, min/max_adv should be in [0x0020,0x4000].");
	}
	if ((adv_type < 0) || (adv_type > 4))
	{
		mp_raise_ValueError("invalid value, adv_type should be in [0,4].");
	}
	if ((addr_type != 0) && (addr_type != 1))
	{
		mp_raise_ValueError("invalid value, addr_type should be in [0,1].");
	}
	if ((channel_map != 1) && (channel_map != 2) && (channel_map != 4) && (channel_map != 7))
	{
		mp_raise_ValueError("invalid value, channel_map should be 1,2,4,7.");
	}
	if ((filter < 0) || (filter > 3))
	{
		mp_raise_ValueError("invalid value, filter_policy should be in [0,3].");
	}
	if ((discov_mode < 0) || (discov_mode > 255))
	{
		mp_raise_ValueError("invalid value, discov_mode should be in [0,255].");
	}
	if ((no_br_edr != 0) && (no_br_edr != 1))
	{
		mp_raise_ValueError("invalid value, no_br_edr should be in [0,1].");
	}
	if ((enable_adv != 0) && (enable_adv != 1))
	{
		mp_raise_ValueError("invalid value, enable_adv should be in [0,1].");
	}

	adv_param.max_adv = max_adv;
	adv_param.min_adv = min_adv;
	adv_param.adv_type = adv_type;
	adv_param.own_addr_type = addr_type;
	adv_param.channel_map = channel_map;
	adv_param.filter = filter;
	adv_param.discov_mode = discov_mode;
	adv_param.no_br_edr = no_br_edr;
	adv_param.enable_adv = enable_adv;

	int ret = Helios_BLE_AdvSetParam(&adv_param);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_set_adv_param_obj, 9, 9, qpy_ble_set_adv_param);


STATIC mp_obj_t qpy_ble_set_adv_data(mp_obj_t data)
{
	int ret = 0;
	Helios_BleAdvSetData adv_data = {0};
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(data, &datainfo, MP_BUFFER_READ);

	if (datainfo.len < 1 || datainfo.len >31)
	{
		mp_raise_ValueError("invalid value, the length of data should be no more than 31 bytes.");
	}

	adv_data.date_len = datainfo.len;
	memcpy((void *)adv_data.data, (const void *)datainfo.buf, datainfo.len);
	qpy_ble_data_print(datainfo.len, (void *)adv_data.data);
	
	ret = Helios_BLE_AdvSetData(&adv_data);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ble_set_adv_data_obj, qpy_ble_set_adv_data);


STATIC mp_obj_t qpy_ble_set_adv_rsp_data(mp_obj_t data)
{
	int ret = 0;
	Helios_BleAdvSetData adv_data = {0};
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(data, &datainfo, MP_BUFFER_READ);

	if (datainfo.len < 1 || datainfo.len >31)
	{
		mp_raise_ValueError("invalid value, the length of data should be no more than 31 bytes.");
	}

	adv_data.date_len = datainfo.len;
	memcpy((void *)adv_data.data, (const void *)datainfo.buf, datainfo.len);
	qpy_ble_data_print(datainfo.len, (void *)adv_data.data);
	
	ret = Helios_BLE_AdvSetScanRspData(&adv_data);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ble_set_adv_rsp_data_obj, qpy_ble_set_adv_rsp_data);


STATIC mp_obj_t qpy_ble_gatt_add_service(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	int primary  = mp_obj_get_int(args[0]);
	int server_id = mp_obj_get_int(args[1]);
	int uuid_type = mp_obj_get_int(args[2]);
	int uuid_s = mp_obj_get_int(args[3]);
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(args[4], &datainfo, MP_BUFFER_READ);
	Helios_BleGattUuid uuid = {0};

	if ((uuid_type != 0) && (uuid_type != 1))
	{
		mp_raise_ValueError("invalid value, uuid_type should be in [0,1].");
	}
	if (datainfo.len > 16)
	{
		mp_raise_ValueError("invalid value, the length of data should be no more than 16 bytes.");
	}

	uuid.uuid_type = uuid_type;
	uuid.uuid_s = (uint16_t)uuid_s;
	memcpy((void *)uuid.uuid_l, (const void *)datainfo.buf, datainfo.len);
	qpy_ble_data_print(datainfo.len, (void *)uuid.uuid_l);
	ret = Helios_BLE_GattAddService((uint16_t)server_id, &uuid, (uint8_t)primary);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_add_service_obj, 5, 5, qpy_ble_gatt_add_service);


STATIC mp_obj_t qpy_ble_gatt_add_chara(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	int server_id  = mp_obj_get_int(args[0]);
	int chara_id   = mp_obj_get_int(args[1]);
	int chara_prop  = mp_obj_get_int(args[2]);
	int uuid_type  = mp_obj_get_int(args[3]);
	int uuid_s 	   = mp_obj_get_int(args[4]);
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(args[5], &datainfo, MP_BUFFER_READ);
	Helios_BleGattUuid uuid = {0};

	if ((chara_prop < 0x00) || (chara_prop > 0xFF))
	{
		mp_raise_ValueError("invalid value, char_prop should be in [0x00,0xFF].");
	}
	if ((uuid_type != 0) && (uuid_type != 1))
	{
		mp_raise_ValueError("invalid value, uuid_type should be in [0,1].");
	}
	if (datainfo.len > 16)
	{
		mp_raise_ValueError("invalid value, the length of long uuid should be no more than 16 bytes.");
	}

	uuid.uuid_type = uuid_type;
	uuid.uuid_s = (uint16_t)uuid_s;
	memcpy((void *)uuid.uuid_l, (const void *)datainfo.buf, datainfo.len);
	qpy_ble_data_print(datainfo.len, (void *)uuid.uuid_l);
	ret = Helios_BLE_GattAddChara((uint16_t)server_id, (uint16_t)chara_id, (uint8_t)chara_prop, &uuid);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_add_chara_obj, 6, 6, qpy_ble_gatt_add_chara);


STATIC mp_obj_t qpy_ble_gatt_add_chara_value(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	int server_id  = mp_obj_get_int(args[0]);
	int chara_id   = mp_obj_get_int(args[1]);
	int permission  = mp_obj_get_int(args[2]);
	int uuid_type  = mp_obj_get_int(args[3]);
	int uuid_s 	   = mp_obj_get_int(args[4]);
	mp_buffer_info_t uuidinfo = {0};
	mp_buffer_info_t valueinfo = {0};
	mp_get_buffer_raise(args[5], &uuidinfo, MP_BUFFER_READ);
	mp_get_buffer_raise(args[6], &valueinfo, MP_BUFFER_READ);
	Helios_BleGattUuid uuid = {0};

	if ((permission < 0x0000) || (permission > 0x0FFF))
	{
		mp_raise_ValueError("invalid value, permission should be in [0x0000,0x0FFF].");
	}
	if ((uuid_type != 0) && (uuid_type != 1))
	{
		mp_raise_ValueError("invalid value, uuid_type should be in [0,1].");
	}
	if (uuidinfo.len > 16)
	{
		mp_raise_ValueError("invalid value, the length of long uuid should be no more than 16 bytes.");
	}

	uint16_t value_len = valueinfo.len;
	uuid.uuid_type = uuid_type;
	uuid.uuid_s = (uint16_t)uuid_s;
	memcpy((void *)uuid.uuid_l, (const void *)uuidinfo.buf, uuidinfo.len);
	MOD_BLE_LOG("long uuid:");
	qpy_ble_data_print(uuidinfo.len, (void *)uuid.uuid_l);
	MOD_BLE_LOG("characteristic value:");
	qpy_ble_data_print(valueinfo.len, (void *)valueinfo.buf);
	ret = Helios_BLE_GattAddCharaValue((uint16_t)server_id, (uint16_t)chara_id, (uint16_t)permission, &uuid, value_len, (uint8_t *)valueinfo.buf);
	
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_add_chara_value_obj, 7, 7, qpy_ble_gatt_add_chara_value);


STATIC mp_obj_t qpy_ble_gatt_add_chara_desc(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	int server_id  = mp_obj_get_int(args[0]);
	int chara_id   = mp_obj_get_int(args[1]);
	int permission  = mp_obj_get_int(args[2]);
	int uuid_type  = mp_obj_get_int(args[3]);
	int uuid_s 	   = mp_obj_get_int(args[4]);
	mp_buffer_info_t uuidinfo = {0};
	mp_buffer_info_t valueinfo = {0};
	mp_get_buffer_raise(args[5], &uuidinfo, MP_BUFFER_READ);
	mp_get_buffer_raise(args[6], &valueinfo, MP_BUFFER_READ);
	Helios_BleGattUuid uuid = {0};

	if ((permission < 0x0000) || (permission > 0x0FFF))
	{
		mp_raise_ValueError("invalid value, permission should be in [0x0000,0x0FFF].");
	}
	if ((uuid_type != 0) && (uuid_type != 1))
	{
		mp_raise_ValueError("invalid value, uuid_type should be in [0,1].");
	}
	if (uuidinfo.len > 16)
	{
		mp_raise_ValueError("invalid value, the length of long uuid should be no more than 16 bytes.");
	}

	uint16_t value_len = valueinfo.len;
	uuid.uuid_type = uuid_type;
	uuid.uuid_s = (uint16_t)uuid_s;
	memcpy((void *)uuid.uuid_l, (const void *)uuidinfo.buf, uuidinfo.len);
	MOD_BLE_LOG("long uuid:");
	qpy_ble_data_print(uuidinfo.len, (void *)uuid.uuid_l);
	MOD_BLE_LOG("characteristic description value:");
	qpy_ble_data_print(valueinfo.len, (void *)valueinfo.buf);
	ret = Helios_BLE_GattAddCharaDesc((uint16_t)server_id, (uint16_t)chara_id, (uint16_t)permission, &uuid, value_len, (uint8_t *)valueinfo.buf);
	
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_add_chara_desc_obj, 7, 7, qpy_ble_gatt_add_chara_desc);


STATIC mp_obj_t qpy_ble_gatt_add_or_clear_service(mp_obj_t type, mp_obj_t mode)
{
	int ret = 0;
	uint16_t opt = (uint16_t)mp_obj_get_int(type);
	Helios_BleSysServiceMode sys_mode = (Helios_BleSysServiceMode)mp_obj_get_int(mode);
	
	if ((opt != 0) && (opt != 1))
	{
		mp_raise_ValueError("invalid value, option should be in [0,1].");
	}
	if ((sys_mode != 0) && (sys_mode != 1))
	{
		mp_raise_ValueError("invalid value, mode should be in [0,1].");
	} 
	
	ret = Helios_BLE_GattAddOrClearServiceComplete(opt, sys_mode);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_ble_gatt_add_or_clear_service_obj, qpy_ble_gatt_add_or_clear_service);


STATIC mp_obj_t qpy_ble_send_notification(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	int conn_id  = mp_obj_get_int(args[0]);
	int attr_handle = mp_obj_get_int(args[1]);
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(args[2], &datainfo, MP_BUFFER_READ);
	uint16_t length = datainfo.len;
	Helios_BleGattNotInd type = HELIOS_BLE_GATT_NOTIFY;

	ret = Helios_BLE_SendData((uint16_t)conn_id, (uint16_t)attr_handle, type, length, (uint8_t *)datainfo.buf);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_send_notification_obj, 3, 3, qpy_ble_send_notification);


STATIC mp_obj_t qpy_ble_send_indication(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	int conn_id  = mp_obj_get_int(args[0]);
	int attr_handle = mp_obj_get_int(args[1]);
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(args[2], &datainfo, MP_BUFFER_READ);
	uint16_t length = datainfo.len;
	Helios_BleGattNotInd type = HELIOS_BLE_GATT_INDICATION;

	ret = Helios_BLE_SendData((uint16_t)conn_id, (uint16_t)attr_handle, type, length, (uint8_t *)datainfo.buf);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_send_indication_obj, 3, 3, qpy_ble_send_indication);


STATIC mp_obj_t qpy_ble_adv_start(void)
{
	int ret = 0;
	ret = Helios_BLE_AdvStart();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_ble_adv_start_obj, qpy_ble_adv_start);


STATIC mp_obj_t qpy_ble_adv_stop(void)
{
	int ret = 0;
	ret = Helios_BLE_AdvStop();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_ble_adv_stop_obj, qpy_ble_adv_stop);

static void qpy_ble_client_user_callback(void *ind_msg_buf, void *ctx)
{
	Helios_EventId *event = (Helios_EventId *)ind_msg_buf;
	uint32_t event_id = event->id;
	uint32_t status = event->param1;
	uint32_t param2 = event->param2;
	uint32_t param3 = event->param3;

	switch (event_id)
	{
		case HELIOS_BT_START_STATUS_IND:
		case HELIOS_BT_STOP_STATUS_IND:
		case HELIOS_BLE_GATT_START_DISCOVER_SERVICE_IND:
		case HELIOS_BLE_GATT_CHARA_WRITE_WITH_RSP_IND:
		case HELIOS_BLE_GATT_DESC_WRITE_WITH_RSP_IND:
		case HELIOS_BLE_GATT_CHARA_WRITE_WITHOUT_RSP_IND:
		{
			mp_obj_t tuple[2] = 
			{
				mp_obj_new_int(event_id),
				mp_obj_new_int(status),
			};
			
			if (g_ble_client_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule_ex(g_ble_client_callback, mp_obj_new_tuple(2, tuple));
				MOD_BLE_LOG("[ble] callback end.\r\n");
			}
			break;
		}
		case HELIOS_BLE_SCAN_REPORT_IND:
		{
			Helios_BleScanReportInfo *pinfo = (Helios_BleScanReportInfo *)param2;
			char addr_buf[20] = {0};

			sprintf(addr_buf, "%02x:%02x:%02x:%02x:%02x:%02x", \
				pinfo->addr.addr[0], pinfo->addr.addr[1], pinfo->addr.addr[2], \
				pinfo->addr.addr[3], pinfo->addr.addr[4], pinfo->addr.addr[5]);
			MOD_BLE_LOG("addr:%s\r\n", addr_buf);
			
			
			mp_obj_t tuple[9] = 
			{
				mp_obj_new_int(event_id),
				mp_obj_new_int(status),
				mp_obj_new_int(pinfo->event_type),
				mp_obj_new_str((const char *)pinfo->name, pinfo->name_length),
				mp_obj_new_int(pinfo->addr_type),
				mp_obj_new_bytes(pinfo->addr.addr, HELIOS_BT_MAC_ADDRESS_SIZE),
				//mp_obj_new_str(addr_buf, strlen(addr_buf)),
				mp_obj_new_int(pinfo->rssi),
				mp_obj_new_int(pinfo->data_length),
				mp_obj_new_bytes(pinfo->raw_data, pinfo->data_length)
			};
		
			if (g_ble_client_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule_ex(g_ble_client_callback, mp_obj_new_tuple(9, tuple));
				MOD_BLE_LOG("[ble] callback end.\r\n");
			}
			break;
		}
		case HELIOS_BLE_CONNECT_IND:
		case HELIOS_BLE_DISCONNECT_IND:
		{
			Helios_BtBleAddr *p = (Helios_BtBleAddr *)param2;
			int conn_id = param3;
			char addr_buf[20] = {0};

			sprintf(addr_buf, "%02x:%02x:%02x:%02x:%02x:%02x", p->addr[0], p->addr[1], p->addr[2], p->addr[3], p->addr[4], p->addr[5]);
			MOD_BLE_LOG("conn_id:%d\r\n", conn_id);
			MOD_BLE_LOG("addr:%s\r\n", addr_buf);

			mp_obj_t tuple[4] = 
			{
				mp_obj_new_int(event_id),
				mp_obj_new_int(status),
				mp_obj_new_int(conn_id),
				mp_obj_new_bytes(p->addr, HELIOS_BT_MAC_ADDRESS_SIZE),
				//mp_obj_new_str(addr_buf, strlen(addr_buf)),
			};
		
			if (g_ble_client_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule_ex(g_ble_client_callback, mp_obj_new_tuple(4, tuple));
				MOD_BLE_LOG("[ble] callback end.\r\n");
			}
			break;
		}
		case HELIOS_BLE_UPDATE_CONN_PARAM_IND:
		{
			Helios_BleUpdateConnInfos *conn_param = (Helios_BleUpdateConnInfos *)param2;
			mp_obj_t tuple[7] = 
			{
				mp_obj_new_int(event_id),
				mp_obj_new_int(status),
				mp_obj_new_int(conn_param->conn_id),
				mp_obj_new_int(conn_param->max_interval),
				mp_obj_new_int(conn_param->min_interval),
				mp_obj_new_int(conn_param->latency),
				mp_obj_new_int(conn_param->timeout)
			};
			
			if (g_ble_client_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule_ex(g_ble_client_callback, mp_obj_new_tuple(7, tuple));
				MOD_BLE_LOG("[ble] callback end.\r\n");
			}
			break;
		}
		case HELIOS_BLE_GATT_MTU:
		{
			unsigned short handle = param2;
			unsigned short ble_mtu = param3;

			MOD_BLE_LOG("connect mtu successful: handle=%d, mtu=%d\r\n", param2, param3);
			mp_obj_t tuple[4] = 
			{
				mp_obj_new_int(event_id),
				mp_obj_new_int(status),
				mp_obj_new_int(handle),
				mp_obj_new_int(ble_mtu),
			};
			
			if (g_ble_client_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule_ex(g_ble_client_callback, mp_obj_new_tuple(4, tuple));
				MOD_BLE_LOG("[ble] callback end.\r\n");
			}
			break;
		}
		case HELIOS_BLE_GATT_DISCOVER_SERVICE_IND:
		{
			Helios_BleGattPrimeService *pinfo = (Helios_BleGattPrimeService *)param2;
			mp_obj_t tuple[5] = 
			{
				mp_obj_new_int(event_id),
				mp_obj_new_int(status),
				mp_obj_new_int(pinfo->start_handle),
				mp_obj_new_int(pinfo->end_handle),
				mp_obj_new_int(pinfo->uuid),
			};
			
			if (g_ble_client_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule_ex(g_ble_client_callback, mp_obj_new_tuple(5, tuple));
				MOD_BLE_LOG("[ble] callback end.\r\n");
			}
			break;
		}
		case HELIOS_BLE_GATT_DISCOVER_CHARACTERISTIC_DATA_IND:
		case HELIOS_BLE_GATT_DISCOVER_CHARA_DESC_IND:
		case HELIOS_BLE_GATT_CHARA_READ_IND:
		case HELIOS_BLE_GATT_DESC_READ_IND:
		case HELIOS_BLE_GATT_CHARA_READ_BY_UUID_IND:
		case HELIOS_BLE_GATT_CHARA_MULTI_READ_IND:
		case HELIOS_BLE_GATT_RECV_NOTIFICATION_IND:
		case HELIOS_BLE_GATT_RECV_INDICATION_IND:
		{
			Helios_AttGeneralRsp *pinfo = (Helios_AttGeneralRsp *)param2;
			mp_obj_t tuple[4] = 
			{
				mp_obj_new_int(event_id),
				mp_obj_new_int(status),
				mp_obj_new_int(pinfo->length),
				mp_obj_new_bytes(pinfo->pay_load, pinfo->length)
			};
			
			if (g_ble_client_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule_ex(g_ble_client_callback, mp_obj_new_tuple(4, tuple));
				MOD_BLE_LOG("[ble] callback end.\r\n");
			}
			free(pinfo->pay_load);
			break;
		}
		case HELIOS_BLE_GATT_ATT_ERROR_IND:
		{
			uint8_t errcode = param2;
			mp_obj_t tuple[3] = 
			{
				mp_obj_new_int(event_id),
				mp_obj_new_int(status),
				mp_obj_new_int(errcode)
			};
			
			if (g_ble_client_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule_ex(g_ble_client_callback, mp_obj_new_tuple(3, tuple));
				MOD_BLE_LOG("[ble] callback end.\r\n");
			}
			break;
		}
		default:
			break;
	}
	
}

STATIC mp_obj_t qpy_ble_gatt_client_init(mp_obj_t callback)
{
	Helios_BLEInitStruct info = {0};
	static c_callback_t cb = {0};
    memset(&cb, 0, sizeof(c_callback_t));
	g_ble_client_callback = &cb;
	mp_sched_schedule_callback_register(g_ble_client_callback, callback);
	
	info.ble_user_cb = qpy_ble_client_user_callback;
	int ret = Helios_BLE_GattClientInit(&info);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ble_gatt_client_init_obj, qpy_ble_gatt_client_init);


STATIC mp_obj_t qpy_ble_gatt_client_release(void)
{
	int ret = 0;
	g_ble_client_callback = NULL;
	ret = Helios_BLE_GattClientRelease();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_ble_gatt_client_release_obj, qpy_ble_gatt_client_release);


STATIC mp_obj_t qpy_ble_set_scan_param(size_t n_args, const mp_obj_t *args)
{
	int scan_mode = mp_obj_get_int(args[0]);
	int interval  = mp_obj_get_int(args[1]);
	int scan_time = mp_obj_get_int(args[2]);
	int filter    = mp_obj_get_int(args[3]);
	int addr_type = mp_obj_get_int(args[4]);
	
	Helios_BleScanParam scan_param = {0};

	if ((scan_mode != 0) && (scan_mode != 1))
	{
		mp_raise_ValueError("invalid value, scan_mode should be in [0,1].");
	}
	if ((interval < 0x0004) || (interval > 0x4000))
	{
		mp_raise_ValueError("invalid value, interval should be in [0x0004,0x4000].");
	}
	if ((scan_time < 0x0004) || (scan_time > 0x4000))
	{
		mp_raise_ValueError("invalid value, scan_time should be in [0x0004,0x4000].");
	}
	if ((filter < 0) || (filter > 3))
	{
		mp_raise_ValueError("invalid value, filter_policy should be in [0,3].");
	}
	if ((addr_type != 0) && (addr_type != 1))
	{
		mp_raise_ValueError("invalid value, addr_type should be in [0,1].");
	}

	scan_param.scan_mode = scan_mode;
	scan_param.interval  = interval;
	scan_param.window    = scan_time;
	scan_param.filter    = filter;
	scan_param.own_addr_type = addr_type;

	int ret = Helios_BLE_ScanSetParam(&scan_param);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_set_scan_param_obj, 5, 5, qpy_ble_set_scan_param);


STATIC mp_obj_t qpy_ble_scan_start(void)
{
	int ret = 0;
	ret = Helios_BLE_ScanStart();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_ble_scan_start_obj, qpy_ble_scan_start);


STATIC mp_obj_t qpy_ble_scan_stop(void)
{
	int ret = 0;
	ret = Helios_BLE_ScanStop();
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_ble_scan_stop_obj, qpy_ble_scan_stop);


STATIC mp_obj_t qpy_ble_connect_addr(mp_obj_t addr_type, mp_obj_t addr)
{
	int addrtype = mp_obj_get_int(addr_type);
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(addr, &datainfo, MP_BUFFER_READ);
	Helios_BtBleAddr bleaddr = {0};

	if ((addrtype != 0) && (addrtype != 1))
	{
		mp_raise_ValueError("invalid value, addr_type should be in [0,1].");
	}
	if (datainfo.len != 6)
	{
		mp_raise_ValueError("invalid value, addr should be 6 bytes.");
	}
	Helios_BleAddressType atype = addrtype;
	memcpy((void *)bleaddr.addr, (const void *)datainfo.buf, datainfo.len);
	int ret = Helios_BLE_ConnectAddr(atype, &bleaddr);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(qpy_ble_connect_addr_obj, qpy_ble_connect_addr);


STATIC mp_obj_t qpy_ble_cancel_connect(mp_obj_t addr)
{
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(addr, &datainfo, MP_BUFFER_READ);
	Helios_BtBleAddr bleaddr = {0};

	if (datainfo.len != 6)
	{
		mp_raise_ValueError("invalid value, addr should be 6 bytes.");
	}
	
	memcpy((void *)bleaddr.addr, (const void *)datainfo.buf, datainfo.len);
	int ret = Helios_BLE_CancelConnect(&bleaddr);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ble_cancel_connect_obj, qpy_ble_cancel_connect);


STATIC mp_obj_t qpy_ble_disconnect(mp_obj_t connect_id)
{
	uint16_t conn_id = mp_obj_get_int(connect_id);
	int ret = Helios_BLE_Disconnect(conn_id);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ble_disconnect_obj, qpy_ble_disconnect);


STATIC mp_obj_t qpy_ble_gatt_discover_all_service(mp_obj_t connect_id)
{
	uint16_t conn_id = mp_obj_get_int(connect_id);
	int ret = Helios_BLE_GattDiscoverAllService(conn_id);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ble_gatt_discover_service_obj, qpy_ble_gatt_discover_all_service);


STATIC mp_obj_t qpy_ble_gatt_discover_by_uuid(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	int conn_id   = mp_obj_get_int(args[0]);
	int uuid_type = mp_obj_get_int(args[1]);
	int uuid_s    = mp_obj_get_int(args[2]);
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(args[3], &datainfo, MP_BUFFER_READ);
	Helios_BleGattUuid uuid = {0};

	if ((uuid_type != 0) && (uuid_type != 1))
	{
		mp_raise_ValueError("invalid value, uuid_type should be in [0,1].");
	}
	if (datainfo.len > 16)
	{
		mp_raise_ValueError("invalid value, the length of data should be no more than 16 bytes.");
	}

	uint16_t connect_id = conn_id;
	uuid.uuid_type = uuid_type;
	uuid.uuid_s = uuid_s;
	memcpy((void *)uuid.uuid_l, (const void *)datainfo.buf, datainfo.len);
	qpy_ble_data_print(datainfo.len, (void *)uuid.uuid_l);
	ret = Helios_BLE_GattDiscoverByUuid(connect_id, &uuid);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_discover_by_uuid_obj, 4, 4, qpy_ble_gatt_discover_by_uuid);


STATIC mp_obj_t qpy_ble_gatt_discover_all_includes(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	int conn_id   = mp_obj_get_int(args[0]);
	int start_handle  = mp_obj_get_int(args[1]);
	int end_handle    = mp_obj_get_int(args[2]);
	Helios_BleHandle handle = {0};

	uint16_t connect_id = conn_id;
	handle.start_handle = start_handle;
	handle.end_handle   = end_handle;
	ret = Helios_BLE_GattDiscoverAllIncludes(connect_id, &handle);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_discover_all_includes_obj, 3, 3, qpy_ble_gatt_discover_all_includes);


STATIC mp_obj_t qpy_ble_gatt_discover_all_chara(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	int conn_id   = mp_obj_get_int(args[0]);
	int start_handle  = mp_obj_get_int(args[1]);
	int end_handle    = mp_obj_get_int(args[2]);
	Helios_BleHandle handle = {0};

	uint16_t connect_id = conn_id;
	handle.start_handle = start_handle;
	handle.end_handle   = end_handle;
	ret = Helios_BLE_GattDiscoverAllChara(connect_id, &handle);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_discover_all_chara_obj, 3, 3, qpy_ble_gatt_discover_all_chara);


STATIC mp_obj_t qpy_ble_gatt_discover_all_chara_desc(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	int conn_id   = mp_obj_get_int(args[0]);
	int start_handle  = mp_obj_get_int(args[1]);
	int end_handle    = mp_obj_get_int(args[2]);
	Helios_BleHandle handle = {0};

	uint16_t connect_id = conn_id;
	handle.start_handle = start_handle;
	handle.end_handle   = end_handle;
	ret = Helios_BLE_GattDiscoverAllCharaDesc(connect_id, &handle);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_discover_all_charadesc_obj, 3, 3, qpy_ble_gatt_discover_all_chara_desc);


STATIC mp_obj_t qpy_ble_gatt_read_chara_by_uuid(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	int conn_id   = mp_obj_get_int(args[0]);
	int start_handle  = mp_obj_get_int(args[1]);
	int end_handle    = mp_obj_get_int(args[2]);
	int uuid_type = mp_obj_get_int(args[3]);
	int uuid_s    = mp_obj_get_int(args[4]);
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(args[5], &datainfo, MP_BUFFER_READ);
	Helios_BleGattUuid uuid = {0};
	Helios_BleHandle handle = {0};

	if ((uuid_type != 0) && (uuid_type != 1))
	{
		mp_raise_ValueError("invalid value, uuid_type should be in [0,1].");
	}
	if (datainfo.len > 16)
	{
		mp_raise_ValueError("invalid value, the length of data should be no more than 16 bytes.");
	}

	uint16_t connect_id = conn_id;
	handle.start_handle = start_handle;
	handle.end_handle   = end_handle;
	uuid.uuid_type = uuid_type;
	uuid.uuid_s = uuid_s;
	memcpy((void *)uuid.uuid_l, (const void *)datainfo.buf, datainfo.len);
	qpy_ble_data_print(datainfo.len, (void *)uuid.uuid_l);
	ret = Helios_BLE_GattReadCharaByUuid(connect_id, &handle, &uuid);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_read_chara_by_uuid_obj, 6, 6, qpy_ble_gatt_read_chara_by_uuid);


STATIC mp_obj_t qpy_ble_gatt_read_chara_by_handle(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	uint16_t conn_id = mp_obj_get_int(args[0]);
	uint16_t handle  = mp_obj_get_int(args[1]);
	uint16_t offset  = mp_obj_get_int(args[2]);
	uint8_t islong   = mp_obj_get_int(args[3]);
	
	ret = Helios_BLE_GattReadCharaByHandle(conn_id, handle, offset, islong);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_read_chara_by_handle_obj, 4, 4, qpy_ble_gatt_read_chara_by_handle);


STATIC mp_obj_t qpy_ble_gatt_read_multi_chara(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	uint16_t conn_id = mp_obj_get_int(args[0]);
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(args[1], &datainfo, MP_BUFFER_READ);

	uint8_t *handle = datainfo.buf;
	uint16_t length = datainfo.len;
	
	ret = Helios_BLE_GattReadMultiChara(conn_id, handle, length);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_read_multi_chara_obj, 2, 2, qpy_ble_gatt_read_multi_chara);


STATIC mp_obj_t qpy_ble_gatt_read_chara_desc(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	uint16_t conn_id = mp_obj_get_int(args[0]);
	uint16_t handle  = mp_obj_get_int(args[1]);
	uint8_t islong   = mp_obj_get_int(args[2]);
	
	ret = Helios_BLE_GattReadCharaDesc(conn_id, handle, islong);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_read_chara_desc_obj, 3, 3, qpy_ble_gatt_read_chara_desc);


STATIC mp_obj_t qpy_ble_gatt_write_chara(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	uint16_t conn_id = mp_obj_get_int(args[0]);
	uint16_t handle  = mp_obj_get_int(args[1]);
	uint16_t offset  = mp_obj_get_int(args[2]);
	uint8_t  islong  = mp_obj_get_int(args[3]);
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(args[4], &datainfo, MP_BUFFER_READ);
	Helios_BleCharaData info = {0};

	info.offset = offset;
	info.islong = islong;
	info.chara.data = datainfo.buf;
	info.chara.length = datainfo.len;
	ret = Helios_BLE_GattWriteChara(conn_id, handle, &info);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_write_chara_obj, 5, 5, qpy_ble_gatt_write_chara);


STATIC mp_obj_t qpy_ble_gatt_write_chara_desc(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	uint16_t conn_id = mp_obj_get_int(args[0]);
	uint16_t handle  = mp_obj_get_int(args[1]);
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(args[2], &datainfo, MP_BUFFER_READ);
	Helios_BleGeneralData info = {0};

	info.data = datainfo.buf;
	info.length = datainfo.len;
	ret = Helios_BLE_GattWriteCharaDesc(conn_id, handle, &info);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_write_chara_desc_obj, 3, 3, qpy_ble_gatt_write_chara_desc);


STATIC mp_obj_t qpy_ble_gatt_write_chara_no_rsp(size_t n_args, const mp_obj_t *args)
{
	int ret = 0;
	uint16_t conn_id = mp_obj_get_int(args[0]);
	uint16_t handle  = mp_obj_get_int(args[1]);
	mp_buffer_info_t datainfo = {0};
	mp_get_buffer_raise(args[2], &datainfo, MP_BUFFER_READ);
	Helios_BleGeneralData info = {0};

	info.data = datainfo.buf;
	info.length = datainfo.len;
	ret = Helios_BLE_GattWriteCharaNoRsp(conn_id, handle, &info);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(qpy_ble_gatt_write_chara_norsp_obj, 3, 3, qpy_ble_gatt_write_chara_no_rsp);

STATIC mp_obj_t qpy_ble_gatt_set_scanreport_filter(mp_obj_t act)
{
	uint16_t switch_filter = mp_obj_get_int(act);
	if ((switch_filter != 0) && (switch_filter != 1))
	{
		mp_raise_ValueError("invalid value, act should be in [0,1].");
	}
	int ret = Helios_BLE_SetScanReportFilter(switch_filter);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ble_set_scanreport_filter_obj, qpy_ble_gatt_set_scanreport_filter);

STATIC mp_obj_t qpy_module_ble_deinit(void)
{
	MOD_BLE_LOG("ble module deinit.\r\n");
	if (g_ble_server_callback != NULL)
	{
		MOD_BLE_LOG("ble server deinit.\r\n");
		Helios_BLE_GattStop();
		Helios_BLE_GattServerRelease();
		g_ble_server_callback = NULL;
	}
	if (g_ble_client_callback != NULL)
	{
		MOD_BLE_LOG("ble client deinit.\r\n");
		Helios_BLE_GattStop();
		Helios_BLE_GattClientRelease();
		g_ble_client_callback = NULL;
	}
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(qpy_module_ble_deinit_obj, qpy_module_ble_deinit);


STATIC const mp_rom_map_elem_t mp_module_ble_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__),			MP_ROM_QSTR(MP_QSTR_ble) 							},
	{ MP_ROM_QSTR(MP_QSTR___qpy_module_deinit__),			MP_ROM_PTR(&qpy_module_ble_deinit_obj) 				},
	{ MP_ROM_QSTR(MP_QSTR_serverInit),			MP_ROM_PTR(&qpy_ble_gatt_server_init_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_serverRelease),		MP_ROM_PTR(&qpy_ble_gatt_server_release_obj) 		},
	{ MP_ROM_QSTR(MP_QSTR_gattStart),			MP_ROM_PTR(&qpy_ble_gatt_start_obj) 				},
	{ MP_ROM_QSTR(MP_QSTR_gattStop),			MP_ROM_PTR(&qpy_ble_gatt_stop_obj) 					},
	{ MP_ROM_QSTR(MP_QSTR_getStatus),			MP_ROM_PTR(&qpy_ble_get_status_obj) 				},
	{ MP_ROM_QSTR(MP_QSTR_getPublicAddr),		MP_ROM_PTR(&qpy_ble_get_public_addr_obj) 			},
	//{ MP_ROM_QSTR(MP_QSTR_getConnStatus),		MP_ROM_PTR(&qpy_ble_get_conn_status_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_setLocalName),		MP_ROM_PTR(&qpy_ble_gatt_set_local_name_obj) 		},
	{ MP_ROM_QSTR(MP_QSTR_setAdvParam),			MP_ROM_PTR(&qpy_ble_set_adv_param_obj) 				},
	{ MP_ROM_QSTR(MP_QSTR_setAdvData),			MP_ROM_PTR(&qpy_ble_set_adv_data_obj) 				},
	{ MP_ROM_QSTR(MP_QSTR_setAdvRspData),		MP_ROM_PTR(&qpy_ble_set_adv_rsp_data_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_addService),			MP_ROM_PTR(&qpy_ble_gatt_add_service_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_addChara),			MP_ROM_PTR(&qpy_ble_gatt_add_chara_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_addCharaValue),		MP_ROM_PTR(&qpy_ble_gatt_add_chara_value_obj) 		},
	{ MP_ROM_QSTR(MP_QSTR_addCharaDesc),		MP_ROM_PTR(&qpy_ble_gatt_add_chara_desc_obj) 		},
	{ MP_ROM_QSTR(MP_QSTR_addOrClearService),	MP_ROM_PTR(&qpy_ble_gatt_add_or_clear_service_obj) 	},
	{ MP_ROM_QSTR(MP_QSTR_sendNotification),	MP_ROM_PTR(&qpy_ble_send_notification_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_sendIndication),		MP_ROM_PTR(&qpy_ble_send_indication_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_advStart),			MP_ROM_PTR(&qpy_ble_adv_start_obj) 					},
	{ MP_ROM_QSTR(MP_QSTR_advStop),				MP_ROM_PTR(&qpy_ble_adv_stop_obj) 					},
	
	{ MP_ROM_QSTR(MP_QSTR_clientInit),			MP_ROM_PTR(&qpy_ble_gatt_client_init_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_clientRelease),		MP_ROM_PTR(&qpy_ble_gatt_client_release_obj) 		},
	{ MP_ROM_QSTR(MP_QSTR_setScanParam),		MP_ROM_PTR(&qpy_ble_set_scan_param_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_scanStart),			MP_ROM_PTR(&qpy_ble_scan_start_obj) 				},
	{ MP_ROM_QSTR(MP_QSTR_scanStop),			MP_ROM_PTR(&qpy_ble_scan_stop_obj) 					},
	{ MP_ROM_QSTR(MP_QSTR_connect),				MP_ROM_PTR(&qpy_ble_connect_addr_obj) 				},
	{ MP_ROM_QSTR(MP_QSTR_cancelConnect),		MP_ROM_PTR(&qpy_ble_cancel_connect_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_disconnect),			MP_ROM_PTR(&qpy_ble_disconnect_obj) 				},
	{ MP_ROM_QSTR(MP_QSTR_discoverAllService),	MP_ROM_PTR(&qpy_ble_gatt_discover_service_obj) 		},
	{ MP_ROM_QSTR(MP_QSTR_discoverByUUID),		MP_ROM_PTR(&qpy_ble_gatt_discover_by_uuid_obj) 		},
	{ MP_ROM_QSTR(MP_QSTR_discoverAllIncludes),	MP_ROM_PTR(&qpy_ble_gatt_discover_all_includes_obj) },
	{ MP_ROM_QSTR(MP_QSTR_discoverAllChara),	MP_ROM_PTR(&qpy_ble_gatt_discover_all_chara_obj) 	},
	{ MP_ROM_QSTR(MP_QSTR_discoverAllCharaDesc),MP_ROM_PTR(&qpy_ble_gatt_discover_all_charadesc_obj)},
	{ MP_ROM_QSTR(MP_QSTR_readCharaByUUID),		MP_ROM_PTR(&qpy_ble_gatt_read_chara_by_uuid_obj)	},
	{ MP_ROM_QSTR(MP_QSTR_readCharaByHandle),	MP_ROM_PTR(&qpy_ble_gatt_read_chara_by_handle_obj)	},
	{ MP_ROM_QSTR(MP_QSTR_readMultiChara),		MP_ROM_PTR(&qpy_ble_gatt_read_multi_chara_obj)		},
	{ MP_ROM_QSTR(MP_QSTR_readCharaDesc),		MP_ROM_PTR(&qpy_ble_gatt_read_chara_desc_obj)		},
	{ MP_ROM_QSTR(MP_QSTR_writeChara),			MP_ROM_PTR(&qpy_ble_gatt_write_chara_obj)			},
	{ MP_ROM_QSTR(MP_QSTR_writeCharaDesc),		MP_ROM_PTR(&qpy_ble_gatt_write_chara_desc_obj)		},
	{ MP_ROM_QSTR(MP_QSTR_writeCharaNoRsp),		MP_ROM_PTR(&qpy_ble_gatt_write_chara_norsp_obj)		},
	{ MP_ROM_QSTR(MP_QSTR_setScanFilter),		MP_ROM_PTR(&qpy_ble_set_scanreport_filter_obj)		},
	
};
STATIC MP_DEFINE_CONST_DICT(mp_module_ble_globals, mp_module_ble_globals_table);


const mp_obj_module_t mp_module_ble = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ble_globals,
};


