/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Damien P. George
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

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "mphalport.h"
#include "callbackdeal.h"

#if MICROPY_KBD_EXCEPTION
// This function may be called asynchronously at any time so only do the bare minimum.
void MICROPY_WRAP_MP_KEYBOARD_INTERRUPT(mp_keyboard_interrupt)(void) {
    MP_STATE_VM(mp_kbd_exception).traceback_data = NULL;
    MP_STATE_VM(mp_pending_exception) = MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception));
#if MICROPY_ENABLE_CALLBACK_DEAL
#if !defined(PLAT_RDA) && !defined(PLAT_Qualcomm)
    quecpython_send_msg_to_sleep_func();
#endif
#endif
    #if MICROPY_ENABLE_SCHEDULER
    if (MP_STATE_VM(sched_state) == MP_SCHED_IDLE) {
        MP_STATE_VM(sched_state) = MP_SCHED_PENDING;
    }
    #endif
}
#endif

#if MICROPY_ENABLE_SCHEDULER

#define IDX_MASK(i) ((i) & (MICROPY_SCHEDULER_DEPTH - 1))

// This is a macro so it is guaranteed to be inlined in functions like
// mp_sched_schedule that may be located in a special memory region.
#define mp_sched_full() (mp_sched_num_pending() == MICROPY_SCHEDULER_DEPTH)

static inline bool mp_sched_empty(void) {
    MP_STATIC_ASSERT(MICROPY_SCHEDULER_DEPTH <= 255); // MICROPY_SCHEDULER_DEPTH must fit in 8 bits
    MP_STATIC_ASSERT((IDX_MASK(MICROPY_SCHEDULER_DEPTH) == 0)); // MICROPY_SCHEDULER_DEPTH must be a power of 2

    return mp_sched_num_pending() == 0;
}

// A variant of this is inlined in the VM at the pending exception check
void mp_handle_pending(bool raise_exc) {
    if (MP_STATE_VM(sched_state) == MP_SCHED_PENDING) {
        mp_uint_t atomic_state = MICROPY_BEGIN_ATOMIC_SECTION();
        // Re-check state is still pending now that we're in the atomic section.
        if (MP_STATE_VM(sched_state) == MP_SCHED_PENDING) {
            mp_obj_t obj = MP_STATE_VM(mp_pending_exception);
            if (obj != MP_OBJ_NULL) {
                #if MICROPY_KBD_EXCEPTION
                CHECK_MAINPY_KBD_INTERRUPT_ENTER()
                #endif
                MP_STATE_VM(mp_pending_exception) = MP_OBJ_NULL;
                if (!mp_sched_num_pending()) {
                    MP_STATE_VM(sched_state) = MP_SCHED_IDLE;
                }
                if (raise_exc) {
                    MICROPY_END_ATOMIC_SECTION(atomic_state);
                    nlr_raise(obj);
                }
                #if MICROPY_KBD_EXCEPTION
                CHECK_MAINPY_KBD_INTERRUPT_EXIT()
                #endif
            }
            mp_handle_pending_tail(atomic_state);
        } else {
            MICROPY_END_ATOMIC_SECTION(atomic_state);
        }
    }
}

// This function should only be called by mp_handle_pending,
// or by the VM's inlined version of that function.
void mp_handle_pending_tail(mp_uint_t atomic_state) {
    MP_STATE_VM(sched_state) = MP_SCHED_LOCKED;
    if (!mp_sched_empty()) {
        mp_sched_item_t item = MP_STATE_VM(sched_queue)[MP_STATE_VM(sched_idx)];
        MP_STATE_VM(sched_idx) = IDX_MASK(MP_STATE_VM(sched_idx) + 1);
        --MP_STATE_VM(sched_len);
        MICROPY_END_ATOMIC_SECTION(atomic_state);
        mp_call_function_1_protected(item.func, item.arg);
    } else {
        MICROPY_END_ATOMIC_SECTION(atomic_state);
    }
    mp_sched_unlock();
}

void mp_sched_lock(void) {
    mp_uint_t atomic_state = MICROPY_BEGIN_ATOMIC_SECTION();
    if (MP_STATE_VM(sched_state) < 0) {
        --MP_STATE_VM(sched_state);
    } else {
        MP_STATE_VM(sched_state) = MP_SCHED_LOCKED;
    }
    MICROPY_END_ATOMIC_SECTION(atomic_state);
}

void mp_sched_unlock(void) {
    mp_uint_t atomic_state = MICROPY_BEGIN_ATOMIC_SECTION();
    assert(MP_STATE_VM(sched_state) < 0);
    if (++MP_STATE_VM(sched_state) == 0) {
        // vm became unlocked
        if (MP_STATE_VM(mp_pending_exception) != MP_OBJ_NULL || mp_sched_num_pending()) {
            MP_STATE_VM(sched_state) = MP_SCHED_PENDING;
        } else {
            MP_STATE_VM(sched_state) = MP_SCHED_IDLE;
        }
    }
    MICROPY_END_ATOMIC_SECTION(atomic_state);
}

extern void mp_hal_stdin_send_msg_to_rx_chr(void);
bool MICROPY_WRAP_MP_SCHED_SCHEDULE(mp_sched_schedule)(mp_obj_t function, mp_obj_t arg) {
    mp_uint_t atomic_state = MICROPY_BEGIN_ATOMIC_SECTION();
    bool ret;
    if (!mp_sched_full()) {
        if (MP_STATE_VM(sched_state) == MP_SCHED_IDLE) {
            MP_STATE_VM(sched_state) = MP_SCHED_PENDING;
        }
        uint8_t iput = IDX_MASK(MP_STATE_VM(sched_idx) + MP_STATE_VM(sched_len)++);
        MP_STATE_VM(sched_queue)[iput].func = function;
        MP_STATE_VM(sched_queue)[iput].arg = arg;
        ret = true;
    } else {
        // schedule queue is full
        ret = false;
    }
    MICROPY_END_ATOMIC_SECTION(atomic_state);

    if(true == ret) {
    #if MICROPY_ENABLE_CALLBACK_DEAL
        extern Helios_Thread_t qpy_callback_deal_task_ref;
	    if(Helios_Thread_GetID() != qpy_callback_deal_task_ref) {
    	    qpy_send_msg_to_callback_deal_thread(CALLBACK_TYPE_ID_NONE, NULL);
    	}
    #else
        #if !defined(PLAT_RDA) && !defined(PLAT_Qualcomm)
        quecpython_send_msg_to_sleep_func();
        #endif
    #endif
    	mp_hal_stdin_send_msg_to_rx_chr();//forrest.liu@20210809 add for quecpython task repl using waiting msg
    }
    return ret;
}

extern bool mp_obj_is_boundmeth(mp_obj_t o);
extern mp_obj_t mp_obj_bound_get_self(void *bound);
extern mp_obj_t mp_obj_bound_get_meth(void *bound);
//Add a callback to the unscheduled table. If it is a Method,
//load it first so that the recorded callback is not collected by the GC
bool mp_sched_schedule_ex(c_callback_t *callback, mp_obj_t arg)
{
    if(NULL == callback)
    {
        return false;
    }

    if((false == callback->is_method) && mp_obj_is_callable(callback->cb))//function
    {
        return mp_sched_schedule(callback->cb, arg);
    }
    else if(true == callback->is_method)//bound_method
    {
        if(mp_obj_is_boundmeth(callback->cb)) {
            return mp_sched_schedule(callback->cb, arg);
        } else {
            mp_obj_t cb = mp_load_attr(callback->method_self, callback->method_name);
            return mp_sched_schedule(cb, arg);
        }
    }
    else
    {
	    return false;
	}
}

//Register callback
bool mp_sched_schedule_callback_register(c_callback_t *callback, mp_obj_t func_obj)
{
    if(NULL == callback) {
        return false;
    }
    memset(callback, 0 , sizeof(c_callback_t));
    if(mp_const_none != func_obj)
    {
        const mp_obj_type_t *type = mp_obj_get_type(func_obj);
        if(type->name == MP_QSTR_bound_method)//is bound_meth. Records the object for this method, along with the bytecode for the name of this method
        {
            callback->is_method = true;
            callback->method_self = mp_obj_bound_get_self(func_obj);
            callback->method_name = mp_obj_fun_get_name(mp_obj_bound_get_meth(func_obj));
            callback->cb = func_obj;
            return true;
        }
    }

    callback->is_method = false;
    callback->cb = func_obj;

    return true;
}

#else // MICROPY_ENABLE_SCHEDULER

// A variant of this is inlined in the VM at the pending exception check
void mp_handle_pending(bool raise_exc) {
    if (MP_STATE_VM(mp_pending_exception) != MP_OBJ_NULL) {
        mp_obj_t obj = MP_STATE_VM(mp_pending_exception);
        MP_STATE_VM(mp_pending_exception) = MP_OBJ_NULL;
        if (raise_exc) {
            nlr_raise(obj);
        }
    }
}

#endif // MICROPY_ENABLE_SCHEDULER
