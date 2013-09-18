PROJECT = bootloader

MAJOR_VERSION = 1
MINOR_VERSION = 0
PATCH_VERSION = 0

BOARD = II-MT-CONTROLLER

PROJECT_CSRC = \
       main.c \
       ../common/crc/crc32.c \
       ../common/iflash.c \
       ../common/xflash.c

include make-bin.mk
