
NAME := MICROPYTHON

include config/$(KCONFIG_CONFIG)

$(NAME)_SRCS = \
	extmod/modubinascii.c \
	extmod/moductypes.c \
	extmod/modujson.c \
	extmod/modurandom.c \
	extmod/modure.c \
	extmod/moduzlib.c \
	extmod/moduasyncio.c \
	extmod/moduselect.c \
	extmod/utime_mphal.c \
	extmod/vfs.c \
	extmod/vfs_blockdev.c \
	extmod/vfs_lfs.c \
	extmod/vfs_reader.c \
	extmod/moducryptolib.c \
	extmod/moduhashlib.c  \
	extmod/modaes.c \
	extmod/modhash.c  \
	lib/littlefs/lfs1.c \
	lib/littlefs/lfs1_util.c \
	lib/mp-readline/readline.c \
	lib/netutils/netutils.c \
	lib/timeutils/timeutils.c \
	lib/utils/interrupt_char.c \
	lib/utils/pyexec.c \
	lib/utils/stdout_helpers.c \
	lib/utils/sys_stdio_mphal.c \
	ports/quectel/core/source/gccollect.c \
	ports/quectel/core/source/help.c \
	ports/quectel/core/source/mphalport.c \
	ports/quectel/core/source/mpthreadport.c \
	ports/quectel/core/source/quecpython.c \
	ports/quectel/core/source/linklist.c \
	ports/quectel/core/source/modsha1.c \
	ports/quectel/core/source/modutils.c \
	ports/quectel/core/source/utils_crc32.c \
	ports/quectel/core/build/_frozen_mpy.c \
	ports/quectel/core/source/moduos.c \
	ports/quectel/core/source/modexample.c \
	ports/quectel/core/source/modutime.c \
	ports/quectel/core/source/modmachine.c \
	ports/quectel/core/source/machine_uart.c \
	ports/quectel/core/source/machine_pin.c \
	ports/quectel/core/source/machine_extint.c \
	ports/quectel/core/source/machine_hw_spi.c \
	ports/quectel/core/source/modsocket.c \
	ports/quectel/core/source/moddatacall.c \
	ports/quectel/core/source/modmisc.c \
	ports/quectel/core/source/misc_power.c \
    ports/quectel/core/source/misc_adc.c \
    ports/quectel/core/source/modsim.c \
	ports/quectel/core/source/modnet.c \
	ports/quectel/core/source/machine_iic.c \
	ports/quectel/core/source/modfota.c \
	ports/quectel/core/source/moddev.c \
	ports/quectel/core/source/modostimer.c \
	ports/quectel/core/source/machine_timer.c \
	ports/quectel/core/source/misc_powerkey.c \
	ports/quectel/core/source/modlpm.c \
	py/argcheck.c \
	py/asmarm.c \
	py/asmbase.c \
	py/asmthumb.c \
	py/asmx64.c \
	py/asmx86.c \
	py/asmxtensa.c \
	py/bc.c \
	py/binary.c \
	py/builtinevex.c \
	py/builtinhelp.c \
	py/builtinimport.c \
	py/compile.c \
	py/emitbc.c \
	py/emitcommon.c \
	py/emitglue.c \
	py/emitinlinethumb.c \
	py/emitinlinextensa.c \
	py/emitnarm.c \
	py/emitnthumb.c \
	py/emitnx64.c \
	py/emitnx86.c \
	py/emitnxtensa.c \
	py/emitnxtensawin.c \
	py/formatfloat.c \
	py/frozenmod.c \
	py/gc.c \
	py/lexer.c \
	py/malloc.c \
	py/map.c \
	py/modarray.c \
	py/modbuiltins.c \
	py/modcmath.c \
	py/modcollections.c \
	py/modgc.c \
	py/modio.c \
	py/modmath.c \
	py/modmicropython.c \
	py/modstruct.c \
	py/modsys.c \
	py/modthread.c \
	py/moduerrno.c \
	py/mpprint.c \
	py/mpstate.c \
	py/mpz.c \
	py/nativeglue.c \
	py/nlr.c \
	py/nlrpowerpc.c \
	py/nlrsetjmp.c \
	py/nlrthumb.c \
	py/nlrx64.c \
	py/nlrx86.c \
	py/nlrxtensa.c \
	py/obj.c \
	py/objarray.c \
	py/objattrtuple.c \
	py/objbool.c \
	py/objboundmeth.c \
	py/objcell.c \
	py/objclosure.c \
	py/objcomplex.c \
	py/objdeque.c \
	py/objdict.c \
	py/objenumerate.c \
	py/objexcept.c \
	py/objfilter.c \
	py/objfloat.c \
	py/objfun.c \
	py/objgenerator.c \
	py/objgetitemiter.c \
	py/objint.c \
	py/objint_longlong.c \
	py/objint_mpz.c \
	py/objlist.c \
	py/objmap.c \
	py/objmodule.c \
	py/objnamedtuple.c \
	py/objnone.c \
	py/objobject.c \
	py/objpolyiter.c \
	py/objproperty.c \
	py/objrange.c \
	py/objreversed.c \
	py/objset.c \
	py/objsingleton.c \
	py/objslice.c \
	py/objstr.c \
	py/objstringio.c \
	py/objstrunicode.c \
	py/objtuple.c \
	py/objtype.c \
	py/objzip.c \
	py/opmethods.c \
	py/pairheap.c \
	py/parse.c \
	py/parsenum.c \
	py/parsenumbase.c \
	py/persistentcode.c \
	py/profile.c \
	py/pystack.c \
	py/qstr.c \
	py/reader.c \
	py/repl.c \
	py/ringbuf.c \
	py/runtime.c \
	py/runtime_utils.c \
	py/scheduler.c \
	py/scope.c \
	py/sequence.c \
	py/showbc.c \
	py/smallint.c \
	py/stackctrl.c \
	py/stream.c \
	py/unicode.c \
	py/vm.c \
	py/vstr.c \
	py/warning.c


ifneq ($(strip $(PLAT)),Qualcomm)
$(NAME)_SRCS += \
	ports/quectel/core/source/machine_rtc.c
endif

ifeq ($(strip $(PLAT)),Qualcomm)
$(NAME)_SRCS += \
	extmod/vfs_efs.c \
	extmod/vfs_efs_file.c \
	extmod/modussl_mbedtls.c \
	extmod/mb_ussl_error.c
endif

ifeq ($(strip $(PLAT)),RDA)
$(NAME)_SRCS += \
	ports/quectel/core/source/modnb.c \
	ports/quectel/core/source/nb_oc.c \
	ports/quectel/core/source/nb_aep.c \
	ports/quectel/core/source/nb_onenet.c 
endif

ifneq ($(strip $(PLAT)),RDA)
ifneq ($(strip $(PLAT)),Qualcomm)
$(NAME)_SRCS += \
	extmod/modussl_mbedtls.c \
	extmod/mb_ussl_error.c \
	ports/quectel/core/source/modsms.c \
	ports/quectel/core/source/modvoicecall.c \
	ports/quectel/core/source/machine_lcd.c \
	ports/quectel/core/source/machine_wdt.c \
	ports/quectel/core/source/modcelllocator.c \
	ports/quectel/core/source/modwifiscan.c \
	ports/quectel/core/source/sensor_sn95500.c \
	ports/quectel/core/source/modsensor.c \
	ports/quectel/core/source/modaudio.c \
	ports/quectel/core/source/audio_audio.c \
	ports/quectel/core/source/audio_record.c \
    ports/quectel/core/source/misc_pwm.c \
    ports/quectel/core/source/misc_usb.c \
	ports/quectel/core/source/audio_queue.c \

ifeq ($(strip $(PLAT)),ASR)
$(NAME)_SRCS += \
	ports/quectel/core/source/modsecuredata.c \
	ports/quectel/core/source/misc_usbnet.c \
	ports/quectel/core/source/modslipif.c \
	ports/quectel/core/source/modethernet.c \
	ports/quectel/core/source/machine_keypad.c \
	ports/quectel/core/source/modethernet_dm9051.c
endif

endif
endif

ifeq ($(strip $(PLAT)),Unisoc)
$(NAME)_SRCS += \
	ports/quectel/core/source/modgnss.c
endif

ifeq ($(CONFIG_TTS), y)
$(NAME)_SRCS += \
	ports/quectel/core/source/audio_tts.c
endif

ifeq ($(CONFIG_BT), y)
$(NAME)_SRCS += \
	ports/quectel/core/source/modble.c
endif

ifeq ($(CONFIG_LVGL), y)
$(NAME)_SRCS += \
	ports/quectel/core/source/modlvgl.c
endif

ifeq ($(CONFIG_CAMERA), y)
$(NAME)_SRCS += \
	ports/quectel/core/source/modcamera.c \
	ports/quectel/core/source/camera_preview.c
endif

ifeq ($(CONFIG_SPINAND), y)
$(NAME)_SRCS += \
	ports/quectel/core/source/machine_nandflash.c
endif

ifeq ($(CONFIG_RTMP), y)
$(NAME)_SRCS += \
	ports/quectel/core/source/modrtmp.c
endif

ifeq ($(CONFIG_SPI_SDCARD), y)
$(NAME)_SRCS += \
	extmod/vfs_fat.c \
	extmod/vfs_fat_file.c 
endif

$(NAME)_INCS = \
	. \
	extmod \
	genhdr \
	lib \
	lib/littlefs \
	lib/mp-readline \
	lib/netutils \
	lib/timeutils \
	lib/utils \
	ports/quectel/core/source \
	py

EMPTY=
SPACE=$(EMPTY) $(EMPTY)
COMMA=$(EMPTY),$(EMPTY)
BUILD_TIMESTAMP=\\\"$(subst $(SPACE),_,$(subst $(COMMA),,$(shell echo $$(env LANG=en_US.UTF-8 date))))\\\"
$(NAME)_DEFINE = \
	MP_ENDIANNESS_LITTLE \
	MICROPY_BUILD_DATE=${BUILD_TIMESTAMP}



ifneq ($(strip $(PLAT)),Qualcomm)
$(NAME)_CFLAGS = \
	-Wno-error=unused-parameter \
	-Wno-error=format-truncation \
	-Wno-error=unused-variable \
	-Wno-error=unused-function \
	-Wno-error=format=
else
$(NAME)_CFLAGS = -Wno-error=tautological-constant-out-of-range-compare \
				 -Wno-error=missing-field-initializers \
				 -Wno-error=unused-parameter \
				 -Wno-error=unused-variable \
				 -Wno-error=unused-function \
				 -Wno-error=missing-braces \
				 -Wno-error=sometimes-uninitialized
endif


ifeq ($(CONFIG_LVGL), y)
$(NAME)_CFLAGS += \
	-Wno-error=incompatible-pointer-types \
	-Wno-error=sign-compare \
	-Wno-error=unused-but-set-variable \
	-Wno-error=switch-unreachable \
	-Wno-error=unused-function
endif

ifeq ($(CONFIG_SPINAND), y)
$(NAME)_CFLAGS += \
	-Wno-error=restrict \
	-Wno-error=format-overflow=
endif

$(NAME)_COMPONENTS = peripheral

ifeq ($(CONFIG_CAMERA_DECODE), y)
$(NAME)_COMPONENTS += components/ZBar
$(NAME)_SRCS += \
	ports/quectel/core/source/camera_scandecode.c

endif


ifeq ($(CONFIG_CAMERA_SAVEPHOTOS), y)
$(NAME)_SRCS += \
	ports/quectel/core/source/camera_capture.c
endif

ifeq ($(CONFIG_JPEG), y)
$(NAME)_COMPONENTS += components/jpeg
endif

$(NAME)_COMPONENTS += utilities

ifeq ($(CONFIG_LVGL), y)
$(NAME)_COMPONENTS += components/lvgl
endif

ifneq (,$(filter y,$(CONFIG_SPINAND) $(CONFIG_SPI_SDCARD)))
$(NAME)_COMPONENTS += components/fs
endif

ifeq ($(CONFIG_RTMP), y)
$(NAME)_COMPONENTS += components/rtmpdump
endif

ifeq ($(CONFIG_POC), y)
$(NAME)_CFLAGS += \
	-Wno-error=incompatible-pointer-types \
	-Wno-error=ignored-qualifiers \
	-Wno-error=int-conversion \
	-Wno-error=implicit-function-declaration \
	-Wno-error=discarded-qualifiers \
	-Wno-error=unused-const-variable=

$(NAME)_COMPONENTS += components/poc
$(NAME)_SRCS += \
	ports/quectel/core/source/modpoc.c
endif

ifeq ($(CONFIG_QRCODE), y)
$(NAME)_COMPONENTS += components/qrcode
$(NAME)_SRCS += \
	ports/quectel/core/source/modqrcode.c
endif



ifeq ($(CONFIG_QUECTHING), y)
ifeq ($(strip $(PLAT)),$(filter $(PLAT),ASR Unisoc))
$(NAME)_SRCS += ports/quectel/core/source/modquecIot.c
$(NAME)_INCS += ../../components/quecsdk
$(NAME)_COMPONENTS += components/quecsdk
endif
endif

$(NAME)_CFLAGS += -Wno-error=implicit-fallthrough

$(NAME)_PRIVATE_SCRIPT = private.mk

$(NAME)_PRIVATE_SCRIPT_TARGETS = construct clean
