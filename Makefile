# DO NOT FORGET to define BITBOX environment variable 

USE_SDCARD = 1      # allow use of SD card for io
#DEFINES += DEBUG_CHIPTUNE

NAME = bored
# font files need to be first in order to be generated first:
C_FILES = font.c name.c tetris.c snake.c invaders.c io.c menu.c palette.c \
    chiptune.c verse.c instrument.c anthem.c main.c
H_FILES = font.h name.h tetris.h snake.h invaders.h io.h menu.h palette.h \
    chiptune.h verse.h instrument.h anthem.h common.h
GAME_C_OPTS += -DVGA_MODE=320

GAME_C_FILES = $(C_FILES:%=src/%)
GAME_H_FILES = $(H_FILES:%=src/%)

# see this file for options
include $(BITBOX)/kernel/bitbox.mk

destroy:
	rm -f RECENT16.TXT *.*16

very-clean: clean destroy
