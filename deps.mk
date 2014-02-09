
CHIBIOS   ?= ../ChibiOS-RT
SNACKA    ?= ../snacka
NANOPB    ?= ../nanopb
BBMT_MSGS ?= ../brewbit-protobuf-messages

DEPENDENCIES = $(CHIBIOS) $(SNACKA) $(NANOPB) $(BBMT_MSGS)

######################################
#              SNACKA                #
######################################
SNACKA_SRCDIR = $(SNACKA)/src
SNACKA_INCDIR = $(SNACKA_SRCDIR)
SNACKA_DEFS = -DPB_FIELD_16BIT
SNACKA_CSRC = \
	$(SNACKA_SRCDIR)/snacka/frame.c \
	$(SNACKA_SRCDIR)/snacka/frameheader.c \
	$(SNACKA_SRCDIR)/snacka/frameparser.c \
	$(SNACKA_SRCDIR)/snacka/logging.c \
	$(SNACKA_SRCDIR)/snacka/mutablestring.c \
	$(SNACKA_SRCDIR)/snacka/openinghandshakeparser.c \
	$(SNACKA_SRCDIR)/snacka/utf8.c \
	$(SNACKA_SRCDIR)/snacka/websocket.c \
	$(SNACKA_SRCDIR)/external/base64/base64.c \
	$(SNACKA_SRCDIR)/external/http_parser/http_parser.c \
	$(SNACKA_SRCDIR)/external/sha1/sha1.c

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

