PROJECT = app_test

MAJOR_VERSION = 1
MINOR_VERSION = 0
PATCH_VERSION = 0

WEB_API_HOST = dg.brewbit.com
WEB_API_PORT = 80

BOARD = II-MT-CONTROLLER

PROJECT_INCDIR = \
       gui \
       gui/controls \
       util \
       wifi

#PROJECT_AUTOGEN_CSRC = \
#       image_resources.c \
#       font_resources.c \
#       bbmt.pb.c

PROJECT_CSRC = \
       main.c

include make-bin.mk
