#
# linux/arch/arm/boot/j820/Makefile
#
extra-$(CONFIG_SA1100_JORNADA820) := j820

ZIMAGE		= arch/arm/boot/zImage
INITRD		= ../../../ramdisk/ramdiskimage.gz
# Override the above default if not using the jornada820.sf.net CVS.

ZLDFLAGS	= -p -X -T j820.lds -N -s \
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
