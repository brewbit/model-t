PROJECT = app_test

MAJOR_VERSION = 1
MINOR_VERSION = 0
PATCH_VERSION = 0

WEB_API_HOST = staging.brewbit.com
WEB_API_PORT = 80

BOARD = II-MT-CONTROLLER

DEPS = SNACKA NANOPB

PROJECT_INCDIR = \
       util \
       wifi

PROJECT_AUTOGEN_CSRC = \
       image_resources.c \
       font_resources.c \
       bbmt.pb.c

PROJECT_CSRC = \
       app_cfg.c \
       app_hdr.c \
       fault.c \
       font.c \
       gfx.c \
       lcd.c \
       main.c \
       message.c \
       net.c \
       onewire.c \
       sensor.c \
       touch.c \
       snacka_backend/cryptocallbacks_chibios.c \
       snacka_backend/iocallbacks_socket.c \
       snacka_backend/socket_bsd.c \
       wifi/core/cc3000_common.c \
       wifi/core/cc3000_spi.c \
       wifi/core/evnt_handler.c \
       wifi/core/hci.c \
       wifi/core/c_netapp.c \
       wifi/core/c_nvmem.c \
       wifi/core/c_security.c \
       wifi/core/c_socket.c \
       wifi/core/c_wlan.c \
       wifi/netapp.c \
       wifi/nvmem.c \
       wifi/patch.c \
       wifi/security.c \
       wifi/socket.c \
       wifi/wlan.c \
       util/linked_list.c \
       ../common/crc/crc8.c \
       ../common/crc/crc16.c \
       ../common/crc/crc32.c \
       ../common/iflash.c \
       ../common/xflash.c \
       ../common/sxfs.c

include make-bin.mk
