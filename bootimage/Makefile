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
#                    build the boot image
# --------------------------------------------------------------------------

j820: config depend ramdisk
	export RDIMAGE=`pwd`/ramdisk/ramdiskimage.gz && \
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
