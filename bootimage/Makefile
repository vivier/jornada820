OS			= linux
TARGET			= arm-linux
TOPDIR			= $(shell pwd)
PATH:=$(TOPDIR)/gcc-3.3.x/toolchain_arm/bin:$(PATH)

export PATH

.PHONY: ramdisk

# ------------- programs ---------------------------------------------------

LN			= ln
MKDIR			= mkdir
TEE			= tee
TOUCH			= touch
CP			= cp -f
RM			= rm -f

# ------------- directories for source code ----------------------------

KERNELDIR		= linux/kernel
KBUILDDIR		= linux/build/kernel
KUPSTREAMDIR		= linux/upstream/kernel

# --------------------------------------------------------------------------
#                       default targets                                    |
# --------------------------------------------------------------------------
all:: j820

localclean::
	-${RM} vmlinux j820

clean:: localclean
	${MAKE} -C ${KBUILDDIR} ARCH=arm clean
	${MAKE} -C ramdisk clean

distclean:: localclean
	${MAKE} -C ${KBUILDDIR} ARCH=arm mrproper
	${MAKE} -C ramdisk ARCH=arm mrproper

kbuildtree:: $(KUPSTREAMDIR)/Makefile
	cp -fal $(KUPSTREAMDIR) linux/build/
	cp -fal $(KERNELDIR) linux/build/

$(KUPSTREAMDIR)/CVS/Root $(KUPSTREAMDIR)/CVS/Repository:
	mkdir -p $(KUPSTREAMDIR)/CVS ; cd $(KUPSTREAMDIR)/CVS || exit 2 ; \
	echo :pserver:anoncvs@cvs.handhelds.org:/cvs > Root ; \
	echo linux/kernel > Repository ; \
	touch Entries ; \
	grep -q ':pserver:anoncvs@cvs.handhelds.org:2401/cvs' ~/.cvspass || \
	echo '/1 :pserver:anoncvs@cvs.handhelds.org:2401/cvs Ay=0=h<Z' >> ~/.cvspass

$(KUPSTREAMDIR)/Makefile: $(KUPSTREAMDIR)/CVS/Root
	cd $(KUPSTREAMDIR) && cvs -z9 update -dfP &&
	touch $(KUPSTREAMDIR)/Makefile

# -------------------------------------------------------------------------- 
#                    optional: build the toolchain
# --------------------------------------------------------------------------

gcc_version:
	${TARGET}-gcc -v

gcc-3.3.x/CVS/Root gcc-3.3.x/CVS/Repository:
	mkdir -p gcc-3.3.x/CVS ; cd gcc-3.3.x/CVS || exit 2 ; \
	echo :pserver:anonymous@uclibc.org:/var/cvs > Root ; \
	echo toolchain/gcc-3.3.x > Repository ; \
	touch Entries ; \
	grep -q ':pserver:anonymous@uclibc.org:2401/var/cvs' ~/.cvspass || \
	echo '/1 :pserver:anonymous@uclibc.org:2401/var/cvs A' >> ~/.cvspass

gcc-3.3.x/Makefile: gcc-3.3.x/CVS/Root
	cd gcc-3.3.x && cvs -z9 update -dfP

gcc-3.3.x/toolchain_arm/bin/arm-linux-gcc: gcc-3.3.x/Makefile
	cd gcc-3.3.x && ( echo 2 ; echo 9 ; echo y ) | make ARCH=arm

toolchain: gcc-3.3.x/toolchain_arm/bin/arm-linux-gcc

# -------------------------------------------------------------------------- 
#                    build the boot image
# --------------------------------------------------------------------------

j820: config depend ramdisk kbuildtree
	export RDIMAGE=$(TOPDIR)/ramdisk/ramdiskimage.gz && \
	cd ${KBUILDDIR} && \
	${MAKE} \
	    ARCH=arm \
	    CROSS_COMPILE=${TARGET}- \
	    INITRD=$${RDIMAGE} \
	    zImage j820
	cp ${KBUILDDIR}/arch/arm/boot/j820/j820 $@

sandisk: j820
	mount /mnt/sandisk/; cp j820 /mnt/sandisk/; umount /mnt/sandisk/

# -------------------------------------------------------------------------- 
#                  Initial configuration and dependences
# --------------------------------------------------------------------------


menuconfig: kbuildtree
	cd ${KBUILDDIR} && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- menuconfig

depend: config ${KBUILDDIR}/.depend
${KBUILDDIR}/.depend:
	cd ${KBUILDDIR} && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- depend

config: ${KBUILDDIR}/.config
${KBUILDDIR}/.config: kbuildtree
	cd ${KBUILDDIR} && \
	cp arch/arm/def-configs/jornada820 .config && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- oldconfig

# --------------------------------------------------------------------------
#                     build loadable modules
# --------------------------------------------------------------------------

modules: config depend ramdisk/modules.tar.bz2

ramdisk/modules.tar.bz2: ramdisk/lib/modules
	cd ramdisk && tar jcf modules.tar.bz2 lib/modules

ramdisk/lib/modules: kbuildtree
	export MODDIR=$(TOPDIR)/ramdisk/ && \
	echo $${MODDIR} && \
	cd ${KBUILDDIR} && \
	${MAKE} ARCH=arm \
	        CROSS_COMPILE=${TARGET}- \
		INSTALL_MOD_PATH=$${MODDIR} \
		DEPMOD=true \
	        modules modules_install

# --------------------------------------------------------------------------
#                     build ramdisk image
# --------------------------------------------------------------------------

ramdisk: modules
	${MAKE} -C ramdisk TARGET=${TARGET} OS=${OS}
