##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#

# Compiler options here.
ifeq ($(USE_OPT),)
  USE_OPT = -Os -ggdb -fomit-frame-pointer -falign-functions=16
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

PROJECT_SRC_DIR = src/$(PROJECT)
BUILDDIR   = build/$(PROJECT)
AUTOGEN_DIR = $(BUILDDIR)/autogen

# Imported source files and paths
include board/board.mk
include $(CHIBIOS)/os/hal/platforms/STM32F2xx/platform.mk
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/ports/GCC/ARMCMx/STM32F2xx/port.mk
include $(CHIBIOS)/os/kernel/kernel.mk

# Define linker script file here
LDSCRIPT= $(PROJECT_SRC_DIR)/$(PROJECT).ld

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
       $(addprefix $(AUTOGEN_DIR)/,$(PROJECT_AUTOGEN_CSRC)) \
       $(addprefix $(PROJECT_SRC_DIR)/,$(PROJECT_CSRC)) \
       $(foreach dep,$(addsuffix _CSRC,$(DEPS)),$($(dep)))

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC = $(addprefix $(PROJECT_SRC_DIR)/,$(PROJECT_CPPSRC))

# C sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACSRC = $(addprefix $(PROJECT_SRC_DIR)/,$(PROJECT_ACSRC))

# C++ sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACPPSRC = $(addprefix $(PROJECT_SRC_DIR)/,$(PROJECT_ACPPSRC))

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCSRC = $(addprefix $(PROJECT_SRC_DIR)/,$(PROJECT_TCSRC))

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCPPSRC = $(addprefix $(PROJECT_SRC_DIR)/,$(PROJECT_TCPPSRC))

# List ASM source files here
ASMSRC = $(PORTASM) \
         $(addprefix $(PROJECT_SRC_DIR)/,$(PROJECT_ASMSRC))

INCDIR = $(PORTINC) $(KERNINC) \
         $(HALINC) $(PLATFORMINC) $(BOARDINC) \
         $(CHIBIOS)/os/various \
         $(foreach dep,$(addsuffix _INCDIR,$(DEPS)),$($(dep)))

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
DDEFS = -DSTDOUT_SD=SD3 -DSTDIN_SD=SD3 -DREQUIRE_PRINTF_FLOAT

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
UDEFS = -DMAJOR_VERSION=$(MAJOR_VERSION) \
        -DMINOR_VERSION=$(MINOR_VERSION) \
        -DPATCH_VERSION=$(PATCH_VERSION)

# Define ASM defines here
UADEFS =

# List all user directories here
UINCDIR = src/common \
          $(AUTOGEN_DIR) \
          $(PROJECT_SRC_DIR) \
          $(addprefix $(PROJECT_SRC_DIR)/,$(PROJECT_INCDIR))
          
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
	mkdir -p $@

$(AUTOGEN_DIR)/font_resources.c $(AUTOGEN_DIR)/font_resources.h: $(AUTOGEN_DIR) $(wildcard fonts/*.bmfc)
	python scripts/fontconv fonts $(AUTOGEN_DIR)

$(AUTOGEN_DIR)/image_resources.c $(AUTOGEN_DIR)/image_resources.h: $(AUTOGEN_DIR) $(wildcard images/*.png)
	python scripts/imgconv images/*.png


