OS                      = linux
TARGET                  = arm-linux

# ----------------- busybox files     --------------------------------------

BUSYBOXVER		= 1.00
BUSYBOXTAR		= busybox-${BUSYBOXVER}.tar.bz2
BUSYBOXDIR		= busybox-${BUSYBOXVER}
#BUSYBOXDIR		= busybox

# ------------- programs ---------------------------------------------------
LN                      = ln
MKDIR                   = mkdir
TEE                     = tee
TOUCH                   = touch
CP                      = cp -f
RM                      = rm -f

# ---------------------------------------------------------------------------
default: busybox.bin

${BUSYBOXTAR}:
	wget http://busybox.net/downloads/${BUSYBOXTAR}

busybox-dir: ${BUSYBOXTAR}
	@bunzip2 -c ${BUSYBOXTAR} | tar xf - && \
	touch $@

src/CVS/Root:
	mkdir -p src/CVS ; cd src/CVS || exit 2 ; \
	echo :pserver:anonymous@busybox.net:/var/cvs > Root ; \
	echo busybox > Repository ; \
	touch Entries ; \
	grep -q ':pserver:anonymous@busybox.net:/var/cvs' ~/.cvspass || \
	echo '/1 :pserver:anonymous@busybox.net:/var/cvs A' >> ~/.cvspass

busybox-cvs: src/CVS/Root
	cd src && cvs -z9 update -dfP
	touch $@

busybox-svn:
	svn co svn://busybox.net/trunk/busybox

compile: ${BUSYBOXDIR}/_install/bin/busybox
	@cd ${BUSYBOXDIR} && \
	cp _install/bin/busybox ../busybox.bin

${BUSYBOXDIR}/_install/bin/busybox:
	@cd ${BUSYBOXDIR} && \
	cp ../busybox.cfg .config && \
	${MAKE}	CROSS=${TARGET}- oldconfig && \
	${MAKE} CROSS=${TARGET}- busybox && \
	${MAKE} CROSS=${TARGET}- install

busybox.bin: busybox-dir compile

localclean:
	$(RM) busybox-dir busybox.links busybox

clean:	localclean

distclean veryclean mrproper:: localclean
	$(RM) -rf ${BUSYBOXDIR} busybox-cvs
	#$(RM) ${BUSYBOXTAR}
