#
# linux/arch/arm/boot/bootp/Makefile
#

ZIMAGE		=$(TOPDIR)/arch/arm/boot/zImage
INITRD		=$(TOPDIR)/../../ramdisk/ramdiskimage.gz
# Override the above default if not using the jornada820.sf.net CVS.

ZLDFLAGS	= -p -X -T j820.lds -N -s \
		 --defsym params=$(PARAMS_PHYS)

all:		j820

# Note that bootp.lds picks up kernel.o and initrd.o
j820:		kernel.o initrd.o init.o j820.lds
		$(LD) $(ZLDFLAGS) -o $@ init.o

kernel.o:	$(ZIMAGE)
		$(LD) -r -s -o $@ -b binary $(ZIMAGE)

initrd.o:       $(INITRD)
		$(LD) -r -s -o $@ -b binary $(INITRD)

.PHONY:		$(INITRD)

clean:;		$(RM) j820 j820.gz *.o
