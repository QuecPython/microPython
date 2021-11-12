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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "obj.h"
#include "compile.h"
#include "runtime.h"
#include "repl.h"
#include "mperrno.h"
#include "pyexec.h"
#include "gc.h"
#include "helios.h"
#include "helios_os.h"
#include "mphalport.h"
#include "stackctrl.h"
#include "mphal.h"
#include "readline.h"
#include "mpprint.h"
#include "objmodule.h"

#if !defined(PLAT_RDA)
#if CONFIG_MBEDTLS
#include "mbedtls_init.h"
#endif
#endif

Helios_Thread_t ql_micropython_task_ref;

#if defined(PLAT_RDA)
#define MP_TASK_STACK_SIZE      (32 * 1024)
#else
#define MP_TASK_STACK_SIZE      (64 * 1024)
#endif
#define MP_TASK_STACK_LEN       (MP_TASK_STACK_SIZE/sizeof(uint32_t))


void nlr_jump_fail(void *val) {
    while(1);
}

void NORETURN __fatal_error(const char *msg) {
    while(1);
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif


/*
int _lseek() {return 0;}
int _read() {return 0;}
int _write() {return 0;}
int _close() {return 0;}
void _exit(int x) {for(;;){}}
int _sbrk() {return 0;}
int _kill() {return 0;}
int _getpid() {return 0;}
int _fstat() {return 0;}
int _isatty() {return 0;}
*/
/*
void *malloc(size_t n) {return NULL;}
void *calloc(size_t nmemb, size_t size) {return NULL;}
void *realloc(void *ptr, size_t size) {return NULL;}
void free(void *p) {}
int printf(const char *m, ...) {return 0;}
void *memcpy(void *dest, const void *src, size_t n) {return NULL;}
int memcmp(const void *s1, const void *s2, size_t n) {return 0;}
void *memmove(void *dest, const void *src, size_t n) {return NULL;}
void *memset(void *s, int c, size_t n) {return NULL;}
int strcmp(const char *s1, const char* s2) {return 0;}
int strncmp(const char *s1, const char* s2, size_t n) {return 0;}
size_t strlen(const char *s) {return 0;}
char *strcat(char *dest, const char *src) {return NULL;}
char *strchr(const char *dest, int c) {return NULL;}
#include <stdarg.h>
int vprintf(const char *format, va_list ap) {return 0;}
int vsnprintf(char *str,  size_t  size,  const  char  *format, va_list ap) {return 0;}

#undef putchar
int putchar(int c) {return 0;}
int puts(const char *s) {return 0;}

void _start(void) {main(0, NULL);}
*/
static char *stack_top;
#if MICROPY_ENABLE_GC
#if defined(PLAT_Qualcomm)
static char heap[650 * 1024];
#else
static char heap[1024 * 512];
#endif
#endif
extern pyexec_mode_kind_t pyexec_mode_kind;
extern void machine_timer_deinit_all(void);

MAINPY_RUNNING_FLAG_DEF
MAINPY_INTERRUPT_BY_KBD_FLAG_DEF

void quecpython_task(void *arg)
{
	Helios_Thread_t id = 0;
	void *stack_ptr = NULL;
	int stack_dummy;

#if !defined(PLAT_RDA) && !defined(PLAT_Qualcomm)
    //Added by Freddy @20210520 在线程sleep时，通过wait queue的方式超时代替实际的sleep,
    //当有callback执行时，给此queue发消息即可快速执行callback
    void quecpython_callback_deal_queue_create(void);
    quecpython_callback_deal_queue_create();
#endif

#if !defined(PLAT_RDA)
#if CONFIG_MBEDTLS
    mbedtls_platform_setup(NULL);
#endif
#endif

	#if MICROPY_PY_THREAD
    id = Helios_Thread_GetID();
    ql_micropython_task_ref = id;
    stack_ptr = Helios_Thread_GetStaskPtr(id);
	mp_thread_init(stack_ptr, MP_TASK_STACK_LEN);
	//uart_printf("===========id: 0x%4X, stack_ptr: 0x%08X==========\r\n", id, stack_ptr);
	#endif

	if(mp_hal_stdio_init()) return;

    /*
	** Jayceon-2020/09/23:
	** Resolve all sorts of weird bugs that occur using the quecpython.py tool.
	*/
#if 0
	//ql_uart_register_cb(3, hal_uart_cb);
	qpy_uart_registerCB(1, qpy_hal_uart_cb);
#endif	
soft_reset:
	mp_stack_set_top((void *)&stack_dummy);
    mp_stack_set_limit(MP_TASK_STACK_SIZE - 1024);
	//uart_printf("stack_dummy: %p, stack_ptr: %p\r\n", &stack_dummy, stack_ptr);
    stack_top = (char*)&stack_dummy;
    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif
    mp_init();
    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_));
//  mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_lib));
    mp_obj_list_init(mp_sys_argv, 0);
    readline_init0();
	
	// run boot-up scripts
#if defined(PLAT_RDA)
    pyexec_frozen_module("_boot_RDA.py");
#elif defined(PLAT_Qualcomm)
	pyexec_frozen_module("_boot_Qualcomm.py");
#else
    pyexec_frozen_module("_boot.py");
#endif
    if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL && MAINPY_INTERRUPT_BY_KBD_FLAG_FALSE()) {
        MAINPY_RUNNING_FLAG_SET();
        int ret = pyexec_file_if_exists("/usr/main.py");
        MAINPY_RUNNING_FLAG_CLEAR();
        if(RET_KBD_INTERRUPT == ret)
        {
            MAINPY_INTERRUPT_BY_KBD_FLAG_SET();
        }
    }
    else
    {
        MAINPY_INTERRUPT_BY_KBD_FLAG_CLEAR();
    }

    if(MAINPY_INTERRUPT_BY_KBD_FLAG_FALSE())
    {
    	for (;;) {
            if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
                if (pyexec_raw_repl() != 0) {
                    break;
                }
            } else {
                if (pyexec_friendly_repl() != 0) {
                    break;
                }
            }
        }
    }
    
    machine_timer_deinit_all();

	#if MICROPY_PY_THREAD
	//uart_printf("mp_thread_deinit in quecpython task.\r\n");
    mp_thread_deinit();
    #endif

    mp_module_deinit_all();

	gc_sweep_all();

    mp_hal_stdout_tx_str("MPY: soft reboot\r\n");
	mp_deinit();
#if !defined(PLAT_RDA)
    fflush(stdout);
#endif
    goto soft_reset;
}
/* void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    gc_dump_info();
}
 */

#include "stdarg.h"
int DEBUG_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = mp_vprintf(MICROPY_DEBUG_PRINTER, fmt, ap);
    va_end(ap);
    return ret;
}

application_init(quecpython_task, "quecpython_task", (MP_TASK_STACK_SIZE)/1024, 0);

