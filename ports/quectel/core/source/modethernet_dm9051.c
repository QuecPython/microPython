#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "obj.h"
#include "runtime.h"
#include "mperrno.h"
#include "sockets.h"
#include "helios_pin.h"
#include "helios_dm9051.h"

/* 定义DM9051_obj_t结构体 */
typedef struct _DM9051_obj_t
{
    mp_obj_base_t base;   //定义的对象结构体要包含该成员
}DM9051_obj_t;

const mp_obj_type_t ethernet_DM9051_type;
STATIC mp_obj_t ethernet_DM9051_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 0, 6, false);   //检查参数个数
    DM9051_obj_t *self = m_new_obj(DM9051_obj_t);     //创建对象，分配空间
    self->base.type = &ethernet_DM9051_type;         //定义对象的类型

    uint8_t mac[6] = {0};
    char *ip_str = NULL, *mask_str = NULL, *gw_str = NULL;
    int spi_port = -1, spi_cs_pin = -1;

    switch(n_args) {
        case 6: {
            extern int export2internal_pin(Helios_GPIONum export_pin);
            spi_cs_pin = mp_obj_get_int(args[5]);
            if(spi_cs_pin != -1) {
                if(!(spi_cs_pin >= HELIOS_GPIO0 && spi_cs_pin <= HELIOS_GPIOMAX)) {
                    mp_raise_ValueError(MP_ERROR_TEXT("invalid spi cs pin"));
                    break;
                }
                spi_cs_pin = export2internal_pin((Helios_GPIONum)spi_cs_pin);
            }
            // no break
        }

        case 5: {
            spi_port = mp_obj_get_int(args[4]);
            if(spi_port != -1) {
                if(spi_port != 0 && spi_port != 1) {
                    mp_raise_ValueError(MP_ERROR_TEXT("invalid spi port"));
                    break;
                }
            }
            // no break
        }

        case 4: {
            char *_gw_str = (char *)mp_obj_str_get_str(args[3]);
            gw_str = (_gw_str[0] == '\0' ? NULL : _gw_str);
            // no break
        }

        case 3: {
            char *_mask_str = (char *)mp_obj_str_get_str(args[2]);
            mask_str = (_mask_str[0] == '\0' ? NULL : _mask_str);
            // no break
        }

        case 2: {
            char *_ip_str = (char *)mp_obj_str_get_str(args[1]);
            ip_str = (_ip_str[0] == '\0' ? NULL : _ip_str);
            // no break
        }

        case 1: {
            mp_buffer_info_t macinfo;
            mp_get_buffer_raise(args[0], &macinfo, MP_BUFFER_READ);
            memcpy(&mac, macinfo.buf, 6);
            break;
        }

        default:
            mp_raise_OSError(MP_EFAULT);
            break;
    }

    Helios_DM9051_NICSetSPI(spi_port, spi_cs_pin);
    if(Helios_DM9051_NICRegister(mac, ip_str, mask_str, gw_str))
        mp_raise_OSError(MP_EIO);
    
    return MP_OBJ_FROM_PTR(self);                 //返回对象的指针
}

/* 定义set_addr函数 */

STATIC const mp_arg_t ethernet_dm9051_set_addr_allowed_args[] = {
	{ MP_QSTR_ipaddr,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_maskaddr,   MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_gwaddr,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
};
STATIC mp_obj_t set_addr(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    mp_arg_val_t args[MP_ARRAY_SIZE(ethernet_dm9051_set_addr_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(ethernet_dm9051_set_addr_allowed_args), ethernet_dm9051_set_addr_allowed_args, args);
    
    DM9051_obj_t *self = (DM9051_obj_t *)MP_OBJ_TO_PTR(pos_args[0]);
    mp_buffer_info_t ipaddrinfo, maskaddrinfo, gwaddrinfo;
    mp_get_buffer_raise(args[0].u_obj, &ipaddrinfo, MP_BUFFER_READ);
    mp_get_buffer_raise(args[1].u_obj, &maskaddrinfo, MP_BUFFER_READ);
    mp_get_buffer_raise(args[2].u_obj, &gwaddrinfo, MP_BUFFER_READ);

    char ip_str[16] = {0}, mask_str[16] = {0}, gw_str[16] = {0};
    char *ip_str_fin = NULL, *mask_str_fin = NULL, *gw_str_fin = NULL;

    if(ipaddrinfo.len) {
        memcpy(ip_str, ipaddrinfo.buf, ipaddrinfo.len);
        ip_str_fin = ip_str;
    }
    
    if(maskaddrinfo.len) {
        memcpy(mask_str, maskaddrinfo.buf, maskaddrinfo.len);
        mask_str_fin = mask_str;
    }
    
    if(gwaddrinfo.len) {
        memcpy(gw_str, gwaddrinfo.buf, gwaddrinfo.len);
        gw_str_fin = gw_str;
    }
    
    return mp_obj_new_int(Helios_DM9051_NICSetAddr(ip_str_fin, mask_str_fin, gw_str_fin));
}

/* 定义set_dns函数 */
STATIC mp_obj_t set_dns(mp_obj_t self_in, mp_obj_t pri_addr, mp_obj_t sec_addr) {
    (void)self_in;
    char *pri_str = NULL, *sec_str = NULL;

    char *_pri_str = (char *)mp_obj_str_get_str(pri_addr);
    if (_pri_str[0] != '\0')
        pri_str = _pri_str;

    char *_sec_str = (char *)mp_obj_str_get_str(sec_addr);
    if (_sec_str[0] != '\0')
        sec_str = _sec_str;

    Helios_DM9051_NICSetDNS(pri_str, sec_str);
    
    return mp_const_none;
}

/* 定义set_up函数 */
STATIC mp_obj_t set_up(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_int(Helios_DM9051_NICSetUp());
}

/* 定义set_down函数 */
STATIC mp_obj_t set_down(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_int(Helios_DM9051_NICSetDown());
}

/* 定义start_dhcp函数 */
STATIC mp_obj_t start_dhcp(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_int(Helios_DM9051_NICDHCP());
}

/* 定义ipconfig函数 */
STATIC mp_obj_t ipconfig(mp_obj_t self_in) {
    (void)self_in;
    dm_ethnetif_ipconfig_t ipconfig = {0};
    char mac[24] = {0};
    char ip[16] = {0};
    char mask[16] = {0};
    char gw[16] = {0};
    char dns1[16] = {0};
    char dns2[16] = {0};

    Helios_DM9051_IPConfig(&ipconfig);

    sprintf(mac, "%02X-%02X-%02X-%02X-%02X-%02X", ipconfig.mac[0], ipconfig.mac[1], ipconfig.mac[2], ipconfig.mac[3], ipconfig.mac[4], ipconfig.mac[5]);

    mp_obj_t generic_tuple[2] = {
        mp_obj_new_str(mac, strlen(mac)),
        mp_obj_new_str(ipconfig.hostname, strlen(ipconfig.hostname))
    };

    inet_ntop(AF_INET, (void *)&ipconfig.ipv4_info.ipaddr, ip, sizeof(ip));
    inet_ntop(AF_INET, (void *)&ipconfig.ipv4_info.netmask, mask, sizeof(mask));
    inet_ntop(AF_INET, (void *)&ipconfig.ipv4_info.gw, gw, sizeof(gw));
    inet_ntop(AF_INET, (void *)&ipconfig.ipv4_info.dns_server[0], dns1, sizeof(dns1));
    inet_ntop(AF_INET, (void *)&ipconfig.ipv4_info.dns_server[1], dns2, sizeof(dns2));

    mp_obj_t ipv4info_tuple[6] = {
        mp_obj_new_int(ipconfig.ipv4_info.iptype),
        mp_obj_new_str(ip, strlen(ip)),
        mp_obj_new_str(mask, strlen(mask)),
        mp_obj_new_str(gw, strlen(gw)),
        mp_obj_new_str(dns1, strlen(dns1)),
        mp_obj_new_str(dns2, strlen(dns2))
    };

    mp_obj_t ipconfig_list[2] = {
        mp_obj_new_tuple(2, generic_tuple),
        mp_obj_new_tuple(6, ipv4info_tuple)
    };

    return mp_obj_new_list(2, ipconfig_list);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(set_addr_obj, 4, set_addr);
STATIC MP_DEFINE_CONST_FUN_OBJ_3(set_dns_obj, set_dns);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(set_up_obj, set_up);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(set_down_obj, set_down);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(start_dhcp_obj, start_dhcp);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ipconfig_obj, ipconfig);

/* 定义type的本地字典 */
STATIC const mp_rom_map_elem_t DM9051_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_addr), MP_ROM_PTR(&set_addr_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_dns), MP_ROM_PTR(&set_dns_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_up), MP_ROM_PTR(&set_up_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_down), MP_ROM_PTR(&set_down_obj) },
    { MP_ROM_QSTR(MP_QSTR_dhcp), MP_ROM_PTR(&start_dhcp_obj) },
    { MP_ROM_QSTR(MP_QSTR_ipconfig), MP_ROM_PTR(&ipconfig_obj) },
};

/* 把DM9051_locals_dict_table注册到DM9051_locals_dict里面去 */
STATIC MP_DEFINE_CONST_DICT(DM9051_locals_dict, DM9051_locals_dict_table);

/* 定义mp_obj_type_t类型的结构体；注意这里和定义module使用的类型是不一样的 */
const mp_obj_type_t ethernet_DM9051_type = {
    .base={ &mp_type_type }, 
    .name = MP_QSTR_DM9051, //type类的name属性是放在这里定义，而不是放在DICT中
    .make_new = ethernet_DM9051_make_new, //添加的make new属性
    .locals_dict = (mp_obj_dict_t*)&DM9051_locals_dict, //注册DM9051_locals_dict
};