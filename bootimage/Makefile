OS			= linux
TARGET			= arm-linux

.PHONY: ramdisk

# ------------- programs ---------------------------------------------------

LN			= ln
MKDIR			= mkdir
TEE			= tee
TOUCH			= touch
CP			= cp -f
RM			= rm -f
CANONICAL_PATH		= /bin:/usr/bin

# ------------- directories for source code ----------------------------

KERNELDIR		= linux/kernel

# --------------------------------------------------------------------------
#                       default targets                                    |
# --------------------------------------------------------------------------
all:: j820

localclean::
	-${RM} vmlinux j820

clean:: localclean
	${MAKE} -C ${KERNELDIR} ARCH=arm clean
	${MAKE} -C ramdisk clean

distclean:: localclean
	${MAKE} -C ${KERNELDIR} ARCH=arm mrproper
	${MAKE} -C ramdisk ARCH=arm mrproper


# -------------------------------------------------------------------------- 
#                    build the toolchain
# --------------------------------------------------------------------------

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

j820: config depend ramdisk
	export RDIMAGE=`pwd`/ramdisk/ramdiskimage.gz && \
	export PATH=`pwd`/gcc-3.3.x/toolchain_arm/bin:${PATH} && \
	cd ${KERNELDIR} && \
	${MAKE} \
	    ARCH=arm \
	    CROSS_COMPILE=${TARGET}- \
	    INITRD=$${RDIMAGE} \
	    zImage j820
	cp ${KERNELDIR}/arch/arm/boot/j820/j820 $@

sandisk: j820
	mount /mnt/sandisk/; cp j820 /mnt/sandisk/; umount /mnt/sandisk/

# -------------------------------------------------------------------------- 
#                  Initial configuration and dependences
# --------------------------------------------------------------------------


menuconfig:
	cd ${KERNELDIR} && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- menuconfig

depend: config ${KERNELDIR}/.depend
${KERNELDIR}/.depend:
	cd ${KERNELDIR} && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- depend

config: ${KERNELDIR}/.config
${KERNELDIR}/.config:
	cd ${KERNELDIR} && \
	cp arch/arm/def-configs/jornada820 .config && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- oldconfig

# --------------------------------------------------------------------------
#                     build loadable modules
# --------------------------------------------------------------------------

modules: config depend ramdisk/modules.tar.bz2

ramdisk/modules.tar.bz2: ramdisk/lib/modules
	cd ramdisk && tar jcf modules.tar.bz2 lib/modules

ramdisk/lib/modules:
	export MODDIR=`pwd`/ramdisk/ && \
	echo $${MODDIR} && \
	cd ${KERNELDIR} && \
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
