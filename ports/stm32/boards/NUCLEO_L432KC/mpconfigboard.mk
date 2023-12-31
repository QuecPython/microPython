MCU_SERIES = l4
CMSIS_MCU = STM32L432xx
AF_FILE = boards/stm32l432_af.csv
LD_FILES = boards/stm32l432.ld boards/common_basic.ld
OPENOCD_CONFIG = boards/openocd_stm32l4.cfg

# MicroPython settings
MICROPY_VFS_FAT = 0

# Don't include default frozen modules because MCU is tight on flash space
FROZEN_MANIFEST ?=
