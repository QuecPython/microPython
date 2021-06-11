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

#include "mpstate.h"
#include "gc.h"
#include "mpthread.h"
//#include "utils.h"

#if defined(BOARD_BC25PA)
unsigned int ReadSP(void)
{
    uint32_t res;
    __asm volatile (
        "move %0, $29\n"
        :"=r"(res)
        :
        :
    );
    
    return res;
}
#else
unsigned int ReadSP(void)
{
    uint32_t res;
    __asm volatile (
        "mov %0, r13\n"
        :"=r"(res)
        :
        :
    );
    
    return res;
}
#endif

void gc_collect(void) {
    // get current time, in case we want to time the GC
    #if 0
    uint32_t start = mp_hal_ticks_us();
    #endif

    // start the GC
    gc_collect_start();

    // get the registers and the sp
    //uintptr_t regs[10];
    //uintptr_t sp = gc_helper_get_regs_and_sp(regs);
    uintptr_t sp = (uintptr_t)ReadSP();

    // trace the stack, including the registers (since they live on the stack in this function)
    gc_collect_root((void **)sp, ((uint32_t)MP_STATE_THREAD(stack_top) - sp) / sizeof(uint32_t));

    // trace root pointers from any threads
    #if MICROPY_PY_THREAD
    mp_thread_gc_others();
    #endif

    // end the GC
    gc_collect_end();

    #if 0
    // print GC info
    uint32_t ticks = mp_hal_ticks_us() - start;
    gc_info_t info;
    gc_info(&info);
    printf("GC@%lu %lums\n", start, ticks);
    printf(" " UINT_FMT " total\n", info.total);
    printf(" " UINT_FMT " : " UINT_FMT "\n", info.used, info.free);
    printf(" 1=" UINT_FMT " 2=" UINT_FMT " m=" UINT_FMT "\n", info.num_1block, info.num_2block, info.max_block);
    #endif
}
