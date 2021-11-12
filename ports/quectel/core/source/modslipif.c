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
#include "string.h"

#include "helios_audio.h"
#include "helios_debug.h"

#include "slipif.h"
#include "netif.h"
#include "ip4.h"

#include "obj.h"
#include "runtime.h"
#include "mphalport.h"
#include "helios_debug.h"



extern const mp_obj_type_t slipif_type;

#define sl_name "slip"
struct netif sl_netif;
static Helios_UARTNum global_dev;


unsigned long user_inet_addr(const char *str)
{
    unsigned long lHost = 0;
    int i = 1, j = 1;
    const char *pstr[4] = { NULL };
    pstr[0] = strchr(str, '.');
    pstr[1] = strchr(pstr[0] + 1, '.');
    pstr[2] = strchr(pstr[1] + 1, '.');
    pstr[3] = strchr(str, '\0');
 
    for (j = 0; j < 4; j++)
    {
        i = 1;
        if (j == 0)
        {
            while (str != pstr[0])
            {
                lHost += (*--pstr[j] - '0') * i;
                i *= 10;
            }
        }
        else
        {
            while (*--pstr[j] != '.')
            {
                lHost += (*pstr[j] - '0') * i << 8 * j;
                i *= 10;
            }
        }
    }
    return lHost;
}


static int _netif_init(char *ip_str, char *netmask_str, char *gw_str)
{
	ip_addr_t ipaddr;
	ip_addr_t netmask;
	ip_addr_t gw;
	int ret = 1;
	//char ip4_addr_str[16] = {0};
	//slip_log("user_netif_init start! port = %d\n", port);

	struct netif *temp;
	memset(&sl_netif, 0x00, sizeof(sl_netif));
	ipaddr.addr = user_inet_addr(ip_str);
	netmask.addr = user_inet_addr(netmask_str);
	gw.addr = user_inet_addr(gw_str);
	
    temp = netif_add(&sl_netif, &ipaddr, &netmask, &gw, &global_dev, slipif_init, ip_input);
	if( temp == NULL ) {
	//	slip_log("fail to netif init---------------\n");
		return -1;
	}
	sl_netif.hostname = sl_name;
    netif_set_up(&sl_netif);
	netif_set_link_up(&sl_netif);
	return ret;
}


STATIC mp_obj_t qpy_slipif_construct(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	int ret = 0;
	enum { ARG_port = 0, ARG_ip, ARG_netmask, ARG_gw};
	static const mp_arg_t allowed_args[] = 
	{
		{ MP_QSTR_port, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_slipIp, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_slipNetmask, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_slipGW, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
	};
	mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);
	
	int new_port = 0;
	new_port = vals[ARG_port].u_int;
	char *ip_str = (char *)mp_obj_str_get_str(vals[ARG_ip].u_obj);
	char *netmask_str = (char *)mp_obj_str_get_str(vals[ARG_netmask].u_obj);
	char *gw_str = (char *)mp_obj_str_get_str(vals[ARG_gw].u_obj);

	global_dev = (Helios_UARTNum)new_port;

	ret = _netif_init(ip_str, netmask_str, gw_str);

	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(qpy_slipif_construct_obj, 1, qpy_slipif_construct);

STATIC mp_obj_t qpy_slipif_netif_set_default(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	static const mp_arg_t allowed_args[] = 
	{
		{ MP_QSTR_slipExternalIp, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
	};
	mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);

	char *ip_str = (char *)mp_obj_str_get_str(vals[0].u_obj);
	ip_addr_t ipaddr;
	ipaddr.addr = user_inet_addr(ip_str);
	int ret = slipif_netif_set_default(ipaddr.addr);
	return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(qpy_slipif_netif_set_default_obj, 1, qpy_slipif_netif_set_default);


STATIC mp_obj_t qpy_slipif_set_mtu(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	static const mp_arg_t allowed_args[] = 
	{
		{ MP_QSTR_slipIp, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
	};
	mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);
	
	unsigned int mtu = vals[0].u_int;
	if( mtu > 1500 ) return mp_obj_new_int(-1);
	sl_netif.mtu = mtu;
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(qpy_slipif_set_mtu_obj, 1, qpy_slipif_set_mtu);

STATIC mp_obj_t qpy_slipif_destroy(mp_obj_t self_in)
{
	(void)self_in;
	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(qpy_slipif_destroy_obj, qpy_slipif_destroy);

STATIC const mp_rom_map_elem_t slipif_module_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_construct), MP_ROM_PTR(&qpy_slipif_construct_obj) },
    { MP_ROM_QSTR(MP_QSTR_netif_set_default), MP_ROM_PTR(&qpy_slipif_netif_set_default_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_smtu), MP_ROM_PTR(&qpy_slipif_set_mtu_obj) },
    { MP_ROM_QSTR(MP_QSTR_destroy), MP_ROM_PTR(&qpy_slipif_destroy_obj) },
};
STATIC MP_DEFINE_CONST_DICT(slipif_module_dict, slipif_module_dict_table);
#if 0
const mp_obj_type_t slipif_type = {
    { &mp_type_type },
  //  .name = MP_QSTR_SLIPIF,
    .locals_dict = (mp_obj_dict_t *)&slipif_locals_dict,
};
#endif
const mp_obj_module_t module_slip = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&slipif_module_dict,
};




// 1. 构建(串口，ip，netmask，gw)
// 2. 设置默认网卡
// 3. 设置mtu
// 4. 关闭

