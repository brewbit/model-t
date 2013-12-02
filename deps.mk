
CHIBIOS ?= ../ChibiOS-RT
SNACKA ?= ../snacka
NANOPB ?= ../nanopb


SNACKA_SRCDIR = $(SNACKA)/src
SNACKA_INCDIR = $(SNACKA_SRCDIR)
SNACKA_CSRC = \
       $(SNACKA_SRCDIR)/snacka/frame.c \
       $(SNACKA_SRCDIR)/snacka/frameheader.c \
       $(SNACKA_SRCDIR)/snacka/frameparser.c \
       $(SNACKA_SRCDIR)/snacka/logging.c \
       $(SNACKA_SRCDIR)/snacka/mutablestring.c \
       $(SNACKA_SRCDIR)/snacka/openinghandshakeparser.c \
       $(SNACKA_SRCDIR)/snacka/utf8.c \
       $(SNACKA_SRCDIR)/snacka/websocket.c \
       $(SNACKA_SRCDIR)/external/http_parser/http_parser.c


NANOPB_INCDIR = $(NANOPB)
NANOPB_CSRC = 

.EXPORT_ALL_VARIABLES:
