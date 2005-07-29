CROSS_COMPILE	= arm-linux-uclibc-
TOPDIR		= $(shell pwd)
BUILDROOT	= ${TOPDIR}/buildroot
TOOLCHAINPATH	= ${BUILDROOT}/build_arm/staging_dir/bin
PATH:=${TOOLCHAINPATH}:${PATH}
export PATH

EXTRAVERSION	= -j820

# We might have to pick a uclibc revision that works for us.
# Last revision I was successful with is 10929, aka "{2005-07-27}".
# Beware that said revision doesn't build for me with gcc 4.0.1 or gcc-3.4
# on a x86_64, where I HAD TO SYMLINK THE gcc IN /usr/bin TO gcc-3.3.
# But revision 10941 did work with gcc 4.0.1 on a i386.
# Note: kernel 2.4 contains gcc-isms and requires gcc 3.3, which itself
# seems to not be compilable by a more recent gcc in a 64-bit safe way.
# Try HEAD if you're adventurous, 10929 or 10941 if not.
# If you try anything more recent, please report what works for you.
UCLIBC_SVN_REVISION = "HEAD"

KERNEL_FTP	= ftp://ftp.bz2.de.kernel.org/pub/linux/kernel
ARM_FTP		= ftp://ftp.arm.linux.org.uk/pub/armlinux/source/kernel-patches
JORNADA7XX	= http://wwwcip.informatik.uni-erlangen.de/~simigern/jornada-7xx/linux-2.4.31-j720-4/

KVER		= 2.4.31
KERNEL24	= linux-${KVER}
KERNELVERSION	= ${KVER}${EXTRAVERSION}
KERNEL24_TB2	= ${KERNEL24}.tar.bz2
KERNEL24_URL	= ${KERNEL_FTP}/v2.4/${KERNEL24_TB2}
#ARM24PATCH	= patch-2.4.27-vrs1.bz2
#ARM24PATCH_URL	= ${ARM_FTP}/v2.4/${ARM24PATCH}
ARM24PATCH	= patch-2.4.31-j720-4.bz2
ARM24PATCH_URL	= ${JORNADA7XX}/${ARM24PATCH}
KVER26		= 2.6.12
KERNEL26	= linux-${KVER26}
KERNELVERSION26	= ${KVER26}${EXTRAVERSION}
KERNEL26_TB2	= ${KERNEL26}.tar.bz2
KERNEL26_URL	= ${KERNEL_FTP}/v2.6/${KERNEL26_TB2}
#ARM26PATCH	= No need for a patch as ARM is well-integrated in kernel 2.6
#ARM26PATCH_URL	= ${ARM_FTP}/v2.6/${ARM26PATCH}
ARM26PATCH	= patch-2.6.12.3.bz2
ARM26PATCH_URL	= ${KERNEL_FTP}/v2.6/${ARM26PATCH}

# ------------- directories for source and build ----------------------------

DLDIR			= ${TOPDIR}/dl
BUILDDIR		= ${TOPDIR}/work
RAMDISKDIR		= ${TOPDIR}/ramdisk
CONFDIR			= ${TOPDIR}/conf
KERNELDIR		= ${TOPDIR}/linux/kernel
KERNELDIR26		= ${TOPDIR}/linux/kernel26
UPSTREAMDIR		= ${TOPDIR}/upstream
KBUILDDIR		= ${BUILDDIR}/kernel
TARGETDIR		= ${BUILDDIR}/target
KUPSTREAMDIR		= ${UPSTREAMDIR}/${KERNEL24}
RAMDISKIMAGE		= ${TARGETDIR}/ramdiskimage
KBUILDDIR26		= ${BUILDDIR}/kernel26
TARGETDIR26		= ${BUILDDIR}/target26
KUPSTREAMDIR26		= ${UPSTREAMDIR}/${KERNEL26}
RAMDISKIMAGE26		= ${TARGETDIR26}/ramdiskimage
KUPSTREAMDIRHH		= ${UPSTREAMDIR}/kernel-hh
KUPSTREAMDIR26HH	= ${UPSTREAMDIR}/kernel26-hh

# -------------- xxx ----------------------------------

KMAKEFILE_TRANS	:= \
		s/^EXTRAVERSION.*[?:]?=.*$$/EXTRAVERSION = ${EXTRAVERSION}/ ; \
		s/^ARCH\s*[?:]?= .*$$/ARCH := arm/ ; \
		s/^\#?CROSS_COMPILE\s*[?:]?=.*$$/CROSS_COMPILE = ${CROSS_COMPILE}/ ;

K24PATCHES		:= ${DLDIR}/${ARM24PATCH}
K24EXTENSIONS		:= 
K24MAKEFILE_TRANS	:= ${KMAKEFILE_TRANS}

K26PATCHES		:= ${DLDIR}/${ARM26PATCH}
K26EXTENSIONS		:= 
K26MAKEFILE_TRANS	:= ${KMAKEFILE_TRANS}

BUSYBOXVER		= 1.00
BUSYBOXTAR		= busybox-${BUSYBOXVER}.tar.bz2
BUSYBOXURL		= http://busybox.net/downloads/${BUSYBOXTAR}
BUSYBOXDIR		= ${BUILDDIR}/busybox-${BUSYBOXVER}

PCMCIAVER		= 3.2.8
PCMCIATAR		= pcmcia-cs-${PCMCIAVER}.tar.gz
PCMCIAURL		= http://pcmcia-cs.sourceforge.net/ftp/${PCMCIATAR}
PCMCIADIR		= ${BUILDDIR}/pcmcia-cs-${PCMCIAVER}

#SCHEMIX_URL	= http://ftp.gnu.org/savannah/files/schemix/schemix.pkg/0.2.1/schemix-0.2.1.tar.gz
#KALLSYMS_ORIG	= http://www.atomicrocketturtle.com/kernels/2.4/linux-2.4.9-kallsyms.patch
KALLSYMS24PATCH	= linux-2.4.31-kallsyms.patch.bz2
KALLSYMS_URL	= http://jornada820.sourceforge.net/files/upstream/${KALLSYMS24PATCH}

CROSS_GCC	= ${CROSS_COMPILE}gcc


.PHONY: ramdisk force busybox pcmcia


# ------------- programs ---------------------------------------------------

LN			= ln
MKDIR			= mkdir -p
TEE			= tee
TOUCH			= touch
CP			= cp -f
RM			= rm -f
MKE2FS		 	= mke2fs
DEBUGFS		 	= debugfs
E2FSCK		 	= e2fsck
CANONICAL_PATH          = /bin:/usr/bin


$(shell ${MKDIR} ${BUILDDIR} ${DLDIR} ${UPSTREAMDIR})

# -------------- default targets -------------------------------------------

help::
	@$${PAGER:-cat} HELP

all:: j820


# --------------------- schemix (optional) ---------------------------------
ifneq (${SCHEME},)
K24PATCHES		:= ${K24PATCHES} ${DLDIR}/${KALLSYMS24PATCH} 
K24EXTENSIONS		:= ${K24EXTENSIONS} schemix-src
K24MAKEFILE_TRANS	:= ${K24MAKEFILE_TRANS} \
				s/^SUBDIRS\s*=(.*)$$/SUBDIRS =$$1 schemix/ ;
#K26PATCHES		:= ${K26PATCHES} ${DLDIR}/${KALLSYMS24PATCH} 
K26EXTENSIONS		:= ${K26EXTENSIONS} schemix-src26
#K26MAKEFILE_TRANS	:= ${K24MAKEFILE_TRANS}

schemix-dir:
	[ -f ${UPSTREAMDIR}/schemix/Makefile ] || make schemix-cvs
schemix-cvs:
	cd ${UPSTREAMDIR} && \
	cvs -z3 -d:ext:anoncvs@savannah.nongnu.org:/cvsroot/schemix \
		co schemix
schemix-src: schemix-dir
	cp -fal ${UPSTREAMDIR}/schemix ${KUPSTREAMDIR}/
	cd ${KUPSTREAMDIR}/schemix && \
	${RM} schemix-init.c schemix-pre-init.c && \
	echo '(load "schemix-make-init.scm")' | ${SCHEME} && \
	( echo "obj-m := schemix.o" ; echo "include \$$(TOPDIR)/Rules.make" \
	) > Makefile
schemix-src26: schemix-dir
	cp -fal ${UPSTREAMDIR}/schemix ${KUPSTREAMDIR26}/
	cd ${KUPSTREAMDIR26}/schemix && \
	${RM} schemix-init.c schemix-pre-init.c && \
	echo '(load "schemix-make-init.scm")' | ${SCHEME} && \
	( echo "obj-m := schemix.o" ) > Makefile
schemix-compile: schemix-src kbuildtree
	${MAKE} -C ${KBUILDDIR} \
		SUBDIRS=schemix modules
schemix-compile26: schemix-src26 kbuildtree26
	${MAKE} -C ${KBUILDDIR} \
		SUBDIRS=schemix modules

endif

# --------------------- kernel build tree ----------------------------------

kbuildtree:: upstream-kernel
	${MKDIR} ${KBUILDDIR}
	cp -fal ${KUPSTREAMDIR}/. ${KBUILDDIR}/.
	cp -fal ${KERNELDIR}/. ${KBUILDDIR}/.
	perl -pi -e '${K24MAKEFILE_TRANS}' ${KBUILDDIR}/Makefile

ourtree:
	cp -fal ${KERNELDIR}/. ${KBUILDDIR}/.

${KUPSTREAMDIRHH}/CVS/Root ${KUPSTREAMDIRHH}/CVS/Repository:
	${MKDIR} ${KUPSTREAMDIRHH}/CVS ; cd ${KUPSTREAMDIRHH}/CVS || exit 2 ; \
	echo :pserver:anoncvs@cvs.handhelds.org:/cvs > Root ; \
	echo linux/kernel > Repository ; \
	touch Entries ; \
	grep -q ':pserver:anoncvs@cvs.handhelds.org:2401/cvs' ~/.cvspass || \
	echo '/1 :pserver:anoncvs@cvs.handhelds.org:2401/cvs Ay=0=h<Z' >> ~/.cvspass
${KUPSTREAMDIRHH}/Makefile: ${KUPSTREAMDIRHH}/CVS/Root
	cd ${KUPSTREAMDIRHH} && cvs -z9 update -dfP && touch Makefile

upstream-kernel:
	[ -f ${KUPSTREAMDIR}/Makefile ] || make do-upstream-kernel
do-upstream-kernel: linux24-dir ${K24EXTENSIONS} ${K24PATCHES}
	bzcat ${K24PATCHES} | ( cd ${KUPSTREAMDIR} && patch -p1 -E )
linux24-dir: ${DLDIR}/${KERNEL24_TB2}
	rm -rf ${UPSTREAMDIR}/${KERNEL24}
	tar jxfC ${DLDIR}/${KERNEL24_TB2} ${UPSTREAMDIR}
${DLDIR}/${KERNEL24_TB2}:
	cd ${DLDIR} && wget -c ${KERNEL24_URL}
${DLDIR}/${ARM24PATCH}:
	cd ${DLDIR} && wget -c ${ARM24PATCH_URL}
get-kallsyms: ${DLDIR}/${KALLSYMS24PATCH}
${DLDIR}/${KALLSYMS24PATCH}:
	cd ${DLDIR} && wget -c ${KALLSYMS_URL}


# --------------------- kernel build tree, v2.6 ----------------------------

kbuildtree26:: upstream-kernel26
	${MKDIR} ${KBUILDDIR26}
	cp -fal ${KUPSTREAMDIR26}/. ${KBUILDDIR26}/.
	cp -fal ${KERNELDIR26}/. ${KBUILDDIR26}/.
	perl -pi -e '${K26MAKEFILE_TRANS}' ${KBUILDDIR26}/Makefile

ourtree26:
	cp -fal ${KERNELDIR26}/. ${KBUILDDIR26}/.

${KUPSTREAMDIR26HH}/CVS/Root ${KUPSTREAMDIR26HH}/CVS/Repository:
	${MKDIR} ${KUPSTREAMDIR26HH}/CVS ; cd ${KUPSTREAMDIR26HH}/CVS || exit 2 ; \
	echo :pserver:anoncvs@cvs.handhelds.org:/cvs > Root ; \
	echo linux/kernel26 > Repository ; \
	touch Entries ; \
	grep -q ':pserver:anoncvs@cvs.handhelds.org:2401/cvs' ~/.cvspass || \
	echo '/1 :pserver:anoncvs@cvs.handhelds.org:2401/cvs Ay=0=h<Z' >> ~/.cvspass

${KUPSTREAMDIR26HH}/Makefile: ${KUPSTREAMDIR26HH}/CVS/Root
	cd ${KUPSTREAMDIR26HH} && cvs -z9 update -dfP && touch Makefile

linux26: ${DLDIR}/${KERNEL26_TB2}
${DLDIR}/${KERNEL26_TB2}:
	cd ${DLDIR} && wget -c ${KERNEL26_URL}
linux26-dir: linux26
	rm -rf ${UPSTREAMDIR}/${KERNEL26}
	tar jxfC ${DLDIR}/${KERNEL26_TB2} ${UPSTREAMDIR}


arm26patch: ${DLDIR}/${ARM26PATCH}
${DLDIR}/${ARM26PATCH}:
	cd ${DLDIR} && wget -c ${ARM26PATCH_URL}

upstream-kernel26:
	[ -f ${KUPSTREAMDIR26}/Makefile ] || make do-upstream-kernel26
do-upstream-kernel26: linux26-dir ${K26EXTENSIONS} ${K26PATCHES}
	[ -f ${KUPSTREAMDIR26}/Makefile ] || ( \
	cd ${UPSTREAMDIR} && \
	tar jxf ${DLDIR}/${KERNEL26_TB2} && \
	if [ -n "${ARM26PATCH}" ] ; then \
	cd kernel26 && \
	bzcat ../${ARM26PATCH} | patch -p1 -E ; fi )

# ------------------ optional: build the toolchain -------------------------

# Is there a target we can give to uclibc's buildroot
# to only compile the toolchain and none of the rest of the runtime,
# in case we want a toolchain now and a runtime later?
# "toolchain" or "uclibc" doesn't work, as of svn revision 10929
toolchain: uclibc-config
	make -C ${BUILDROOT}

everything: toolchain
	make -C ${BUILDROOT}

gcc_version:
	${CROSS_GCC} -v

uclibc-svn:
	svn co -r ${UCLIBC_SVN_REVISION} svn://uclibc.org/trunk/buildroot

uclibc-setup: uclibc-svn
	### This configures buildroot for our purposes.
	cp ${CONFDIR}/buildroot.config ${BUILDROOT}/.config
	### This is because uclibc revision 10929 otherwise borks.
	### Hopefully we can get rid of it later.
	[ -f ${BUILDROOT}/toolchain/uClibc/uClibc.config.bak ] || \
	cp ${BUILDROOT}/toolchain/uClibc/uClibc.config \
	   ${BUILDROOT}/toolchain/uClibc/uClibc.config.bak
	cp ${CONFDIR}/uclibc.config ${BUILDROOT}/toolchain/uClibc/uClibc.config
	### This is in case someday we'd like to get the kernel compiled
	### through uclibc's buildroot
	#make -C ${BUILDROOT} ${BUILDROOT}/toolchain_build_arm/uClibc/.unpacked
	#-ln -sf ../../../../linux/linux.mk \
	#       ../../../../linux/Config.in \
	#       ${BUILDROOT}/package/linux/ ; \
	#perl -p -i.bak -e 'if (m,package/linux/Config.in,) { $$A = 1 } ; unless ($$A) { s,endmenu,source "package/linux/Config.in"\nendmenu,s }' ${BUILDROOT}/package/Config.in

uclibc-menuconfig: uclibc-setup
	make -C ${BUILDROOT} menuconfig
	( cmp -s ${BUILDROOT}/.config ${CONFDIR}/buildroot.config || \
	cat ${BUILDROOT}/.config > ${CONFDIR}/buildroot.config )

uclibc-config: uclibc-setup
	make -C ${BUILDROOT} oldconfig

# remove all the built stuff so we can try again a new compilation
uclibc-clean:
	${RM} -rf ${BUILDROOT}/*build_arm

# remove all uclibc svn sources
uclibc-drop-sources:
	cd ${BUILDROOT}/ && \
	${RM} -rf Makefile Config.in \
		docs target toolchain package

# remove everything except the expensive downloads in dl/
uclibc-mrproper: uclibc-clean uclibc-drop-sources

# remove everything except the useful binaries
toolchain-keep-only-binaries: uclibc-drop-sources
	cd ${BUILDROOT}/ && \
	${RM} -rf dl
# Note that the uclibc toolchain cares about the path it was created
# to reside in, so if you care where that is, or want to make it so it
# can be redistributed, build it with BUILDROOT=/usr/local/uclibc or so.
# Also, it appears that when the toolchain is built, mostly the only
# directory that matters is build_arm, but some stuff in toolchain_build_arm
# is necessary too. I didn't dig to find out what to delete and what to keep.
# Maybe only the ccache cache is required?
# This will matter to you if you don't have 600 MB that sits around just for
# that, like I do. If you find a way to sort the chaff from the grain, tell me.
# Surely there's something in the uclibc documentation that will tell you.
# There might even be a hidden script or make target to delete everything
# but what is necessary for the toolchain to work.
# Oh: symlinks are your friends.

# remove everything about uclibc
uclibc-distclean:
	${RM} -rf ${BUILDROOT}

# ------------------ build the boot image ----------------------------------

j820: all-configuration all-compilation
all-configuration: config depend
all-compilation: ramdisk do-j820

do-j820:
	${MAKE} -C ${KBUILDDIR} \
	    INITRD=${RAMDISKIMAGE} \
	    zImage j820
	cp ${KBUILDDIR}/arch/arm/boot/j820/j820 j820

kernel-compilation:
	${MAKE} -C {KBUILDDIR} \
	    zImage

modules-compilation:
	${MKDIR} ${TARGETDIR}
	${RM} -rf ${TARGETDIR}/lib/modules/${KERNELVERSION}
	${MAKE} -C ${KBUILDDIR} \
		INSTALL_MOD_PATH=${TARGETDIR} \
		DEPMOD=true \
		modules modules_install

all-kernel-compilation: \
	kbuildtree config depend \
	kernel-compilation modules-compilation

just-j820:
	${MAKE} -C ${KBUILDDIR} \
	    INITRD=${RAMDISKIMAGE} \
	    SUBDIRS=arch/arm/boot \
	    j820
	cp ${KBUILDDIR}/arch/arm/boot/j820/j820 j820

j820-26: config26 modules26 ramdisk26 do-j820-26
do-j820-26:
	${MAKE} -C ${KBUILDDIR26} \
	    INITRD=${RAMDISKIMAGE26} \
	    zImage j820
	cp ${KBUILDDIR26}/arch/arm/boot/j820/j820 j820-26

modules26-compilation:
	${MKDIR} ${TARGETDIR26}
	${RM} -rf ${TARGETDIR}/lib/modules/${KERNELVERSION}
	${MAKE} -C ${KBUILDDIR26} \
		INSTALL_MOD_PATH=${TARGETDIR26} \
		DEPMOD=true \
		modules modules_install

# ---------------- Initial configuration and dependences -------------------

config: kbuildtree do-config
do-config: ${KBUILDDIR}/.config
${KBUILDDIR}/.config: force
	cd ${KBUILDDIR} && \
	cp arch/arm/def-configs/jornada820 .config && \
	${MAKE} oldconfig && \
	( cmp -s .config arch/arm/def-configs/jornada820 || \
	cat .config > arch/arm/def-configs/jornada820 )

menuconfig: kbuildtree do-menuconfig
do-menuconfig: force
	cd ${KBUILDDIR} && \
	cp arch/arm/def-configs/jornada820 .config && \
	${MAKE} menuconfig && \
	${MAKE} oldconfig && \
	( cmp -s .config arch/arm/def-configs/jornada820 || \
	cat .config > arch/arm/def-configs/jornada820 )

depend: config do-depend
do-depend: ${KBUILDDIR}/.depend
${KBUILDDIR}/.depend: force
	${MAKE} -C ${KBUILDDIR} depend

menuconfig26: kbuildtree26 do-menuconfig26
do-menuconfig26: force
	cd ${KBUILDDIR26} && \
	cp arch/arm/configs/jornada820_defconfig .config && \
	${MAKE} menuconfig && \
	( cmp -s .config arch/arm/configs/jornada820_defconfig || \
	cat .config > arch/arm/configs/jornada820_defconfig )

config26: kbuildtree26 do-config26
do-config26: ${KBUILDDIR26}/.config
${KBUILDDIR26}/.config: force
	cd ${KBUILDDIR26} && \
	cp arch/arm/configs/jornada820_defconfig .config && \
	${MAKE} oldconfig && \
	( cmp -s .config arch/arm/configs/jornada820_defconfig || \
	cat .config > arch/arm/configs/jornada820_defconfig )

# ------------------- package loadable modules -----------------------------

modules: config depend do-modules

do-modules: modules-compilation modules-tarbz2

modules-tarbz2:
	cd ${TARGETDIR} && \
	tar -jcf modules.tar.bz2 --owner 0 --group 0 \
		lib/modules/${KERNELVERSION}

modules26: config26 do-modules26

do-modules26: ${TARGET26}/modules.tar.bz2

modules26-tarbz2:
	cd ${TARGETDIR26} && \
	tar -jcf modules.tar.bz2 --owner 0 --group 0 \
		lib/modules/${KERNELVERSION26}

# ------------------- build ramdisk image ----------------------------------

ramdisk: modules do-ramdisk
do-ramdisk: ${RAMDISKIMAGE}.gz
${RAMDISKIMAGE}.gz: ${RAMDISKIMAGE}
	gzip -9 < $< > $@
${RAMDISKIMAGE}: ${CONFDIR}/mkcmdfile.sh busybox pcmcia
	cd ramdisk && \
	dd if=/dev/zero of=${RAMDISKIMAGE} count=3072 bs=1024 && \
	$(MKE2FS) -F ${RAMDISKIMAGE} && \
	sh $< ${RAMDISKIMAGE} ${RAMDISKDIR} ${BUILDDIR} ${TARGETDIR} | \
	$(DEBUGFS) && \
	$(E2FSCK) -f -y ${RAMDISKIMAGE} ; :

ramdisk26: modules26 do-ramdisk26
do-ramdisk26: ${RAMDISKIMAGE26}.gz
${RAMDISKIMAGE26}: ${CONFDIR}/mkcmdfile.sh busybox pcmcia
	cd ramdisk && \
	dd if=/dev/zero of=${RAMDISKIMAGE26} count=4096 bs=1024 && \
	$(MKE2FS) -F ${RAMDISKIMAGE26} && \
	sh $< ${RAMDISKIMAGE26} ${RAMDISKDIR} ${BUILDDIR} ${TARGETDIR26} | \
	$(DEBUGFS) && \
	$(E2FSCK) -f -y ${RAMDISKIMAGE26} ; :
${RAMDISKIMAGE26}.gz: ${RAMDISKIMAGE26}
	gzip -9 < $< > $@

# -----------------------  minimal userland binaries  ----------------------

busybox:	busybox-dir busybox-compile

${DLDIR}/${BUSYBOXTAR}:
	cd ${DLDIR} && wget -c ${BUSYBOXURL}

busybox-dir:
	[ -d ${BUSYBOXDIR} ] || make do-busybox-dir

do-busybox-dir: ${DLDIR}/${BUSYBOXTAR}
	rm -rf ${BUSYBOXDIR}
	tar jxfC ${DLDIR}/${BUSYBOXTAR} ${BUILDDIR}

# if using svn, then the BUSYBOXDIR parameter above
# should be stripped from its a -version.number
busybox-svn:
	cd ${BUILDDIR} && \
	svn co svn://busybox.net/trunk/busybox

busybox-compile: ${BUILDDIR}/busybox.bin
${BUILDDIR}/busybox.bin:
	cp ${CONFDIR}/busybox.config ${BUSYBOXDIR}/.config
	${MAKE}	-C ${BUSYBOXDIR} CROSS=${CROSS_COMPILE} oldconfig
	${MAKE} -C ${BUSYBOXDIR} CROSS=${CROSS_COMPILE} busybox
	${MAKE} -C ${BUSYBOXDIR} CROSS=${CROSS_COMPILE} install
	cp ${BUSYBOXDIR}/_install/bin/busybox $@

pcmcia:	pcmcia-install
pcmcia-install: pcmcia-dir pcmcia-compile
pcmcia-dir:
	[ -d ${PCMCIADIR} ] || make do-pcmcia-dir
pcmcia-tar: ${DLDIR}/${PCMCIATAR}
${DLDIR}/${PCMCIATAR}:
	cd ${DLDIR} && wget -c ${PCMCIAURL}
do-pcmcia-dir: ${DLDIR}/${PCMCIATAR}
	rm -rf ${PCMCIADIR}
	tar zxfC ${DLDIR}/${PCMCIATAR} ${BUILDDIR}

PCMCIA_CFLAGS	= -Os -static -s \
	-I../include/static -I../include -I../modules \
	-I../../../../linux/kernel/include

pcmcia-compile: ${BUILDDIR}/cardctl ${BUILDDIR}/cardmgr
${BUILDDIR}/cardctl ${BUILDDIR}/cardmgr:
	@-cd ${PCMCIADIR}/include/linux && \
	${LN} -s ../pcmcia/config.h . && \
	${LN} -s ../pcmcia/config.h version.h && \
	${LN} -s ../pcmcia/config.h compile.h
	touch ${PCMCIADIR}/include/pcmcia/autoconf.h
	cd ${PCMCIADIR}/cardmgr && \
	$(CROSS_GCC) $(PCMCIA_CFLAGS) cardctl.c -o cardctl && \
	$(CROSS_GCC) $(PCMCIA_CFLAGS) cardmgr.c lex_config.c yacc_config.c -o cardmgr && \
	cp cardctl cardmgr ../..

# ----------------------- cleaning up --------------------------------------
GOALS = j820 j820-26
DIRT = \
	ramdisk/ramdiskimage* \
	ramdisk/cmdfile ramdisk/cmdfile26 ramdisk/modules*.tar.bz2
OLDCRUFT = \
	ramdisk/pcmcia/ ramdisk/busybox ramdisk/modules*.tar.bz2 \
	ramdisk/ramdiskimage* ramdisk/cmdfile* ramdisk/lib \
	linux/upstream linux/build uclibc/buildroot

localclean::
	-${RM} ${DIRT}

clean:: localclean kclean kclean26 bboxclean pcmciaclean
kclean:
	[ -d ${KBUILDDIR} ] && make -C ${KBUILDDIR} clean ; :
kclean26:
	[ -d ${KBUILDDIR26} ] && make -C ${KBUILDDIR26} clean ; :
bboxclean:
	[ -d ${BUSYBOXDIR} ] && make -C ${BUSYBOXDIR} clean ; :
	-${RM} ramdisk/busybox/busybox.bin
pcmciaclean:
	[ -d ${PCMCIADIR} ] && make -C ${PCMCIADIR} clean ; :
	-${RM} ramdisk/pcmcia/cardmgr ramdisk/pcmcia/cardctl

# remove everything but the expensively downloaded files in upstream/
mrproper:: localclean
	-${RM} -rf linux/build linux/upstream/*/ \
		${BUSYBOXDIR} ${PCMCIADIR}

nocruft:
	-${RM} -rf `find . -name '*~' -o -name '.#*'` \
		$(OLDCRUFT)

distclean:: mrproper nocruft
	-${RM} -rf ${BUILDROOT} linux/upstream \
		ramdisk/*/*.tar.* \

# ---------------- shortcuts used during development -----------------------

just-j820-from-modules: \
	modules-compilation modules-tarbz2 \
	do-ramdisk just-j820
