# List of all the board related files.
BOARDSRC = board/$(BOARD)/board.c \
           board/$(BOARD)/newlib_support.c \
           board/$(BOARD)/system.c

# Required include directories
BOARDINC = board/$(BOARD)
