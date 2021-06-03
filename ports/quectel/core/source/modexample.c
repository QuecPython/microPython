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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "compile.h"
#include "runtime0.h"
#include "nlr.h"
#include "objlist.h"
#include "objstr.h"
#include "runtime.h"
#include "mperrno.h"
#include "mphal.h"
#include "stream.h"
#include "mperrno.h"
#include "pyexec.h"

//void do_str(const char *src, mp_parse_input_kind_t input_kind) {
//    nlr_buf_t nlr;
//    if (nlr_push(&nlr) == 0) {
//        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
//        qstr source_name = lex->source_name;
//        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
//        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);
//        mp_call_function_0(module_fun);
//        nlr_pop();
//    } else {
//        // uncaught exception
//        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
//    }
//}


//STATIC mp_obj_t example_exec(const mp_obj_t arg0)
//{
//	QFILE * fp = NULL;
//	int ret, fsize = 0;
//	char fname[128] = {0};
//	mp_buffer_info_t bufinfo;
//	
//	mp_get_buffer_raise(arg0, &bufinfo, MP_BUFFER_READ);
//	uart_printf("example_exec: %s\r\n", bufinfo.buf);
//	if(strlen(bufinfo.buf) > 0)
//	{
//		snprintf(fname, sizeof(fname), "%s/%s", "U:", bufinfo.buf);
//		fp = ql_fopen(fname, "r");
//		if(fp == NULL)
//		{
//			mp_raise_ValueError("file open failed.");
//		}
//		else
//		{
//			fsize = ql_fsize(fp);
//			char *content = malloc(fsize + 1);
//			if(content == NULL)
//			{
//				ql_fclose(fp);
//				mp_raise_ValueError("malloc error.");
//				return mp_const_none;
//			}
//			else
//			{
//				ret = ql_fread((void*)content, fsize, 1, fp);
//				if(ret != fsize)
//				{
//					ql_fclose(fp);
//					mp_raise_ValueError("read error.");
//				}
//				else
//				{
//					content[fsize] = '\0';
//					do_str(content, MP_PARSE_FILE_INPUT);
//				}
//				free(content);
//			}
//		}
//	}
//	else
//	{
//		mp_raise_ValueError("file not exit.");
//	}
//	return mp_const_none;
//}

STATIC mp_obj_t example_exec(const mp_obj_t arg0)
{
	int ret;
	
	mp_buffer_info_t bufinfo;
	char fname[128] = {0};
	char path[128] = {0};
	mp_get_buffer_raise(arg0, &bufinfo, MP_BUFFER_READ);

	memcpy(path, bufinfo.buf, bufinfo.len);
	
	if(bufinfo.buf != NULL)
	{
		// Pawn 2021-01-18 for JIRA STASR3601-2428 begin
		if (path[0] != '/')
		{
			snprintf(fname, sizeof(fname), "/%s", (char *)bufinfo.buf);
		}
		else
		{
			snprintf(fname, sizeof(fname), "%s", (char *)bufinfo.buf);
		}
		ret = pyexec_file_if_exists(fname);
	}
	if ( ret == -1 )
	{
		mp_raise_msg_varg(&mp_type_OSError, "File path error or not exist: [%s]", (char *)bufinfo.buf);
	}
	// Pawn 2021-01-18 for JIRA STASR3601-2428 end
	return mp_const_none;
}



STATIC MP_DEFINE_CONST_FUN_OBJ_1(example_exec_obj, example_exec);


STATIC mp_obj_t example_initialize()
{
    static int initialized = 0;
    if (!initialized) {
        initialized = 1;
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(example_initialize_obj, example_initialize);

STATIC const mp_rom_map_elem_t mp_module_example_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_example) },
    { MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&example_initialize_obj) },
    { MP_ROM_QSTR(MP_QSTR_exec), MP_ROM_PTR(&example_exec_obj) },

};


STATIC MP_DEFINE_CONST_DICT(mp_module_example_globals, mp_module_example_globals_table);


const mp_obj_module_t example_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_example_globals,
};

