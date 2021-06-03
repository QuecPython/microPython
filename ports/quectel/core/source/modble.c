/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2021, QUECTEL 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nlr.h"
#include "objlist.h"
#include "objstr.h"
#include "runtime.h"
#include "mperrno.h"
#include "helios_debug.h"
#include "helios_ble.h"

#define MOD_BLE_LOG(msg, ...)      custom_log(ble, msg, ##__VA_ARGS__)

static mp_obj_t g_ble_callback = NULL;

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


static void qpy_ble_user_callback(void *ind_msg_buf, void *ctx)
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
			
			if (g_ble_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule(g_ble_callback, mp_obj_new_tuple(2, tuple));
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
				mp_obj_new_str(addr_buf, strlen(addr_buf)),
			};
		
			if (g_ble_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule(g_ble_callback, mp_obj_new_tuple(4, tuple));
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
			
			if (g_ble_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule(g_ble_callback, mp_obj_new_tuple(7, tuple));
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
			
			if (g_ble_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule(g_ble_callback, mp_obj_new_tuple(4, tuple));
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
			
			if (g_ble_callback)
			{
				MOD_BLE_LOG("[ble] callback start.\r\n");
				mp_sched_schedule(g_ble_callback, mp_obj_new_tuple(7, tuple));
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
	g_ble_callback = callback;
	info.ble_user_cb = qpy_ble_user_callback;
	int ret = Helios_BLE_GattServerInit(&info);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_ble_gatt_server_init_obj, qpy_ble_gatt_server_init);


STATIC mp_obj_t qpy_ble_gatt_server_release(void)
{
	int ret = 0;
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


STATIC const mp_rom_map_elem_t mp_module_ble_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__),			MP_ROM_QSTR(MP_QSTR_ble) 							},
	{ MP_ROM_QSTR(MP_QSTR_serverInit),			MP_ROM_PTR(&qpy_ble_gatt_server_init_obj) 			},
	{ MP_ROM_QSTR(MP_QSTR_serverRelease),		MP_ROM_PTR(&qpy_ble_gatt_server_release_obj) 		},
	{ MP_ROM_QSTR(MP_QSTR_gattStart),			MP_ROM_PTR(&qpy_ble_gatt_start_obj) 				},
	{ MP_ROM_QSTR(MP_QSTR_gattStop),			MP_ROM_PTR(&qpy_ble_gatt_stop_obj) 					},
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
};
STATIC MP_DEFINE_CONST_DICT(mp_module_ble_globals, mp_module_ble_globals_table);


const mp_obj_module_t mp_module_ble = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ble_globals,
};


