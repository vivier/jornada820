#
# linux/arch/arm/boot/j820/Makefile
#
# $Id: Makefile,v 1.3 2004/06/28 12:40:44 fare Exp $
#
extra-$(CONFIG_SA1100_JORNADA820) := j820

ZIMAGE		= arch/arm/boot/zImage
INITRD		= ../../../ramdisk/ramdiskimage.gz
# Override the above default if not using the jornada820.sf.net CVS.

ZLDFLAGS	= -p -X -T $(obj)/j820.lds -N -s \
		 --defsym params=$(PARAMS_PHYS)

# Note that bootp.lds picks up kernel.o and initrd.o
$(obj)/j820:	$(addprefix $(obj)/,kernel.o initrd.o init.o j820.lds)
		$(LD) $(ZLDFLAGS) -o $@ $(obj)/init.o

$(obj)/kernel.o:	$(ZIMAGE)
		$(LD) -r -s -o $@ -b binary $(ZIMAGE)

$(obj)/initrd.o:       $(INITRD)
		$(LD) -r -s -o $@ -b binary $(INITRD)

.PHONY:		$(INITRD)

clean:;		$(RM) $(obj)/j820
