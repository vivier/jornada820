OS			= linux
TARGET			= arm-linux
TOPDIR			= $(shell pwd)
PATH:=$(TOPDIR)/gcc-3.3.x/toolchain_arm/bin:$(PATH)

export PATH

.PHONY: ramdisk force

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
KERNELDIR26		= linux/kernel26
KBUILDDIR26		= linux/build/kernel26
KUPSTREAMDIR26		= linux/upstream/kernel26

# --------------------------------------------------------------------------
#                       default targets                                    |
# --------------------------------------------------------------------------
help::
	@$${PAGER:-cat} HELP

all:: j820

localclean::
	-${RM} vmlinux j820

clean:: localclean
	${MAKE} -C ${KBUILDDIR} ARCH=arm clean
	${MAKE} -C ramdisk clean

distclean:: localclean
	${MAKE} -C ${KBUILDDIR} ARCH=arm mrproper
	${MAKE} -C ramdisk ARCH=arm mrproper

# --------------------------------------------------------------------------
#                       kernel build tree                                  |
# --------------------------------------------------------------------------

kbuildtree:: $(KUPSTREAMDIR)/Makefile
	cp -fal $(KUPSTREAMDIR) linux/build/
	cp -fal $(KERNELDIR) linux/build/

ourtree:
	cp -fal $(KERNELDIR) linux/build/

$(KUPSTREAMDIR)/CVS/Root $(KUPSTREAMDIR)/CVS/Repository:
	mkdir -p $(KUPSTREAMDIR)/CVS ; cd $(KUPSTREAMDIR)/CVS || exit 2 ; \
	echo :pserver:anoncvs@cvs.handhelds.org:/cvs > Root ; \
	echo linux/kernel > Repository ; \
	touch Entries ; \
	grep -q ':pserver:anoncvs@cvs.handhelds.org:2401/cvs' ~/.cvspass || \
	echo '/1 :pserver:anoncvs@cvs.handhelds.org:2401/cvs Ay=0=h<Z' >> ~/.cvspass

$(KUPSTREAMDIR)/Makefile: $(KUPSTREAMDIR)/CVS/Root
	cd $(KUPSTREAMDIR) && cvs -z9 update -dfP && \
	touch Makefile

# --------------------------------------------------------------------------
#                       kernel build tree, v2.6                            |
# --------------------------------------------------------------------------

kbuildtree26:: $(KUPSTREAMDIR26)/Makefile
	cp -fal $(KUPSTREAMDIR26) linux/build/
	cp -fal $(KERNELDIR26) linux/build/

ourtree26:
	cp -fal $(KERNELDIR26) linux/build/

$(KUPSTREAMDIR26)/CVS/Root $(KUPSTREAMDIR26)/CVS/Repository:
	mkdir -p $(KUPSTREAMDIR26)/CVS ; cd $(KUPSTREAMDIR26)/CVS || exit 2 ; \
	echo :pserver:anoncvs@cvs.handhelds.org:/cvs > Root ; \
	echo linux/kernel26 > Repository ; \
	touch Entries ; \
	grep -q ':pserver:anoncvs@cvs.handhelds.org:2401/cvs' ~/.cvspass || \
	echo '/1 :pserver:anoncvs@cvs.handhelds.org:2401/cvs Ay=0=h<Z' >> ~/.cvspass

$(KUPSTREAMDIR26)/Makefile: $(KUPSTREAMDIR26)/CVS/Root
	cd $(KUPSTREAMDIR26) && cvs -z9 update -dfP && \
	touch Makefile

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

j820: config depend ramdisk do-j820
do-j820:
	export RDIMAGE=$(TOPDIR)/ramdisk/ramdiskimage.gz && \
	cd ${KBUILDDIR} && \
	${MAKE} \
	    ARCH=arm \
	    CROSS_COMPILE=${TARGET}- \
	    INITRD=$${RDIMAGE} \
	    zImage j820
	cp ${KBUILDDIR}/arch/arm/boot/j820/j820 j820

sandisk: j820
	mount /mnt/sandisk/; cp j820 /mnt/sandisk/; umount /mnt/sandisk/
sandisk-fare:
	( mount /dev/sda1 /mnt && ( ls -l j820-26; cp -f j820-26 /mnt ; umount /mnt ) ; echo -e '\007' )

j820-26: config26 modules26 ramdisk26 do-j820-26
do-j820-26:
	export RDIMAGE=$(TOPDIR)/ramdisk/ramdiskimage26.gz && \
	cd ${KBUILDDIR26} && \
	${MAKE} \
	    ARCH=arm \
	    CROSS_COMPILE=${TARGET}- \
	    INITRD=$${RDIMAGE} \
	    zImage j820
	cp ${KBUILDDIR26}/arch/arm/boot/j820/j820 j820-26


# -------------------------------------------------------------------------- 
#                  Initial configuration and dependences
# --------------------------------------------------------------------------

config: kbuildtree do-config
do-config: ${KBUILDDIR}/.config
${KBUILDDIR}/.config: force
	cd ${KBUILDDIR} && \
	cp arch/arm/def-configs/jornada820 .config && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- oldconfig && \
	( cmp -s .config arch/arm/def-configs/jornada820 || \
	cat .config > arch/arm/def-configs/jornada820 )

menuconfig: kbuildtree do-menuconfig
do-menuconfig: force
	cd ${KBUILDDIR} && \
	cp arch/arm/def-configs/jornada820 .config && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- menuconfig && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- oldconfig && \
	( cmp -s .config arch/arm/def-configs/jornada820 || \
	cat .config > arch/arm/def-configs/jornada820 )

depend: config do-depend
do-depend: ${KBUILDDIR}/.depend
${KBUILDDIR}/.depend: force
	cd ${KBUILDDIR} && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- depend

menuconfig26: kbuildtree26 do-menuconfig26
do-menuconfig26: force
	cd ${KBUILDDIR26} && \
	cp arch/arm/def-configs/jornada820 .config && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- menuconfig && \
	( cmp -s .config arch/arm/def-configs/jornada820 || \
	cat .config > arch/arm/def-configs/jornada820 )

config26: kbuildtree26 do-config26
do-config26: ${KBUILDDIR26}/.config
${KBUILDDIR26}/.config: force
	cd ${KBUILDDIR26} && \
	cp arch/arm/def-configs/jornada820 .config && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- oldconfig && \
	( cmp -s .config arch/arm/def-configs/jornada820 || \
	cat .config > arch/arm/def-configs/jornada820 )

# --------------------------------------------------------------------------
#                     build loadable modules
# --------------------------------------------------------------------------

modules: config depend do-modules

do-modules: ramdisk/modules.tar.bz2

ramdisk/modules.tar.bz2: ramdisk/lib/modules
	cd ramdisk && tar jcf modules.tar.bz2 lib/modules

ramdisk/lib/modules: force
	export MODDIR=$(TOPDIR)/ramdisk/ && \
	echo $${MODDIR} && \
	cd ${KBUILDDIR} && \
	${MAKE} ARCH=arm \
	        CROSS_COMPILE=${TARGET}- \
		INSTALL_MOD_PATH=$${MODDIR} \
		DEPMOD=true \
	        modules modules_install

modules26: config26 do-modules26

do-modules26: ramdisk/modules26.tar.bz2

ramdisk/modules26.tar.bz2: $(KBUILDDIR26)/target/lib/modules
	cd $(KBUILDDIR26)/target && \
	tar jcf modules.tar.bz2 lib/modules && \
	ln -f modules.tar.bz2 $(TOPDIR)/ramdisk/modules26.tar.bz2

$(KBUILDDIR26)/target/lib/modules: force
	export MODDIR=$(TOPDIR)/$(KBUILDDIR26)/target/ && \
	mkdir -p $${MODDIR} && \
	echo $${MODDIR} && \
	cd ${KBUILDDIR26} && \
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

ramdisk26: modules26
	${MAKE} -C ramdisk TARGET=${TARGET} OS=${OS} ramdisk26
