#include <stdint.h>
#include <stdio.h>

#include "obj.h"
#include "runtime.h"

/* 定义的ethernet全局字典，之后我们添加type和function就要添加在这里 */
extern const mp_obj_type_t ethernet_DM9051_type;
STATIC const mp_rom_map_elem_t ethernet_globals_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ethernet)},   //这个对应python层面的__name__ 属性
    {MP_OBJ_NEW_QSTR(MP_QSTR_DM9051), MP_ROM_PTR(&ethernet_DM9051_type)},
};

/* 把ethernet_globals_table注册到 mp_module_ethernet_globals里面去 */
STATIC MP_DEFINE_CONST_DICT(mp_module_ethernet_globals, ethernet_globals_table);

/* 定义一个module类型 */
const mp_obj_module_t mp_module_ethernet = {
    .base = {&mp_type_module},    
    .globals = (mp_obj_dict_t *)&mp_module_ethernet_globals,
};