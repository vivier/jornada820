#
# linux/arch/arm/boot/bootp/Makefile
#

ZIMAGE		=$(TOPDIR)/arch/arm/boot/zImage
ZLDFLAGS	= -p -X -T j820.lds \
		 --defsym initrd_addr=$(INITRD_PHYS) \
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

clean:;		$(RM) j820 *.o
