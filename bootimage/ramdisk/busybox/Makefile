OS                      = linux
TARGET                  = arm-linux

# ----------------- busybox files     --------------------------------------

#BUSYBOXVER		= 1.00-pre7
#BUSYBOXTAR		= busybox-${BUSYBOXVER}.tar.bz2
#BUSYBOXDIR		= busybox-${BUSYBOXVER}
BUSYBOXDIR		= src

# ------------- programs ---------------------------------------------------
LN                      = ln
MKDIR                   = mkdir
TEE                     = tee
TOUCH                   = touch
CP                      = cp -f
RM                      = rm -f

# ---------------------------------------------------------------------------
default: busybox

${BUSYBOXTAR}:
	wget http://busybox.net/downloads/${BUSYBOXTAR}

busybox-dir: ${BUSYBOXTAR}
	@bunzip2 -c ${BUSYBOXTAR} | tar xf - && \
	touch $@

src/CVS/Root src/CVS/Repository:
	mkdir -p src/CVS ; cd src/CVS || exit 2 ; \
	echo :pserver:anonymous@busybox.net:/var/cvs > Root ; \
	echo busybox > Repository ; \
	touch Entries ; \
	grep -q ':pserver:anonymous@busybox.net:/var/cvs' ~/.cvspass || \
	echo '/1 :pserver:anonymous@busybox.net:/var/cvs A' >> ~/.cvspass

busybox-cvs: src/CVS/Root	
	cd src && cvs -z9 update -dfP

compile:
	@cd ${BUSYBOXDIR} && \
	cp ../busybox.cfg .config && \
	${MAKE}	CROSS=${TARGET}- oldconfig && \
	${MAKE} CROSS=${TARGET}- busybox && \
	${MAKE} CROSS=${TARGET}- install && \
	cp busybox.links .. && \
	cp _install/bin/busybox ..

busybox: busybox-cvs compile

localclean:
	$(RM) busybox-dir busybox.links busybox

clean:	localclean

distclean veryclean mrproper:: localclean
	$(RM) -rf ${BUSYBOXDIR}
	#$(RM) ${BUSYBOXTAR}
