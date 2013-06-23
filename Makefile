##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#

# Compiler options here.
ifeq ($(USE_OPT),)
  USE_OPT = -O2 -ggdb -fomit-frame-pointer -falign-functions=16
endif

# C specific options here (added to USE_OPT).
ifeq ($(USE_COPT),)
  USE_COPT = 
endif

# C++ specific options here (added to USE_OPT).
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti
endif

# Enable this if you want the linker to remove unused code and data
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif

# If enabled, this option allows to compile the application in THUMB mode.
ifeq ($(USE_THUMB),)
  USE_THUMB = yes
endif

# Enable this if you want to see the full log while compiling.
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif

#
# Build global options
##############################################################################

##############################################################################
# Architecture or project specific options
#

# Enable this if you really want to use the STM FWLib.
ifeq ($(USE_FWLIB),)
  USE_FWLIB = no
endif

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, sources and paths
#

# Define project name here
PROJECT = ch


BUILD_DIR   = build
AUTOGEN_DIR = $(BUILD_DIR)/autogen

# Imported source files and paths
CHIBIOS = ../ChibiOS
include board/board.mk
include $(CHIBIOS)/os/hal/platforms/STM32F2xx/platform.mk
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/ports/GCC/ARMCMx/STM32F2xx/port.mk
include $(CHIBIOS)/os/kernel/kernel.mk

# Define linker script file here
LDSCRIPT= board/STM32F205xE.ld

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC = $(PORTSRC) \
       $(KERNSRC) \
       $(HALSRC) \
       $(PLATFORMSRC) \
       $(BOARDSRC) \
       $(CHIBIOS)/os/various/evtimer.c \
       $(CHIBIOS)/os/various/syscalls.c \
       $(CHIBIOS)/os/various/chprintf.c \
       $(AUTOGEN_DIR)/image_resources.c \
       $(AUTOGEN_DIR)/font_resources.c \
       src/app_hdr.c \
       src/bapi.c \
       src/crc.c \
       src/datastream.c \
       src/font.c \
       src/gfx.c \
       src/gui.c \
       src/gui_calib.c \
       src/gui_home.c \
       src/gui_probe.c \
       src/gui_output.c \
       src/gui_settings.c \
       src/image.c \
       src/lcd.c \
       src/main.c \
       src/message.c \
       src/onewire.c \
       src/settings.c \
       src/temp_control.c \
       src/temp_input.c \
       src/temp_widget.c \
       src/touch.c \
       src/touch_calib.c \
       src/txn.c \
       src/wspr.c \
       src/wspr_http.c \
       src/wspr_parser.c \
       src/wspr_tcp.c \
       src/gui/button.c \
       src/gui/icon.c \
       src/gui/label.c \
       src/gui/widget.c \
       src/util/json/cJSON.c

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC =

# C sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACSRC =

# C++ sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACPPSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCPPSRC =

# List ASM source files here
ASMSRC = $(PORTASM)

INCDIR = $(PORTINC) $(KERNINC) \
         $(HALINC) $(PLATFORMINC) $(BOARDINC) \
         $(CHIBIOS)/os/various

#
# Project, sources and paths
##############################################################################

##############################################################################
# Compiler settings
#

MCU  = cortex-m3

#TRGT = arm-elf-
TRGT = arm-none-eabi-
CC   = $(TRGT)gcc
CPPC = $(TRGT)g++
# Enable loading with g++ only if you need C++ runtime support.
# NOTE: You can use C++ even without C++ support if you are careful. C++
#       runtime support makes code size explode.
LD   = $(TRGT)gcc
#LD   = $(TRGT)g++
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
OD   = $(TRGT)objdump
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary

# ARM-specific options here
AOPT =

# THUMB-specific options here
TOPT = -mthumb -DTHUMB

# Define C warning options here
CWARN = -Wall -Wextra -Wstrict-prototypes

# Define C++ warning options here
CPPWARN = -Wall -Wextra

#
# Compiler settings
##############################################################################

##############################################################################
# Start of default section
#

# List all default C defines here, like -D_DEBUG=1
DDEFS =

# List all default ASM defines here, like -D_DEBUG=1
DADEFS =

# List all default directories to look for include files here
DINCDIR =

# List the default directory to look for the libraries here
DLIBDIR =

# List all default libraries here
DLIBS = -lm

#
# End of default section
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
UDEFS =

# Define ASM defines here
UADEFS =

# List all user directories here
UINCDIR = src src/util/json $(AUTOGEN_DIR)

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

#
# End of user defines
##############################################################################

ifeq ($(USE_FWLIB),yes)
  include $(CHIBIOS)/ext/stm32lib/stm32lib.mk
  CSRC += $(STM32SRC)
  INCDIR += $(STM32INC)
  USE_OPT += -DUSE_STDPERIPH_DRIVER
endif

include $(CHIBIOS)/os/ports/GCC/ARMCMx/rules.mk

$(AUTOGEN_DIR):
	mkdir $@

$(AUTOGEN_DIR)/font_resources.c $(AUTOGEN_DIR)/font_resources.h: $(AUTOGEN_DIR) $(wildcard fonts/*.bmfc)
	python scripts/fontconv fonts/*.bmfc

$(AUTOGEN_DIR)/image_resources.c $(AUTOGEN_DIR)/image_resources.h: $(AUTOGEN_DIR) $(wildcard images/*.png)
	python scripts/imgconv images/*.png

download: $(BUILD_DIR)/ch.elf
	@openocd \
	-f interface/$(JTAG).cfg \
	-f target/stm32f2x.cfg \
	-f stm32f2x-setup.cfg \
	-c "flash write_image erase build/ch.elf" \
	-c shutdown download.log 2>&1 && \
	echo Download complete

run:
	@echo Resetting target
	@sleep 1
	@openocd \
	-f interface/$(JTAG).cfg \
	-f target/stm32f2x.cfg \
	-f stm32f2x-setup.cfg \
	-c "reset init" \
	-c "reset run" \
	-c shutdown download.log 2>&1 && \
	echo Target running
	