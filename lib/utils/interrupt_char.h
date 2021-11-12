/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2016 Damien P. George
 * Copyright (c) Quectel Wireless Solution, Co., Ltd.All Rights Reserved.
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
#ifndef MICROPY_INCLUDED_LIB_UTILS_INTERRUPT_CHAR_H
#define MICROPY_INCLUDED_LIB_UTILS_INTERRUPT_CHAR_H

typedef int Helios_Thread_t; 
extern int mainpy_running_flag;
extern int mainpy_interrupt_by_kbd_flag;
extern int repl_protect_enable;
extern Helios_Thread_t ql_micropython_task_ref;

#define RET_KBD_INTERRUPT (0x00FA00FA)

#define IS_REPL_REFUSED() (1 == repl_protect_enable)

#define MAINPY_RUNNING_FLAG mainpy_running_flag
#define MAINPY_INTERRUPT_BY_KBD_FLAG mainpy_interrupt_by_kbd_flag

#define MAINPY_RUNNING_FLAG_DEF int MAINPY_RUNNING_FLAG = 0;
#define MAINPY_INTERRUPT_BY_KBD_FLAG_DEF int MAINPY_INTERRUPT_BY_KBD_FLAG = 0;

#define MAINPY_RUNNING_FLAG_SET() MAINPY_RUNNING_FLAG = 1;
#define MAINPY_RUNNING_FLAG_CLEAR() MAINPY_RUNNING_FLAG = 0;
#define IS_MAINPY_RUNNING_FLAG_TRUE() (1 == MAINPY_RUNNING_FLAG)
#define IS_MAINPY_RUNNING_FLAG_FALSE() (0 == MAINPY_RUNNING_FLAG)
#define MAINPY_INTERRUPT_BY_KBD_FLAG_SET() MAINPY_INTERRUPT_BY_KBD_FLAG = 1;
#define MAINPY_INTERRUPT_BY_KBD_FLAG_CLEAR() MAINPY_INTERRUPT_BY_KBD_FLAG = 0;
#define MAINPY_INTERRUPT_BY_KBD_FLAG_TRUE() (1 == MAINPY_INTERRUPT_BY_KBD_FLAG)
#define MAINPY_INTERRUPT_BY_KBD_FLAG_FALSE() (0 == MAINPY_INTERRUPT_BY_KBD_FLAG)

#define IS_PYTHON_MAIN_THREAD() (Helios_Thread_GetID() == ql_micropython_task_ref)
#define IS_OBJ_KBD_INTERRUPT_TYPE() (MP_STATE_VM(mp_pending_exception) == MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception)))
#define CHECK_MAINPY_KBD_INTERRUPT_ENTER() if(IS_MAINPY_RUNNING_FLAG_FALSE() || !IS_OBJ_KBD_INTERRUPT_TYPE() || IS_PYTHON_MAIN_THREAD()) {
#define CHECK_MAINPY_KBD_INTERRUPT_EXIT()   }

extern int mp_interrupt_char;
void mp_hal_set_interrupt_char(int c);

#endif // MICROPY_INCLUDED_LIB_UTILS_INTERRUPT_CHAR_H
