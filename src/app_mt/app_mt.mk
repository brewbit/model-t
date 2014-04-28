PROJECT = app_mt

MAJOR_VERSION = 1
MINOR_VERSION = 0
PATCH_VERSION = 0

WEB_API_HOST = dg.brewbit.com
WEB_API_PORT = 31337

BOARD = II-MT-CONTROLLER

DEPS = NANOPB

PROJECT_INCDIR = \
       gui \
       gui/controls \
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
       image.c \
       lcd.c \
       main.c \
       message.c \
       net.c \
       onewire.c \
       ota_update.c \
       pid.c \
       quantity_widget.c \
       sensor.c \
       sntp.c \
       temp_control.c \
       temp_profile.c \
       thread_watchdog.c \
       touch.c \
       touch_calib.c \
       web_api.c \
       gui/gui.c \
       gui/activation.c \
       gui/button_list.c \
       gui/calib.c \
       gui/history.c \
       gui/info.c \
       gui/home.c \
       gui/output_settings.c \
       gui/quantity_select.c \
       gui/recovery.c \
       gui/controller_settings.c \
       gui/settings.c \
       gui/textentry.c \
       gui/update.c \
       gui/wifi.c \
       gui/wifi_scan.c \
       gui/self_test.c \
       wifi/core/cc3000_common.c \
       wifi/core/cc3000_spi.c \
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
       gui/controls/button.c \
       gui/controls/icon.c \
       gui/controls/label.c \
       gui/controls/listbox.c \
       gui/controls/progressbar.c \
       gui/controls/scatter_plot.c \
       gui/controls/widget.c \
       util/linked_list.c \
       ../common/bootloader_api.c \
       ../common/crc/crc8.c \
       ../common/crc/crc16.c \
       ../common/crc/crc32.c \
       ../common/iflash.c \
       ../common/xflash.c \
       ../common/dfuse.c \
       ../common/sxfs.c

include make-bin.mk
