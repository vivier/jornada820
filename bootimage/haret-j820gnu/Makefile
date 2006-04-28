#
# Handheld Reverse Engineering Tool
#
# This program is free software distributed under the terms of the
# GNU GPL
#
# This Makefile requires GNU Make.
#

# Target operating system
TARGET = gnuwince

# Program version
VERSION=0.3.6-11x0gnu

COMPILEDATE = -$(shell date +"%Y%m%d%H%M")

# Output directory
OUT=gnuout/

include $(TARGET).smak

CXXFLAGS.DEF+=-DVERSION=\"$(VERSION)\"

#
vpath %.cpp src src/$(TARGET)

all: dep $(OUT) $(OUT)gnuharet11x0$E #$(OUT)gnuharethelper$E # $(OUT)linboot2$E

clean:
	rm -rf $(OUT) *~ src/*~ src/wince/*~ include/*~ docs/*~ *.bak

dep: $(wildcard src/*.cpp src/*/*.cpp)
	makedepend  $(CXXFLAGS.DEF) $(CXXFLAGS.INC) -p'$$(OUT)'  $^

$(OUT)gnuharet11x0$E: $(addprefix $(OUT),haret.o haret-res.o \
  s-cpu.o s-util.o memory.o gpio.o uart.o \
  asmstuff.o irqchain.o getsetcp.o irq.o \
  util.o output.o script.o network.o cpu.o terminal.o video.o linboot.o toolhelp.lib powerm.lib winsock.lib)
	$(LINK)

$(OUT)gnuharethelper$E: $(addprefix $(OUT),harethelper.o )
	$(LINK)

$(OUT)linboot2$E: $(addprefix $(OUT),linboot2.o )
	$(LINK)

# wince resources
$(OUT)haret-res.rc: include/resource.h

$(OUT):
	mkdir $@

# DO NOT DELETE this line -- makedep finds dependencies by it
