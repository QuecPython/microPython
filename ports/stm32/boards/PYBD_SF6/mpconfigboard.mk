# MCU settings
MCU_SERIES = f7
CMSIS_MCU = STM32F767xx
MICROPY_FLOAT_IMPL = double
AF_FILE = boards/stm32f767_af.csv
LD_FILES = boards/PYBD_SF6/f767.ld
TEXT0_ADDR = 0x08008000

# MicroPython settings
MICROPY_PY_BLUETOOTH ?= 1
MICROPY_BLUETOOTH_NIMBLE ?= 1
MICROPY_BLUETOOTH_BTSTACK ?= 0
MICROPY_PY_LWIP = 1
MICROPY_PY_NETWORK_CYW43 = 1
MICROPY_PY_USSL = 1
MICROPY_SSL_MBEDTLS = 1
MICROPY_VFS_LFS2 = 1

# PYBD-specific frozen modules
FROZEN_MANIFEST = boards/PYBD_SF2/manifest.py
