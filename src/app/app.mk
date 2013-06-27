PROJECT = app

PROJECT_AUTOGEN_CSRC = \
       image_resources.c \
       font_resources.c

PROJECT_CSRC = \
       app_cfg.c \
       app_hdr.c \
       bapi.c \
       crc.c \
       datastream.c \
       font.c \
       gfx.c \
       gui.c \
       gui_calib.c \
       gui_home.c \
       gui_probe.c \
       gui_output.c \
       gui_settings.c \
       gui_update.c \
       iflash.c \
       image.c \
       lcd.c \
       main.c \
       message.c \
       onewire.c \
       temp_control.c \
       temp_input.c \
       temp_widget.c \
       touch.c \
       touch_calib.c \
       txn.c \
       wspr.c \
       wspr_http.c \
       wspr_parser.c \
       wspr_tcp.c \
       gui/button.c \
       gui/icon.c \
       gui/label.c \
       gui/progressbar.c \
       gui/widget.c \
       util/json/cJSON.c

include make-bin.mk
