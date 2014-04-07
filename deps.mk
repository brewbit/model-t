
CHIBIOS   ?= ../ChibiOS-RT
NANOPB    ?= ../nanopb
BBMT_MSGS ?= ../brewbit-protobuf-messages

DEPENDENCIES = $(CHIBIOS) $(NANOPB) $(BBMT_MSGS)

######################################
#              NANOPB                #
######################################
NANOPB_INCDIR = $(NANOPB)
NANOPB_CSRC = \
	$(NANOPB)/pb_encode.c \
	$(NANOPB)/pb_decode.c

######################################
#             BBMT_MSGS              #
######################################
BBMT_MSGS_INCLUDES = -I$(BBMT_MSGS) -I$(NANOPB)/generator -I/usr/include

UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
  BBMT_MSGS_INCLUDES += -I/usr/local/include
endif

.EXPORT_ALL_VARIABLES:

