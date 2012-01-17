# Somehow, ensure here that we're using a cross-compiler
# producing arm-wince-pe binaries...

CC = arm-linux-gcc
CFLAGS = -Os -fomit-frame-pointer -W -Wall -mapcs-32 -b arm-wince-pe
LD = arm-linux-ld
LDFLAGS = --oformat arm-wince-pe -s -N

TARGETBIN = j820boot.exe

# TODO: I had to make sure all sources are at the current directory, fix this
# in the future maybe.
# (You may want to symlink asmstuff.asm and asm.asm)

SRCS = \
    gpio.c	    	\
    memory.c	    	\
    uart.c	    	\
    boot.c	    	\
    asmstuff.S	\
    asm.S		\

OBJS =  boot.o asm.o asmstuff.o memory.o gpio.o uart.o

all:	$(TARGETBIN)

$(TARGETBIN): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
