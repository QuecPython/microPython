# Copyright (c) Quectel Wireless Solution, Co., Ltd.All Rights Reserved.
#  
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#  
#     http://www.apache.org/licenses/LICENSE-2.0
#  
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#env
TOP := ../../..
ROOT := $(TOP)/../..

KCONFIG_CONFIG_DIR := ../../../../..
include $(KCONFIG_CONFIG_DIR)/config/$(KCONFIG_CONFIG)

ifeq ("$(origin V)", "command line")
BUILD_VERBOSE=$(V)
endif
ifndef BUILD_VERBOSE
BUILD_VERBOSE = 0
endif
# ifeq ($(BUILD_VERBOSE),0)
# Q = @
# else
# Q =
# endif

BUILD ?= build
FROZEN_MPY_DIR ?= .

RM = rm
ECHO = echo
CP = cp
MKDIR = mkdir
SED = sed
CAT = cat
TOUCH = touch

MAKE_MANIFEST = $(PYTHON) $(TOP)/tools/makemanifest.py
MAKE_FROZEN = $(PYTHON) $(TOP)/tools/make-frozen.py
#because E-safeNet, mpy-cross could not read the py file, rename as notepad++
ifeq ($(strip $(HOST)),linux)
MPY_CROSS = $(TOP)/ports/quectel/tools/mpy-cross
else
MPY_CROSS = $(TOP)/ports/quectel/tools/notepad++.exe
endif
MPY_TOOL = $(PYTHON) $(TOP)/tools/mpy-tool.py

QSTR_DEFS = source/qstrdefsport.h
#py
PY_SRC ?= $(TOP)/py
HEADER_BUILD = $(TOP)/genhdr
PY_QSTR_DEFS = $(PY_SRC)/qstrdefs.h
ifneq ($(QSTR_AUTOGEN_DISABLE),1)
QSTR_DEFS_COLLECTED = $(HEADER_BUILD)/qstrdefs.collected.h
endif
QSTR_GLOBAL_DEPENDENCIES += $(PY_SRC)/mpconfig.h source/mpconfigport.h
CSUPEROPT = -O3
MPY_CROSS_FLAGS += -march=armv7m
# Enable building 32-bit code on 64-bit host.
ifeq ($(MICROPY_FORCE_32BIT),1)
CC += -m32
CXX += -m32
LD += -m32
endif

# External modules written in C.
ifneq ($(USER_C_MODULES),)
# pre-define USERMOD variables as expanded so that variables are immediate
# expanded as they're added to them
SRC_USERMOD :=
C_FLAGS_USERMOD :=
LD_FLAGS_USERMOD :=
$(foreach module, $(wildcard $(USER_C_MODULES)/*/micropython.mk), \
    $(eval USERMOD_DIR = $(patsubst %/,%,$(dir $(module))))\
    $(info Including User C Module from $(USERMOD_DIR))\
	$(eval include $(module))\
)

SRC_MOD += $(patsubst $(USER_C_MODULES)/%.c,%.c,$(SRC_USERMOD))
C_FLAGS_MOD += $(C_FLAGS_USERMOD)
LD_FLAGS_MOD += $(LD_FLAGS_USERMOD)
endif

# py object files
PY_CORE_O_BASENAME = $(addprefix py/,\
	mpstate.o \
	nlr.o \
	nlrx86.o \
	nlrx64.o \
	nlrthumb.o \
	nlrpowerpc.o \
	nlrxtensa.o \
	nlrsetjmp.o \
	malloc.o \
	gc.o \
	pystack.o \
	qstr.o \
	vstr.o \
	mpprint.o \
	unicode.o \
	mpz.o \
	reader.o \
	lexer.o \
	parse.o \
	scope.o \
	compile.o \
	emitcommon.o \
	emitbc.o \
	asmbase.o \
	asmx64.o \
	emitnx64.o \
	asmx86.o \
	emitnx86.o \
	asmthumb.o \
	emitnthumb.o \
	emitinlinethumb.o \
	asmarm.o \
	emitnarm.o \
	asmxtensa.o \
	emitnxtensa.o \
	emitinlinextensa.o \
	emitnxtensawin.o \
	formatfloat.o \
	parsenumbase.o \
	parsenum.o \
	emitglue.o \
	persistentcode.o \
	runtime.o \
	runtime_utils.o \
	scheduler.o \
	nativeglue.o \
	pairheap.o \
	ringbuf.o \
	stackctrl.o \
	argcheck.o \
	warning.o \
	profile.o \
	map.o \
	obj.o \
	objarray.o \
	objattrtuple.o \
	objbool.o \
	objboundmeth.o \
	objcell.o \
	objclosure.o \
	objcomplex.o \
	objdeque.o \
	objdict.o \
	objenumerate.o \
	objexcept.o \
	objfilter.o \
	objfloat.o \
	objfun.o \
	objgenerator.o \
	objgetitemiter.o \
	objint.o \
	objint_longlong.o \
	objint_mpz.o \
	objlist.o \
	objmap.o \
	objmodule.o \
	objobject.o \
	objpolyiter.o \
	objproperty.o \
	objnone.o \
	objnamedtuple.o \
	objrange.o \
	objreversed.o \
	objset.o \
	objsingleton.o \
	objslice.o \
	objstr.o \
	objstrunicode.o \
	objstringio.o \
	objtuple.o \
	objtype.o \
	objzip.o \
	opmethods.o \
	sequence.o \
	stream.o \
	binary.o \
	builtinimport.o \
	builtinevex.o \
	builtinhelp.o \
	modarray.o \
	modbuiltins.o \
	modcollections.o \
	modgc.o \
	modio.o \
	modmath.o \
	modcmath.o \
	modmicropython.o \
	modstruct.o \
	modsys.o \
	moduerrno.o \
	modthread.o \
	vm.o \
	bc.o \
	showbc.o \
	repl.o \
	smallint.o \
	frozenmod.o \
	)

PY_EXTMOD_O_BASENAME = \
	extmod/moductypes.o \
	extmod/modujson.o \
	extmod/modure.o \
	extmod/moduzlib.o \
	extmod/moduheapq.o \
	extmod/modutimeq.o \
	extmod/modubinascii.o \
	extmod/virtpin.o \
	extmod/machine_mem.o \
	extmod/machine_pinbase.o \
	extmod/machine_signal.o \
	extmod/machine_pulse.o \
	extmod/machine_i2c.o \
	extmod/machine_spi.o \
	extmod/modbluetooth.o \
	extmod/modurandom.o \
	extmod/moduselect.o \
	extmod/moduasyncio.o \
	extmod/moduwebsocket.o \
	extmod/modwebrepl.o \
	extmod/modframebuf.o \
	extmod/vfs.o \
	extmod/vfs_blockdev.o \
	extmod/vfs_reader.o \
	extmod/vfs_posix.o \
	extmod/vfs_posix_file.o \
	extmod/vfs_fat.o \
	extmod/vfs_fat_file.o \
	extmod/vfs_lfs.o \
	extmod/utime_mphal.o \
	extmod/uos_dupterm.o \
	extmod/moduhashlib.o \
	extmod/moducryptolib.o \
	lib/embed/abort_.o \
	lib/utils/printf.o \
	lib/utils/sys_stdio_mphal.o

ifeq ($(strip $(PLAT)),Qualcomm)
PY_EXTMOD_O_BASENAME += \
	extmod/vfs_efs.o \
	extmod/vfs_efs_file.o \
	extmod/modussl_mbedtls.o
endif

ifneq ($(strip $(PLAT)),RDA)
ifneq ($(strip $(PLAT)),Qualcomm)
PY_EXTMOD_O_BASENAME += \
	extmod/modussl_axtls.o \
	extmod/modussl_mbedtls.o 

endif
endif

# Sources that may contain qstrings
SRC_QSTR_IGNORE = py/nlr%
SRC_QSTR += $(SRC_MOD) $(filter-out $(SRC_QSTR_IGNORE),$(PY_CORE_O_BASENAME:.o=.c)) $(PY_EXTMOD_O_BASENAME:.o=.c)

ifneq ($(strip $(PLAT)),Qualcomm)
SRC_QSTR += source/quecpython.c \
			source/moddev.c \
			source/moduos.c \
			source/modexample.c \
			source/modsha1.c \
			source/modutils.c \
            source/utils_crc32.c \
            source/modostimer.c \
            source/modmachine.c \
            source/machine_pin.c \
            source/machine_timer.c \
            source/machine_rtc.c \
			source/machine_uart.c \
			source/modutime.c \
			source/modmisc.c \
            source/misc_power.c \
            source/misc_powerkey.c \
            source/misc_adc.c \
            source/modsocket.c \
			source/modfota.c \
            source/modnet.c \
            source/moddatacall.c \
			source/modsim.c \
			source/modlpm.c \
			source/machine_extint.c \
			source/machine_hw_spi.c \
			source/machine_iic.c \
			source/machine_iic_simulation.c \
			source/callbackdeal.c \
			source/devreport.c
else
SRC_QSTR += source/quecpython.c \
			source/moduos.c \
			source/modexample.c \
			source/modsha1.c \
			source/modutils.c \
            source/utils_crc32.c \
			source/modostimer.c \
			source/modutime.c \
			source/modmachine.c \
			source/machine_uart.c \
			source/machine_pin.c \
			source/machine_timer.c \
			source/modsocket.c \
			source/modfota.c \
			source/moddatacall.c \
			source/machine_extint.c \
			source/machine_hw_spi.c	\
			source/modmisc.c \
            source/misc_power.c \
            source/misc_adc.c \
            source/modsim.c \
            source/modnet.c \
			source/machine_iic.c \
			source/machine_iic_simulation.c \
			source/moddev.c \
			source/misc_powerkey.c \
			source/modlpm.c \
			source/callbackdeal.c \
			source/devreport.c

endif
			
ifeq ($(strip $(PLAT)),ASR)
SRC_QSTR += source/modsecuredata.c \
			source/misc_usbnet.c \
			source/modslipif.c \
			source/modethernet.c \
			source/modethernet_dm9051.c
endif

ifeq ($(strip $(PLAT)),RDA)
SRC_QSTR += source/modnb.c \
            source/nb_oc.c \
			source/nb_aep.c \
			source/nb_onenet.c \
			source/misc_temperature.c
endif

ifneq ($(strip $(PLAT)),RDA)
ifneq ($(strip $(PLAT)),Qualcomm)
SRC_QSTR += source/modsms.c \
			source/machine_wdt.c \
			source/machine_lcd.c \
			source/modcelllocator.c \
			source/modwifiscan.c \
			source/modaudio.c \
			source/audio_audio.c \
			source/audio_record.c \
			source/audio_queue.c \
			source/sensor_sn95500.c \
			source/modsensor.c \
            source/misc_usb.c \
            source/misc_pwm.c \
            source/modwifilocator.c \
			source/modvoicecall.c \
			source/machine_keypad.c

#quecthing
ifeq ($(CONFIG_QUECTHING), y)
ifeq ($(strip $(PLAT)),$(filter $(PLAT),ASR Unisoc))
SRC_QSTR += $(ROOT)/components/quecsdk/cloud/python/modquecIot.c
endif
endif

#spinand
ifeq ($(CONFIG_SPINAND), y)
SRC_QSTR += \
			source/machine_nandflash.c
endif

#qrcode
ifeq ($(CONFIG_QRCODE), y)
SRC_QSTR += \
			source/modqrcode.c
endif

#camera
ifeq (y,$(CONFIG_CAMERA))
SRC_QSTR += \
			source/modcamera.c \
			source/camera_preview.c			
ifeq ($(CONFIG_CAMERA_SAVEPHOTOS), y)
SRC_QSTR +=  \
			source/camera_capture.c
endif
ifeq (y,$(CONFIG_CAMERA_DECODE))
SRC_QSTR += \
			source/camera_scandecode.c
endif
endif

#lvgl
ifeq (y,$(CONFIG_LVGL))
SRC_QSTR += \
			source/modlvgl.c
endif

endif
endif

ifeq ($(strip $(PLAT)),Unisoc)
SRC_QSTR += source/modgnss.c \
			source/misc_usbnet.c
endif

ifeq ($(CONFIG_BT), y)
SRC_QSTR += source/modble.c
endif

ifeq (y,$(CONFIG_POC))
SRC_QSTR += \
			source/modpoc.c
endif

ifeq ($(CONFIG_RTMP), y)
SRC_QSTR += source/modrtmp.c
endif

ifeq ($(CONFIG_TTS), y)
SRC_QSTR += source/audio_tts.c
endif

$(BUILD)/%.pp: %.c
	@$(ECHO) "PreProcess $<"
	$(Q)$(CPP) $(C_FLAGS) -Wp,-C,-dD,-dI -o $@ $<

# Anything that depends on FORCE will be considered out-of-date
FORCE:
.PHONY: FORCE

$(HEADER_BUILD):
	$(MKDIR) -p $@

#$(HEADER_BUILD)/mpversion.h: FORCE | $(HEADER_BUILD)
#	$(Q)$(PYTHON) $(PY_SRC)/makeversionhdr.py $@
# mpconfigport.mk is optional, but changes to it may drastically change
# overall config, so they need to be caught
MPCONFIGPORT_MK = $(wildcard mpconfigport.mk)

# qstr data
# Adding an order only dependency on $(HEADER_BUILD) causes $(HEADER_BUILD) to get
# created before we run the script to generate the .h
# Note: we need to protect the qstr names from the preprocessor, so we wrap
# the lines in "" and then unwrap after the preprocessor is finished.
# See more information about this process in docs/develop/qstr.rst.
$(HEADER_BUILD)/qstrdefs.generated.h: $(PY_QSTR_DEFS) $(QSTR_DEFS) $(QSTR_DEFS_COLLECTED) $(PY_SRC)/makeqstrdata.py source/mpconfigport.h $(MPCONFIGPORT_MK) $(PY_SRC)/mpconfig.h | $(HEADER_BUILD)
	@$(ECHO) "GEN $@"
	$(Q)$(CAT) $(PY_QSTR_DEFS) $(QSTR_DEFS) $(QSTR_DEFS_COLLECTED) | $(SED) 's/^Q(.*)/"&"/' | $(CPP) $(C_FLAGS) - | $(SED) 's/^\"\(Q(.*)\)\"/\1/' > $(HEADER_BUILD)/qstrdefs.preprocessed.h
	$(Q)$(PYTHON) $(PY_SRC)/makeqstrdata.py $(HEADER_BUILD)/qstrdefs.preprocessed.h > $@

# build a list of registered modules for py/objmodule.c.
$(HEADER_BUILD)/moduledefs.h: $(SRC_QSTR) $(QSTR_GLOBAL_DEPENDENCIES) | $(HEADER_BUILD)
	@$(ECHO) "GEN $@"
	$(Q)$(PYTHON) $(PY_SRC)/makemoduledefs.py --vpath="., $(TOP), $(USER_C_MODULES)" $(SRC_QSTR) > $@

SRC_QSTR += $(HEADER_BUILD)/moduledefs.h

ifeq ($(strip $(PLAT)),Qualcomm)
INC += -I$(COMPILER_PATH)/armv7m-none-eabi/libc/include
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/azure_api
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/azure_api/c-utility
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/azure_api/c-utility/inc
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/azure_api/c-utility/inc/azure_c_shared_utility
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/azure_api/c-utility/pal
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/azure_api/c-utility/pal/generic
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/azure_api/iothub_client
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/azure_api/iothub_client/inc
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/azure_api/iothub_client/inc/internal
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/azure_api/serializer
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/azure_api/serializer/inc
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/azure_api/umqtt
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/azure_api/umqtt/inc
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/azure_api/umqtt/inc/azure_umqtt_c
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/qapi
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include/threadx_api
endif

INC += -I.
INC += -I$(HEADER_BUILD)
INC += -I$(TOP)
INC += -I$(TOP)/py
INC += -I$(TOP)/extmod
INC += -I$(TOP)/lib/utils
INC += -I$(TOP)/lib/mp-readline
INC += -I$(TOP)/lib/littlefs
INC += -I$(TOP)/lib/netutils
INC += -I$(TOP)/lib/timeutils
INC += -I$(TOP)/ports/quectel/core
INC += -I$(TOP)/ports/quectel/core/source
INC += -I$(TOP)/ports/quectel/core/py
INC += -I$(ROOT)/peripheral
INC += -I$(ROOT)/system/include
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/include
INC += -I$(ROOT)/system/platform/$(strip $(PLAT))/boards/$(strip $(BOARD))/include
INC += -I$(ROOT)/system/debug
INC += -I$(ROOT)/system/dev
INC += -I$(ROOT)/system/fs
INC += -I$(ROOT)/system/hal
INC += -I$(ROOT)/system/gnss
ifeq ($(strip $(PLAT)),Unisoc)
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/include
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/lwip
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/lwip/apps
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/lwip/priv
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/lwip/prot
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/netif
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/posix
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/posix/arpa
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/posix/net
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/posix/netinet
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/posix/sys
else
ifeq ($(strip $(PLAT)),RDA)
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/include
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/include/arch
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/lwip
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/lwip/apps
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/lwip/priv
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/lwip/prot
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/netif
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/posix
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/posix/arpa
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/posix/net
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/posix/netinet
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/src/include/posix/sys
else
ifeq ($(strip $(PLAT)),ASR)
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/include
else
INC += -I$(ROOT)/system/lwip/$(strip $(PLAT))/include
endif
endif
endif
ifeq (y, $(CONFIG_MBEDTLS))
INC += -I$(ROOT)/system/mbedtls
INC += -I$(ROOT)/system/mbedtls/include
INC += -I$(ROOT)/system/mbedtls/include/mbedtls
INC += -I$(ROOT)/system/mbedtls/library
INC += -I$(ROOT)/system/mbedtls/port/helios/inc
endif
INC += -I$(ROOT)/system/network
INC += -I$(ROOT)/system/os
INC += -I$(ROOT)/system/startup
INC += -I$(ROOT)/system/fota
INC += -I$(ROOT)/system/ble
ifeq (y, $(CONFIG_LVGL))
INC += -I$(ROOT)/components/lvgl
INC += -I$(ROOT)/components/lvgl/porting
INC += -I$(ROOT)/components/lvgl/src
INC += -I$(ROOT)/components/lvgl/src/lv_core
INC += -I$(ROOT)/components/lvgl/src/lv_draw
INC += -I$(ROOT)/components/lvgl/src/lv_font
INC += -I$(ROOT)/components/lvgl/src/lv_hal
INC += -I$(ROOT)/components/lvgl/src/lv_misc
INC += -I$(ROOT)/components/lvgl/src/lv_objx
INC += -I$(ROOT)/components/lvgl/src/lv_themes
INC += -I$(ROOT)/components/lvgl/src/lv_lib_png
endif
ifeq (y, $(CONFIG_QRCODE))
INC += -I$(ROOT)/components/qrcode
endif
ifeq (y, $(CONFIG_QUECTHING))
INC += -I$(ROOT)/components/quecsdk/cloud
INC += -I$(ROOT)/components/quecsdk/driverLayer 
INC += -I$(ROOT)/components/quecsdk/thirdLib/mqtt 
INC += -I$(ROOT)/components/quecsdk/thirdLib/cJSON 
INC += -I$(ROOT)/components/quecsdk/kernel 
endif
ifeq ($(CONFIG_SPINAND), y)
INC += -I$(ROOT)/components/fs/include
INC += -I$(ROOT)/components/fs/yaffs/src/direct
INC += -I$(ROOT)/components/fs/yaffs/src/port
INC += -I$(ROOT)/components/fs/yaffs/src
INC += -I$(ROOT)/components/fs/yaffs
endif
ifeq ($(CONFIG_RTMP), y)
INC += -I$(ROOT)/components/rtmpdump/librtmp/inc
INC += -I$(ROOT)/components/rtmpdump/libz/inc
endif
INC += -I$(ROOT)/utilities
ifeq ($(CONFIG_JPEG), y)
INC += -I$(ROOT)/components/jpeg/jpeg-9c
INC += -I$(ROOT)/components/jpeg
endif
ifeq ($(CONFIG_POC_BND), y)
INC += -I$(ROOT)/components/poc
INC += -I$(ROOT)/components/poc/bnd/inc
endif
ifeq ($(CONFIG_POC_SL), y)
INC += -I$(ROOT)/components/poc
INC += -I$(ROOT)/components/poc/sl/inc
endif
ifeq (y,$(CONFIG_CAMERA_DECODE))
INC += -I$(ROOT)/components/ZBar/zbar
INC += -I$(ROOT)/components/ZBar/include
INC += -I$(ROOT)/components/ZBar/zbar/decoder
INC += -I$(ROOT)/components/ZBar/zbar/qrcode
endif
ifeq ($(CONFIG_SPI_SDCARD), y)
INC += -I$(ROOT)/components/fs/fatfs/include
endif

# compiler settings
CWARN = -Wall -Werror
CWARN += -Wpointer-arith -Wuninitialized
C_FLAGS = $(INC) $(CWARN) -std=gnu99 $(C_FLAGS_MOD) $(COPT) $(C_FLAGS_EXTRA)
C_FLAGS += -fdata-sections -ffunction-sections -fno-asynchronous-unwind-tables --include=../../../../../../config/autoconf.h

LD_FLAGS_ARCH = -Wl,-Map=$@.map,--cref -Wl,--gc-sections
LD_FLAGS = $(LD_FLAGS_MOD) $(LD_FLAGS_ARCH) -lm $(LD_FLAGS_EXTRA)

D_FLAGS += $(foreach dflag, $(DFLAGS), -D$(dflag))
D_FLAGS += -DMP_CONFIGFILE="\"ports/quectel/core/source/mpconfigport.h\"" \
		  -DMICROPY_MPTHREADPORT_H="\"ports/quectel/core/source/mpthreadport.h\"" \
		  -DMICROPY_MPHALPORT_H="\"ports/quectel/core/source/mphalport.h\""

vpath %.S . $(TOP) $(USER_C_MODULES)
$(BUILD)/%.o: %.S
	@$(ECHO) "CC $<"
	$(Q)$(CC) $(C_FLAGS) $(D_FLAGS) -c -o $@ $<

vpath %.s . $(TOP) $(USER_C_MODULES)
$(BUILD)/%.o: %.s
	@$(ECHO) "AS $<"
	$(Q)$(AS) -o $@ $<
QSTR_GEN_EXTRA_C_FLAGS += -DNO_QSTR
vpath %.c . $(TOP) $(USER_C_MODULES)
$(BUILD)/%.pp: %.c
	@$(ECHO) "PreProcess $<"
	$(Q)$(CPP) $(C_FLAGS) $(D_FLAGS) -Wp,-C,-dD,-dI -o $@ $<

#$(OBJ): | $(HEADER_BUILD)/qstrdefs.generated.h $(HEADER_BUILD)/mpversion.h


$(HEADER_BUILD)/qstr.i.last: $(SRC_QSTR) $(QSTR_GLOBAL_DEPENDENCIES) | $(QSTR_GLOBAL_REQUIREMENTS)
	@$(ECHO) "GEN $@"
	@rm -f $@
	@for src in $(if $(filter $?,$(QSTR_GLOBAL_DEPENDENCIES)),$^,$(if $?,$?,$^)); \
	do \
		echo processing on $$src ...; \
		$(CPP) $(QSTR_GEN_EXTRA_C_FLAGS) $(C_FLAGS) $(D_FLAGS) $$src >> $@; \
	done

$(HEADER_BUILD)/qstr.split: $(HEADER_BUILD)/qstr.i.last
	@$(ECHO) "GEN $@"
	$(Q)$(PYTHON) $(PY_SRC)/makeqstrdefs.py split qstr $(HEADER_BUILD)/qstr.i.last $(HEADER_BUILD)/qstr $(QSTR_DEFS_COLLECTED)
	$(Q)$(TOUCH) $@

$(QSTR_DEFS_COLLECTED): $(HEADER_BUILD)/qstr.split
	@$(ECHO) "GEN $@"
	$(Q)$(PYTHON) $(PY_SRC)/makeqstrdefs.py cat qstr $(HEADER_BUILD)/qstr.i.last $(HEADER_BUILD)/qstr $(QSTR_DEFS_COLLECTED)


$(BUILD)/%.mpy: $(FROZEN_MPY_DIR)/%.py
	@$(ECHO) "MPY $<"
	$(Q)$(MKDIR) -p $(dir $@)
	$(Q)$(MPY_CROSS) -o $@ -s $(<:$(FROZEN_MPY_DIR)/%=%) $(MPY_CROSS_FLAGS) $<

ifeq ($(strip $(PLAT)),RDA)
$(BUILD)/_frozen_mpy.c: $(BUILD)/_boot_RDA.mpy \
						$(BUILD)/system.mpy \
						$(BUILD)/dataCall.mpy \
						$(BUILD)/umqtt.mpy \
						$(BUILD)/sys_bus.mpy \
						$(BUILD)/queue.mpy \
						$(BUILD)/log.mpy \
						$(HEADER_BUILD)/qstrdefs.generated.h
	@$(ECHO) "MISC freezing bytecode"
	$(Q)$(MPY_TOOL) -f -q \
						$(HEADER_BUILD)/qstrdefs.preprocessed.h \
						$(BUILD)/_boot_RDA.mpy \
						$(BUILD)/system.mpy \
						$(BUILD)/dataCall.mpy \
						$(BUILD)/umqtt.mpy \
						$(BUILD)/sys_bus.mpy \
						$(BUILD)/queue.mpy \
						$(BUILD)/log.mpy \
						> $@

else
ifeq ($(strip $(PLAT)),Qualcomm)
$(BUILD)/_frozen_mpy.c: $(BUILD)/_boot_Qualcomm.mpy \
						$(BUILD)/system.mpy \
						$(BUILD)/dataCall.mpy \
						$(BUILD)/umqtt.mpy \
						$(BUILD)/log.mpy \
						$(BUILD)/request.mpy \
						$(HEADER_BUILD)/qstrdefs.generated.h
	@$(ECHO) "MISC freezing bytecode"
	$(Q)$(MPY_TOOL) -f -q \
						$(HEADER_BUILD)/qstrdefs.preprocessed.h \
						$(BUILD)/_boot_Qualcomm.mpy \
						$(BUILD)/system.mpy \
						$(BUILD)/dataCall.mpy \
						$(BUILD)/umqtt.mpy \
						$(BUILD)/log.mpy \
						$(BUILD)/request.mpy \
						> $@

else
$(BUILD)/_frozen_mpy.c: \
						$(BUILD)/_boot.mpy \
						$(BUILD)/umqtt.mpy \
						$(BUILD)/ntptime.mpy \
						$(BUILD)/request.mpy \
						$(BUILD)/log.mpy \
						$(BUILD)/aLiYun.mpy \
						$(BUILD)/hmac.mpy \
						$(BUILD)/hashlib.mpy \
						$(BUILD)/base64.mpy \
						$(BUILD)/TenCentYun.mpy \
						$(BUILD)/file_sha256.mpy \
						$(BUILD)/checksum.mpy \
						$(BUILD)/checkNet.mpy \
						$(BUILD)/dataCall.mpy \
						$(BUILD)/ql_fs.mpy \
						$(BUILD)/app_fota_download.mpy\
						$(BUILD)/app_fota_updater.mpy \
						$(BUILD)/app_fota.mpy \
						$(BUILD)/backup_restore.mpy \
						$(BUILD)/file_crc32.mpy \
						$(BUILD)/system.mpy \
						$(BUILD)/gnss.mpy \
						$(BUILD)/unzip.mpy \
						$(BUILD)/queue.mpy \
						$(BUILD)/sys_bus.mpy \
						$(BUILD)/event_message.mpy \
						$(BUILD)/hls.mpy \
						$(BUILD)/wifilocator.mpy \
						$(BUILD)/bak_util.mpy \
						$(HEADER_BUILD)/qstrdefs.generated.h
	@$(ECHO) "MISC freezing bytecode"
	$(Q)$(MPY_TOOL) -f -q \
						$(HEADER_BUILD)/qstrdefs.preprocessed.h \
						$(BUILD)/_boot.mpy \
						$(BUILD)/umqtt.mpy \
						$(BUILD)/ntptime.mpy \
						$(BUILD)/request.mpy \
						$(BUILD)/log.mpy \
						$(BUILD)/aLiYun.mpy \
						$(BUILD)/hmac.mpy \
						$(BUILD)/hashlib.mpy \
						$(BUILD)/base64.mpy \
						$(BUILD)/TenCentYun.mpy \
						$(BUILD)/file_sha256.mpy \
						$(BUILD)/checksum.mpy \
						$(BUILD)/checkNet.mpy \
						$(BUILD)/dataCall.mpy \
						$(BUILD)/ql_fs.mpy \
						$(BUILD)/app_fota_download.mpy \
						$(BUILD)/app_fota_updater.mpy \
						$(BUILD)/app_fota.mpy \
						$(BUILD)/file_crc32.mpy \
						$(BUILD)/backup_restore.mpy \
						$(BUILD)/gnss.mpy \
						$(BUILD)/system.mpy \
						$(BUILD)/unzip.mpy \
						$(BUILD)/queue.mpy \
						$(BUILD)/sys_bus.mpy \
						$(BUILD)/event_message.mpy \
						$(BUILD)/hls.mpy \
                                                $(BUILD)/wifilocator.mpy \
						$(BUILD)/bak_util.mpy \
						> $@
endif
endif
$(BUILD):
	$(Q)echo creating directory: $@
	-$(Q)$(MKDIR) $@

all:$(BUILD) $(HEADER_BUILD)/qstrdefs.generated.h $(HEADER_BUILD)/moduledefs.h  $(BUILD)/_frozen_mpy.c

clean:
	@echo ----- cleaning microPython private target
	@rm -rf $(HEADER_BUILD)
	@rm -rf $(BUILD)
	@echo ----- done.
