OS		= linux
TARGET		= arm-linux-uclibc
TOPDIR		= $(shell pwd)
BUILDROOT	= ${TOPDIR}/uclibc/buildroot
TOOLCHAINPATH	= ${BUILDROOT}/build_arm/staging_dir/bin
PATH:=${TOOLCHAINPATH}:${PATH}
export PATH

EXTRAVERSION	= -j820

KERNEL_FTP	= ftp://ftp.bz2.fr.kernel.org/pub/linux/kernel
ARM_FTP		= ftp://ftp.arm.linux.org.uk/pub/armlinux/source/kernel-patches
JORNADA7XX	= http://wwwcip.informatik.uni-erlangen.de/~simigern/jornada-7xx/linux-2.4.31-j720-4/

KERNEL24	= linux-2.4.31
KERNEL24_TB2	= ${KERNEL24}.tar.bz2
KERNEL24_URL	= ${KERNEL_FTP}/v2.4/${KERNEL24_TB2}
#ARM24PATCH	= patch-2.4.27-vrs1.bz2
#ARM24PATCH_URL	= ${ARM_FTP}/v2.4/${ARM24PATCH}
ARM24PATCH	= patch-2.4.31-j720-4.bz2
ARM24PATCH_URL	= ${JORNADA7XX}/${ARM24PATCH}
KERNEL26	= linux-2.6.12
KERNEL26_TB2	= ${KERNEL26}.tar.bz2
KERNEL26_URL	= ${KERNEL_FTP}/v2.6/${KERNEL26_TB2}
#ARM26PATCH	= No need for a patch as ARM is well-integrated in kernel 2.6
#ARM26PATCH_URL	= ${ARM_FTP}/v2.6/${ARM26PATCH}
ARM26PATCH	= patch-2.6.12.3.bz2
ARM26PATCH_URL	= ${KERNEL_FTP}/v2.6/${ARM26PATCH}

.PHONY: ramdisk force

# ------------- programs ---------------------------------------------------

LN			= ln
MKDIR			= mkdir
TEE			= tee
TOUCH			= touch
CP			= cp -f
RM			= rm -f

# ------------- directories for source and build ----------------------------

KERNELDIR		= linux/kernel
KBUILDDIR		= linux/build/kernel
KUPSTREAMDIR		= linux/upstream/kernel
KERNELDIR26		= linux/kernel26
KBUILDDIR26		= linux/build/kernel26
KUPSTREAMDIR26		= linux/upstream/kernel26
MODDIR			= ${TOPDIR}/${KBUILDDIR}/target
MODDIR26		= ${TOPDIR}/${KBUILDDIR26}/target

RDIMAGE			= ${TOPDIR}/ramdisk/ramdiskimage.gz
RDIMAGE26		= ${TOPDIR}/ramdisk/ramdiskimage26.gz


# -------------- default targets -------------------------------------------
help::
	@$${PAGER:-cat} HELP

all:: j820

localclean::
	-${RM} vmlinux j820

clean:: localclean
	${MAKE} -C ${KBUILDDIR} ARCH=arm clean
	${MAKE} -C ramdisk clean

mrproper:: localclean
	${MAKE} -C ${KBUILDDIR} ARCH=arm mrproper
	${MAKE} -C ramdisk ARCH=arm mrproper

distclean::
	rm -rf linux/build linux/upstream/*/
	${MAKE} -C ramdisk distclean

# --------------------- kernel build tree ----------------------------------

kbuildtree:: upstream-kernel
	mkdir -p linux/build/
	cp -fal ${KUPSTREAMDIR} linux/build/
	cp -fal ${KERNELDIR} linux/build/
	perl -pi -e 's/^EXTRAVERSION.*=.*$$/EXTRAVERSION = ${EXTRAVERSION}/ ; \
		s/^ARCH\s*:= .*$$/ARCH := arm/ ; \
		s/^\#?CROSS_COMPILE\s*=.*$$/CROSS_COMPILE = ${TARGET}-/ ; \
		' linux/build/kernel/Makefile

ourtree:
	cp -fal ${KERNELDIR} linux/build/

${KUPSTREAMDIR}-hh/CVS/Root ${KUPSTREAMDIR}-hh/CVS/Repository:
	mkdir -p ${KUPSTREAMDIR}-hh/CVS ; cd ${KUPSTREAMDIR}-hh/CVS || exit 2 ; \
	echo :pserver:anoncvs@cvs.handhelds.org:/cvs > Root ; \
	echo linux/kernel > Repository ; \
	touch Entries ; \
	grep -q ':pserver:anoncvs@cvs.handhelds.org:2401/cvs' ~/.cvspass || \
	echo '/1 :pserver:anoncvs@cvs.handhelds.org:2401/cvs Ay=0=h<Z' >> ~/.cvspass

${KUPSTREAMDIR}-hh/Makefile: ${KUPSTREAMDIR}-hh/CVS/Root
	cd ${KUPSTREAMDIR}-hh && cvs -z9 update -dfP && \
	touch Makefile

linux24: linux/upstream/${KERNEL24_TB2}
linux/upstream/${KERNEL24_TB2}:
	mkdir -p linux/upstream && cd linux/upstream && \
	wget -c ${KERNEL24_URL}

arm24patch: linux/upstream/${ARM24PATCH}
linux/upstream/${ARM24PATCH}:
	mkdir -p linux/upstream && cd linux/upstream && \
	wget -c ${ARM24PATCH_URL}

upstream-kernel: linux24 arm24patch
	[ -f ${KUPSTREAMDIR}/Makefile ] || ( \
	cd linux/upstream && \
	tar jxf ${KERNEL24_TB2} && \
	mv ${KERNEL24} kernel && \
	cd kernel && \
	bzcat ../${ARM24PATCH} | patch -p1 -E )

# --------------------- kernel build tree, v2.6 ----------------------------

kbuildtree26:: upstream-kernel26
	mkdir -p linux/build/
	cp -fal ${KUPSTREAMDIR26} linux/build/
	cp -fal ${KERNELDIR26} linux/build/
	perl -pi.bak -e 's/^EXTRAVERSION.*=.*$$/EXTRAVERSION = ${EXTRAVERSION}/ ; \
		s/^ARCH\s*:= .*$$/ARCH := arm/ ; \
		s/^\#?CROSS_COMPILE\s*=.*$$/CROSS_COMPILE = ${TARGET}-/ ; \
		' linux/build/kernel/Makefile

ourtree26:
	cp -fal ${KERNELDIR26} linux/build/

old-${KUPSTREAMDIR26}/CVS/Root ${KUPSTREAMDIR26}/CVS/Repository:
	mkdir -p ${KUPSTREAMDIR26}/CVS ; cd ${KUPSTREAMDIR26}/CVS || exit 2 ; \
	echo :pserver:anoncvs@cvs.handhelds.org:/cvs > Root ; \
	echo linux/kernel26 > Repository ; \
	touch Entries ; \
	grep -q ':pserver:anoncvs@cvs.handhelds.org:2401/cvs' ~/.cvspass || \
	echo '/1 :pserver:anoncvs@cvs.handhelds.org:2401/cvs Ay=0=h<Z' >> ~/.cvspass

old-${KUPSTREAMDIR26}/Makefile: ${KUPSTREAMDIR26}/CVS/Root
	cd ${KUPSTREAMDIR26} && cvs -z9 update -dfP && \
	touch Makefile

linux26: linux/upstream/${KERNEL26_TB2}
linux/upstream/${KERNEL26_TB2}:
	mkdir -p linux/upstream && cd linux/upstream && \
	wget -c ${KERNEL26_URL}

arm26patch: linux/upstream/${ARM26PATCH}
linux/upstream/${ARM26PATCH}:
	if [ -n "${ARM26PATCH}" ] ; then \
	mkdir -p linux/upstream && cd linux/upstream && \
	wget -c ${ARM26PATCH_URL} ; fi

upstream-kernel26: linux26 arm26patch
	[ -f ${KUPSTREAMDIR26}/Makefile ] || ( \
	cd linux/upstream && \
	tar jxf ${KERNEL26_TB2} && \
	mv ${KERNEL26} kernel26 && \
	if [ -n "${ARM26PATCH}" ] ; then \
	cd kernel26 && \
	bzcat ../${ARM26PATCH} | patch -p1 -E ; fi )

# ------------------ optional: build the toolchain -------------------------

gcc_version:
	${TARGET}-gcc -v

uclibc/buildroot/Makefile:
	cd uclibc && \
	svn co svn://uclibc.org/trunk/buildroot

uclibc-svn: ${BUILDROOT}/Makefile

uclibc-setup: uclibc-svn
	cp uclibc/buildroot.config ${BUILDROOT}/.config ;
	[ -f ${BUILDROOT}/toolchain/uClibc/uClibc.config.bak ] || \
	cp ${BUILDROOT}/toolchain/uClibc/uClibc.config ${BUILDROOT}/toolchain/uClibc/uClibc.config.bak
	cp uclibc/uclibc.config ${BUILDROOT}/toolchain/uClibc/uClibc.config
	#make -C ${BUILDROOT} ${BUILDROOT}/toolchain_build_arm/uClibc/.unpacked
	#-ln -sf ../../../../linux/linux.mk \
	#       ../../../../linux/Config.in \
	#       uclibc/buildroot/package/linux/ ; \
	#perl -p -i.bak -e 'if (m,package/linux/Config.in,) { $$A = 1 } ; unless ($$A) { s,endmenu,source "package/linux/Config.in"\nendmenu,s }' uclibc/buildroot/package/Config.in

uclibc-config: uclibc-setup
	make -C uclibc/buildroot oldconfig

toolchain: uclibc-config
	make -C uclibc/buildroot uclibc

everything: toolchain
	make -C uclibc/buildroot

# ------------------ build the boot image ----------------------------------

j820: config depend ramdisk do-j820
do-j820:
	export RDIMAGE=${RDIMAGE} && \
	cd ${KBUILDDIR} && \
	${MAKE} \
	    ARCH=arm \
	    CROSS_COMPILE=${TARGET}- \
	    INITRD=${RDIMAGE} \
	    zImage j820
	cp ${KBUILDDIR}/arch/arm/boot/j820/j820 j820

kernel-compilation:
	mkdir -p ${MODDIR} && \
	export RDIMAGE=${TOPDIR}/ramdisk/ramdiskimage.gz && \
	cd ${KBUILDDIR} && \
	${MAKE} \
	    ARCH=arm \
	    CROSS_COMPILE=${TARGET}- \
	    INSTALL_MOD_PATH=${MODDIR} \
	    DEPMOD=true \
	    zImage modules modules_install

all-kernel-compilation: kbuildtree config depend kernel-compilation

just-j820:
	export RDIMAGE=${TOPDIR}/ramdisk/ramdiskimage.gz && \
	cd ${KBUILDDIR} && \
	${MAKE} \
	    ARCH=arm \
	    CROSS_COMPILE=${TARGET}- \
	    INITRD=${RDIMAGE} \
	    SUBDIRS=arch/arm/boot \
	    j820
	cp ${KBUILDDIR}/arch/arm/boot/j820/j820 j820


sandisk: j820
	mount /mnt/sandisk/; cp j820 /mnt/sandisk/; umount /mnt/sandisk/
sandisk-fare:
	( mount /dev/sda1 /mnt && ( ls -l j820-26; cp -f j820-26 /mnt ; umount /mnt ) ; echo -e '\007' )

j820-26: config26 modules26 ramdisk26 do-j820-26
do-j820-26:
	mkdir -p ${MODDIR26} && \
	echo ${MODDIR26} && \
	cd ${KBUILDDIR26} && \
	${MAKE} \
	    ARCH=arm \
	    CROSS_COMPILE=${TARGET}- \
	    INITRD=${RDIMAGE26} \
	    INSTALL_MOD_PATH=${MODDIR26} \
	    zImage j820
	cp ${KBUILDDIR26}/arch/arm/boot/j820/j820 j820-26


# ---------------- Initial configuration and dependences -------------------

config: kbuildtree do-config
do-config: ${KBUILDDIR}/.config
${KBUILDDIR}/.config: force
	cd ${KBUILDDIR} && \
	cp arch/arm/def-configs/jornada820 .config && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- \
		oldconfig && \
	( cmp -s .config arch/arm/def-configs/jornada820 || \
	cat .config > arch/arm/def-configs/jornada820 )

menuconfig: kbuildtree do-menuconfig
do-menuconfig: force
	cd ${KBUILDDIR} && \
	cp arch/arm/def-configs/jornada820 .config && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- \
		menuconfig && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- \
		oldconfig && \
	( cmp -s .config arch/arm/def-configs/jornada820 || \
	cat .config > arch/arm/def-configs/jornada820 )

depend: config do-depend
do-depend: ${KBUILDDIR}/.depend
${KBUILDDIR}/.depend: force
	${MAKE} -C ${KBUILDDIR} ARCH=arm CROSS_COMPILE=${TARGET}- depend

menuconfig26: kbuildtree26 do-menuconfig26
do-menuconfig26: force
	cd ${KBUILDDIR26} && \
	cp arch/arm/configs/jornada820_defconfig .config && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- \
		menuconfig && \
	( cmp -s .config arch/arm/configs/jornada820_defconfig || \
	cat .config > arch/arm/configs/jornada820_defconfig )

config26: kbuildtree26 do-config26
do-config26: ${KBUILDDIR26}/.config
${KBUILDDIR26}/.config: force
	cd ${KBUILDDIR26} && \
	cp arch/arm/configs/jornada820_defconfig .config && \
	${MAKE} ARCH=arm CROSS_COMPILE=${TARGET}- \
		oldconfig && \
	( cmp -s .config arch/arm/configs/jornada820_defconfig || \
	cat .config > arch/arm/configs/jornada820_defconfig )

# ------------------- build loadable modules -------------------------------

modules: config depend do-modules

do-modules: do-modules-compilation do-modules-tarbz2

do-modules-tarbz2:
	cd ${KBUILDDIR}/target && \
	tar jcf modules.tar.bz2 lib/modules/ && \
	ln -f modules.tar.bz2 ${TOPDIR}/ramdisk/modules.tar.bz2

do-modules-compilation:
	mkdir -p ${MODDIR} && \
	${MAKE} -C ${KBUILDDIR} \
		ARCH=arm \
		CROSS_COMPILE=${TARGET}- \
		INSTALL_MOD_PATH=${MODDIR} \
		DEPMOD=true \
		modules modules_install

modules26: config26 do-modules26

do-modules26: ramdisk/modules26.tar.bz2

ramdisk/modules26.tar.bz2: ${KBUILDDIR26}/target/lib/modules
	cd ${KBUILDDIR26}/target && \
	tar jcf modules.tar.bz2 lib/modules && \
	ln -f modules.tar.bz2 ${TOPDIR}/ramdisk/modules26.tar.bz2

${KBUILDDIR26}/target/lib/modules: force
	mkdir -p ${MODDIR26} && \
	${MAKE} -C ${KBUILDDIR26} \
		ARCH=arm \
		CROSS_COMPILE=${TARGET}- \
		INSTALL_MOD_PATH=${MODDIR26} \
		DEPMOD=true \
		modules modules_install

# ------------------- build ramdisk image ----------------------------------

ramdisk: modules
	${MAKE} -C ramdisk TARGET=${TARGET} OS=${OS} ramdisk

ramdisk26: modules26
	${MAKE} -C ramdisk TARGET=${TARGET} OS=${OS} ramdisk26
