MCU_SERIES = m4
MCU_VARIANT = nrf52
MCU_SUB_VARIANT = nrf52832
SOFTDEV_VERSION = 6.1.1
LD_FILES += boards/nrf52832_512k_64k.ld
FLASHER = idap

NRF_DEFINES += -DNRF52832_XXAA
