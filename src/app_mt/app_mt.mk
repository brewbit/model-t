PROJECT = app_mt

MAJOR_VERSION = 1
MINOR_VERSION = 0
PATCH_VERSION = 0

WEB_API_HOST = 192.168.1.146
WEB_API_PORT = 3000

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
       gui.c \
       gui_activation.c \
       gui_calib.c \
       gui_history.c \
       gui_info.c \
       gui_home.c \
       gui_output.c \
       gui_output_function.c \
       gui_output_mode.c \
       gui_output_trigger.c \
       gui_quantity_select.c \
       gui_recovery.c \
       gui_sensor_settings.c \
       gui_settings.c \
       gui_textentry.c \
       gui_update.c \
       gui_wifi.c \
       gui_wifi_scan.c \
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
       touch.c \
       touch_calib.c \
       watchdog.c \
       web_api.c \
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
       gui/button.c \
       gui/icon.c \
       gui/label.c \
       gui/listbox.c \
       gui/progressbar.c \
       gui/scatter_plot.c \
       gui/widget.c \
       util/linked_list.c \
       ../common/crc/crc8.c \
       ../common/crc/crc16.c \
       ../common/crc/crc32.c \
       ../common/iflash.c \
       ../common/xflash.c \
       ../common/sxfs.c

include make-bin.mk
