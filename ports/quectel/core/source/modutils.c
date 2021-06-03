
#include <stdio.h>
#include <stdint.h>
#include "mpconfigport.h"

#include "compile.h"
#include "runtime.h"
#include "repl.h"
#include "mperrno.h"

#include "modutils.h"

STATIC const mp_rom_map_elem_t utils_module_globals_table[] = {
	{ MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_utils) },
	{ MP_ROM_QSTR(MP_QSTR_crc32), MP_ROM_PTR(&mp_crc32_type) },
};

STATIC MP_DEFINE_CONST_DICT(utils_module_globals, utils_module_globals_table);

const mp_obj_module_t mp_module_utils = {
	.base = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&utils_module_globals,
};

