PROJECT = controller

BOARD = II-MT-CONTROLLER

PROJECT_INCDIR = util util/json

PROJECT_AUTOGEN_CSRC = \
       image_resources.c \
       font_resources.c

PROJECT_CSRC = \
       app_cfg.c \
       app_hdr.c \
       bapi.c \
       crc.c \
       datastream.c \
       debug_client.c \
       font.c \
       gfx.c \
       gui.c \
       gui_calib.c \
       gui_history.c \
       gui_home.c \
       gui_probe.c \
       gui_output.c \
       gui_output_function.c \
       gui_output_trigger.c \
       gui_output_delay.c \
       gui_settings.c \
       gui_update.c \
       gui_wifi.c \
       iflash.c \
       image.c \
       lcd.c \
       main.c \
       message.c \
       onewire.c \
       pid.c \
       temp_control.c \
       temp_input.c \
       temp_widget.c \
       touch.c \
       touch_calib.c \
       txn.c \
       wspr.c \
       wspr_http.c \
       wspr_net.c \
       wspr_parser.c \
       wspr_tcp.c \
       gui/button.c \
       gui/icon.c \
       gui/label.c \
       gui/progressbar.c \
       gui/widget.c \
       util/linked_list.c \
       util/json/cJSON.c

include make-bin.mk
