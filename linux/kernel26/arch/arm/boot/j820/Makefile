#
# linux/arch/arm/boot/j820/Makefile
#
# $Id: Makefile,v 1.4 2005/07/25 09:09:11 fare Exp $
#
ifeq ($(CONFIG_SA1100_JORNADA820),y)

extra-y := j820

include $(srctree)/arch/arm/mach-sa1100/Makefile.boot

ZIMAGE		= arch/arm/boot/zImage
INITRD		= ../../../ramdisk/ramdiskimage.gz
# Override the above default if not using the jornada820.sf.net CVS.

ZLDFLAGS	= -p -X -T $(obj)/j820.lds -N -s \
		 --defsym params=$(params_phys-y)

# Note that bootp.lds picks up kernel.o and initrd.o
$(obj)/j820:	$(addprefix $(obj)/,kernel.o initrd.o init.o j820.lds)
		$(LD) $(ZLDFLAGS) -o $@ $(obj)/init.o

$(obj)/kernel.o:	$(ZIMAGE)
		$(LD) -r -s -o $@ -b binary $(ZIMAGE)

$(obj)/initrd.o:       $(INITRD)
		$(LD) -r -s -o $@ -b binary $(INITRD)

.PHONY:		$(INITRD)

clean:;		$(RM) $(obj)/j820

endif
