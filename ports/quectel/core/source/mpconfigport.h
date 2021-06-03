#include <stdint.h>
//#include "ql_uart.h"

	// options to control how MicroPython is built
#define MICROPY_ENABLE_COMPILER     		(1)
#define MICROPY_USE_INTERNAL_ERRNO  		(1)
#define MICROPY_QSTR_BYTES_IN_HASH  		(1)
#define MICROPY_ALLOC_PATH_MAX      		(512)

#define MICROPY_EMIT_X64            		(0)
#define MICROPY_EMIT_THUMB          		(0)
#define MICROPY_EMIT_INLINE_THUMB   		(0)
#define MICROPY_COMP_MODULE_CONST   		(1)
#define MICROPY_COMP_CONST          		(1)
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN 	(0)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN 	(1)
#define MICROPY_MEM_STATS           		(0)
#define MICROPY_DEBUG_PRINTERS      		(1)
#define MICROPY_ENABLE_GC           		(1)
#define MICROPY_HELPER_REPL         		(1)
#define MICROPY_ENABLE_DOC_STRING   		(0)
#define MICROPY_ERROR_REPORTING     			(MICROPY_ERROR_REPORTING_NORMAL)


#define MICROPY_STACK_CHECK    				(1)
#define MICROPY_PY_BUILTINS_BYTEARRAY 		(1)
#define MICROPY_PY_BUILTINS_DICT_FROMKEYS 	(0)
#define MICROPY_PY_BUILTINS_MEMORYVIEW 		(0)
#define MICROPY_PY_BUILTINS_FROZENSET 		(0)
#define MICROPY_PY_BUILTINS_REVERSED 		(0)
#define MICROPY_PY_BUILTINS_SET     		(1)
#define MICROPY_PY_BUILTINS_SLICE   		(1)
#define MICROPY_PY_BUILTINS_PROPERTY 		(1)
#define MICROPY_PY_BUILTINS_STR_COUNT 		(1)
#define MICROPY_PY_BUILTINS_STR_OP_MODULO 	(1)

#define MICROPY_PY_BUILTINS_SLICE_ATTRS     (1)
#define MICROPY_PY_BUILTINS_SLICE_INDICES   (1)
#define MICROPY_PY_BUILTINS_RANGE_ATTRS     (1)
#define MICROPY_PY_BUILTINS_ROUND_INT       (1)

#define MICROPY_PY_BUILTINS_TIMEOUTERROR    (1)
#define MICROPY_PY_ALL_SPECIAL_METHODS      (1)
#define MICROPY_PY_BUILTINS_COMPILE         (1)
#define MICROPY_PY_BUILTINS_ENUMERATE       (1)
#define MICROPY_PY_BUILTINS_EXECFILE        (1)
#define MICROPY_PY_BUILTINS_FILTER          (1)
#define MICROPY_PY_BUILTINS_NOTIMPLEMENTED  (1)
#define MICROPY_PY_BUILTINS_MIN_MAX         (1)
#define MICROPY_PY_BUILTINS_POW3            (1)


#define MICROPY_KBD_EXCEPTION               (1)
#define MICROPY_PY___FILE__         		(1)
#define MICROPY_PY_GC               		(1)
#define MICROPY_PY_ARRAY            		(1)
#define MICROPY_PY_ATTRTUPLE        		(1)
#define MICROPY_PY_COLLECTIONS      		(1)
#define MICROPY_PY_MATH             		(1)
#define MICROPY_PY_CMATH            		(1)
#define MICROPY_PY_IO               		(1)
#define MICROPY_PY_IO_IOBASE        		(1)
#define MICROPY_PY_IO_BYTESIO       		(1)
#define MICROPY_PY_STRUCT           		(1)
#define MICROPY_PY_SYS              		(1)
#define MICROPY_PY_SYS_MAXSIZE      		(1)
#define MICROPY_PY_SYS_MODULES      		(1)
#define MICROPY_PY_SYS_EXIT         		(1)
#define MICROPY_PY_SYS_STDFILES     		(1)
#define MICROPY_PY_SYS_STDIO_BUFFER 		(1)
#define MICROPY_PY_UTIME_MP_HAL     		(1)
#define MICROPY_VFS                 		(1)
#define MICROPY_VFS_LFS1					(1)
#define MICROPY_PY_IO_FILEIO				(1)
#define MICROPY_PY_BUILTINS_INPUT   		(1)

#define MICROPY_CPYTHON_COMPAT      		(1)
#define MICROPY_LONGINT_IMPL        		(MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_FLOAT_IMPL          		(MICROPY_FLOAT_IMPL_FLOAT) //decimal numbers support
#define MICROPY_USE_INTERNAL_PRINTF 		(0)
#define MICROPY_REPL_AUTO_INDENT    		(1)
	//baron 2020/04/17
#define MICROPY_REPL_EMACS_WORDS_MOVE 		(1)
#define MICROPY_REPL_EMACS_KEYS				(1)
#define MICROPY_REPL_EMACS_EXTRA_WORDS_MOVE (1)
#define MICROPY_PY_BUILTINS_HELP            (1)
#define MICROPY_PY_BUILTINS_HELP_TEXT       quecpython_help_text
#define MICROPY_PY_BUILTINS_HELP_MODULES    (1)
#define MICROPY_DEBUG_VERBOSE       		(0)
	// Allow enabling debug prints after each REPL line
#define MICROPY_REPL_INFO           		(1)
	// Whether to include information in the byte code to determine source
	// line number (increases RAM usage, but doesn't slow byte code execution)
#define MICROPY_ENABLE_SOURCE_LINE  		(1)
#define MICROPY_PY_USOCKET_EVENTS			(1)
#define MICROPY_ENABLE_SCHEDULER    		(1)
#define MICROPY_SCHEDULER_DEPTH     		(8)
#define MICROPY_PY_BUILTINS_FILTER			(1)
#define MICROPY_MODULE_FROZEN_MPY   		(1)
#define MICROPY_QSTR_EXTRA_POOL     		mp_qstr_frozen_const_pool


	//ext modules
#define MICROPY_PY_UCTYPES          		(1)
#define MICROPY_PY_UZLIB            		(1)
#define MICROPY_PY_UJSON            		(1)
#define MICROPY_PY_URE                      (1)									 
#define MICROPY_PY_UBINASCII				(1)
#define MICROPY_READER_VFS                  (1)
#define MICROPY_PY_URANDOM					(1)
#define MICROPY_PY_URANDOM_EXTRA_FUNCS 		(1)
#define MICROPY_PY_THREAD           		(1)
#define MICROPY_PY_THREAD_GIL       		(1)
#define MICROPY_PY_THREAD_GIL_VM_DIVISOR    (32)


#define MICROPY_PY_USSL                     (1) // (1) for original, modified by Chavis for compilation on 1/7/2021
#define MICROPY_SSL_MBEDTLS                 (1) // (1) for original, modified by Chavis for compilation on 1/7/2021

#define MICROPY_PY_UHASHLIB 				(1)
#define MICROPY_PY_UHASHLIB_SHA256 			(1)
#define MICROPY_PY_UHASHLIB_SHA1  			(1)
#define MICROPY_PY_UHASHLIB_MD5 			(1)
//#define MBEDTLS_VERSION_NUMBER         0x02160000								   
#define MICROPY_PY_UCRYPTOLIB 				(1)
#define MICROPY_PY_UCRYPTOLIB_CTR 			(1)
#define MICROPY_PY_UCRYPTOLIB_CONSTS 		(1)	

#define MICROPY_PY_COLLECTIONS_DEQUE        (1)							   
//#define MICROPY_CAN_OVERRIDE_BUILTINS       (1)


	//end


	// type definitions for the specific machine
#define MICROPY_PY_MACHINE  				(1)
#define MICROPY_PY_EXAMPLE					(1)
#define MICROPY_PY_QUECLIB					(1)
#define MICROPY_PY_NET						(1)
#define MICROPY_PY_FOTA						(1)
#define MICROPY_PY_MISC						(1)

#define MICROPY_NLR_SETJMP                  (1)
// optimisations
#define MICROPY_OPT_COMPUTED_GOTO           (1)
#define MICROPY_OPT_MPZ_BITWISE             (1)

// emitters
#define MICROPY_PERSISTENT_CODE_LOAD        (1)
#define MICROPY_EMIT_XTENSAWIN              (1)


//#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF (1)
//#define MICROPY_WARNINGS                    (1)
//#define MICROPY_STREAMS_NON_BLOCK           (1)

#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void*)((mp_uint_t)(p) | 1))
#define MICROPY_ENABLE_FINALISER (1) // Pawn 2020/11/03


#define UINT_FMT "%lu"
#define INT_FMT "%ld"

#define BYTES_PER_WORD (4)

typedef int32_t mp_int_t; // must be pointer size
typedef uint32_t mp_uint_t; // must be pointer size
typedef long mp_off_t;

#if MICROPY_PY_USOCKET_EVENTS
#define MICROPY_PY_USOCKET_EVENTS_HANDLER extern void usocket_events_handler(void); usocket_events_handler();
#else
#define MICROPY_PY_USOCKET_EVENTS_HANDLER
#endif


#define mp_type_fileio                      mp_type_vfs_lfs1_fileio
#define mp_type_textio                      mp_type_vfs_lfs1_textio


extern const struct _mp_obj_module_t mp_module_machine;
extern const struct _mp_obj_module_t mp_module_usocket;
extern const struct _mp_obj_module_t example_module;
extern const struct _mp_obj_module_t utime_module;
extern const struct _mp_obj_module_t uos_module;
// extern const struct _mp_obj_module_t mp_module_queclib;
extern const struct _mp_obj_module_t mp_module_dial;
extern const struct _mp_obj_module_t mp_module_sim;
extern const struct _mp_obj_module_t mp_module_celllocator;
extern const struct _mp_obj_module_t mp_module_modem;
extern const struct _mp_obj_module_t mp_module_audio;
// Pawn EditStart -2020.08.18
extern const struct _mp_obj_module_t mp_module_misc;
extern const struct _mp_obj_module_t mp_module_net;
extern const struct _mp_obj_module_t mp_module_hmacSha1;
extern const struct _mp_obj_module_t mp_module_pm;
// end
extern const struct _mp_obj_type_t mp_ostimer_type;
extern const struct _mp_obj_module_t mp_module_sms;
extern const struct _mp_obj_module_t mp_module_utils;
extern const struct _mp_obj_module_t mp_module_hmacSha1;
extern const struct _mp_obj_module_t mp_module_audio;
extern const struct _mp_obj_module_t mp_module_wifiscan;

//#define MICROPY_PORT_BUILTIN_MODULES
extern const struct _mp_obj_module_t mp_module_bma250;

#if defined(PLAT_ASR) || defined(PLAT_Unisoc)
extern const struct _mp_obj_type_t mp_fota_type;
#define MICROPY_PORT_BUILTIN_MODULES_FOTA { MP_OBJ_NEW_QSTR(MP_QSTR_fota), (mp_obj_t)&mp_fota_type},
#else
#define MICROPY_PORT_BUILTIN_MODULES_FOTA
#endif

#ifdef CONFIG_LVGL
extern const struct _mp_obj_module_t mp_module_lvgl;
#define MICROPY_PORT_BUILTIN_MODULES_LVGL { MP_OBJ_NEW_QSTR(MP_QSTR_lvgl), (mp_obj_t)&mp_module_lvgl},
#else
#define MICROPY_PORT_BUILTIN_MODULES_LVGL
#endif

#ifdef CONFIG_CAMERA
extern const struct _mp_obj_module_t mp_module_camera;
#define MICROPY_PORT_BUILTIN_MODULES_CAMERA { MP_OBJ_NEW_QSTR(MP_QSTR_camera), (mp_obj_t)&mp_module_camera},
#else
#define MICROPY_PORT_BUILTIN_MODULES_CAMERA
#endif

#if defined(PLAT_Unisoc)
extern const struct _mp_obj_module_t mp_module_ble;
#define MICROPY_PORT_BUILTIN_MODULES_BLE { MP_OBJ_NEW_QSTR(MP_QSTR_ble), (mp_obj_t)&mp_module_ble},
#else
#define MICROPY_PORT_BUILTIN_MODULES_BLE
#endif

#if defined(PLAT_ASR)
extern const struct _mp_obj_module_t mp_module_sensor;
#define MICROPY_PORT_BUILTIN_MODULES_SENSOR { MP_OBJ_NEW_QSTR(MP_QSTR_sensor), (mp_obj_t)&mp_module_sensor},
#else
#define MICROPY_PORT_BUILTIN_MODULES_SENSOR
#endif


#define MICROPY_PORT_BUILTIN_MODULES \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_uos), (mp_obj_t)&uos_module }, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_machine), (mp_obj_t)&mp_module_machine }, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_usocket), (mp_obj_t)&mp_module_usocket }, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_misc), (mp_obj_t)&mp_module_misc}, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_osTimer), (mp_obj_t)&mp_ostimer_type}, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_example), (mp_obj_t)&example_module }, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_dial), (mp_obj_t)&mp_module_dial}, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_modem), (mp_obj_t)&mp_module_modem}, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_utime), (mp_obj_t)&utime_module }, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_utils), (mp_obj_t)&mp_module_utils},\
		{ MP_OBJ_NEW_QSTR(MP_QSTR_hmacSha1), (mp_obj_t)&mp_module_hmacSha1}, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_sms), (mp_obj_t)&mp_module_sms}, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_sim), (mp_obj_t)&mp_module_sim}, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_pm), (mp_obj_t)&mp_module_pm}, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_net), (mp_obj_t)&mp_module_net}, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_cellLocator), (mp_obj_t)&mp_module_celllocator}, \
		{ MP_OBJ_NEW_QSTR(MP_QSTR_wifiScan), (mp_obj_t)&mp_module_wifiscan},\
		{ MP_OBJ_NEW_QSTR(MP_QSTR_audio), (mp_obj_t)&mp_module_audio},\
		MICROPY_PORT_BUILTIN_MODULES_FOTA \
		MICROPY_PORT_BUILTIN_MODULES_LVGL \
		MICROPY_PORT_BUILTIN_MODULES_CAMERA \
		MICROPY_PORT_BUILTIN_MODULES_BLE \
		MICROPY_PORT_BUILTIN_MODULES_SENSOR


	// use vfs's functions for import stat and builtin open
#define mp_import_stat mp_vfs_import_stat
#define mp_builtin_open mp_vfs_open
#define mp_builtin_open_obj mp_vfs_open_obj

	// dummy print
	//#define MP_PLAT_PRINT_STRN(str, len) uart_printf("%s", str)
	//#define MP_PLAT_PRINT_STRN(str, len) ql_micropy_print_str(QL_MAIN_UART_PORT, str, len)
#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn_cooked(str, len)


//#define MICROPY_BEGIN_ATOMIC_SECTION()      disableInterrupts()
													
//#define MICROPY_END_ATOMIC_SECTION(state)   restoreInterrupts(state)




	// extra built in names to add to the global namespace
#define MICROPY_PORT_BUILTINS \
	    { MP_OBJ_NEW_QSTR(MP_QSTR_input), (mp_obj_t)&mp_builtin_input_obj }, \
	    { MP_OBJ_NEW_QSTR(MP_QSTR_open), (mp_obj_t)&mp_builtin_open_obj },



#if MICROPY_PY_THREAD
#define MICROPY_EVENT_POLL_HOOK \
	    do { \
	        extern void mp_handle_pending(bool); \
	        mp_handle_pending(true); \
	        MICROPY_PY_USOCKET_EVENTS_HANDLER \
	        MP_THREAD_GIL_EXIT(); \
			MP_THREAD_GIL_ENTER(); \
	    } while (0);
#else
#define MICROPY_EVENT_POLL_HOOK \
	    do { \
	        extern void mp_handle_pending(bool); \
	        mp_handle_pending(true); \
	        MICROPY_PY_USOCKET_EVENTS_HANDLER \
	        asm ("waiti 0"); \
	    } while (0);
#endif


#ifdef __thumb__
#define MICROPY_MIN_USE_CORTEX_CPU (1)
#define MICROPY_MIN_USE_STM32_MCU (1)
#endif

#define MP_STATE_PORT MP_STATE_VM

#define MICROPY_PORT_ROOT_POINTERS \
	    const char *readline_hist[8]; \
	    struct _machine_timer_obj_t *machine_timer_obj_head;


#define MICROPY_HW_BOARD_NAME "EC100Y"
#define MICROPY_HW_MCU_NAME	"QUECTEL"

// board specifics
#define MICROPY_PY_SYS_PLATFORM "EC100Y"

#ifndef SSIZE_MAX
#define SSIZE_MAX  0xFFFFFFFF
#endif

// We need to provide a declaration/definition of alloca()
#include "stdlib.h"
