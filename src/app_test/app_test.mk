PROJECT = app_test

MAJOR_VERSION = 1
MINOR_VERSION = 0
PATCH_VERSION = 0

BOARD = II-MT-CONTROLLER

PROJECT_INCDIR = \
       ../app_mt/gui \
       ../app_mt/gui/controls \
       ../app_mt/util \
       ../app_mt/wifi \
       ../app_mt

PROJECT_AUTOGEN_CSRC = \
       image_resources.c \
       font_resources.c

PROJECT_CSRC = \
       main.c \
       ../app_mt/gfx.c \
       ../app_mt/lcd.c \
       ../app_mt/touch.c \
       ../app_mt/touch_calib.c \
       ../app_mt/gui/gui.c \
       ../app_mt/gui/controls/widget.c \
       ../app_mt/message.c \
       ../app_mt/app_cfg.c \
       ../app_mt/sensor.c \
       ../app_mt/onewire.c \
       ../common/crc/crc8.c \
       ../app_mt/net.c \
       ../app_mt/thread_watchdog.c \
       ../app_mt/wifi/core/cc3000_common.c \
       ../app_mt/wifi/core/cc3000_spi.c \
       ../app_mt/wifi/core/hci.c \
       ../app_mt/wifi/core/c_netapp.c \
       ../app_mt/wifi/core/c_nvmem.c \
       ../app_mt/wifi/core/c_security.c \
       ../app_mt/wifi/core/c_socket.c \
       ../app_mt/wifi/core/c_wlan.c \
       ../app_mt/wifi/netapp.c \
       ../app_mt/wifi/nvmem.c \
       ../app_mt/wifi/patch.c \
       ../app_mt/wifi/security.c \
       ../app_mt/wifi/socket.c \
       ../app_mt/wifi/wlan.c \
       ../app_mt/wifi/font.c

include make-bin.mk
