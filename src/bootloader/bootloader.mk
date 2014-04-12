PROJECT = bootloader

MAJOR_VERSION = 1
MINOR_VERSION = 0
PATCH_VERSION = 0

BOARD = II-MT-CONTROLLER

PROJECT_CSRC = \
       bootloader_api.c \
       main.c \
       ../common/crc/crc32.c \
       ../common/iflash.c \
       ../common/xflash.c \
       ../common/sxfs.c \
       ../common/dfuse.c

include make-bin.mk
