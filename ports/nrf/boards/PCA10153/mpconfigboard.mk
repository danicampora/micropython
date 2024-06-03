MCU_SERIES = m33
MCU_VARIANT = nrf91
MCU_SUB_VARIANT = nrf9160
LD_FILES += boards/nrf9161_1M_256k.ld

NRF_DEFINES += -DNRF9160_XXAA -DNRF_TRUSTZONE_NONSECURE

MICROPY_VFS_LFS2 = 1
