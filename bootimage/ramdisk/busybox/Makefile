OS                      = linux
TARGET                  = arm-linux

# ----------------- busybox files     --------------------------------------

BUSYBOXVER		= 1.00-pre7
BUSYBOXTAR		= busybox-${BUSYBOXVER}.tar.bz2
BUSYBOXDIR		= busybox-${BUSYBOXVER}

# ------------- programs ---------------------------------------------------
LN                      = ln
MKDIR                   = mkdir
TEE                     = tee
TOUCH                   = touch
CP                      = cp -f
RM                      = rm -f
CANONICAL_PATH          = /bin:/usr/bin

# ---------------------------------------------------------------------------
default: busybox

${BUSYBOXTAR}:
	wget http://busybox.net/downloads/${BUSYBOXTAR}

busybox-dir: ${BUSYBOXTAR}
	@bunzip2 -c ${BUSYBOXTAR} | tar xf - && \
	touch $@

busybox: busybox-dir
	@cd ${BUSYBOXDIR} && \
	cp ../busybox.cfg .config && \
	${MAKE}	CROSS=${TARGET}- oldconfig && \
	${MAKE} CROSS=${TARGET}- busybox && \
	${MAKE} CROSS=${TARGET}- install && \
	cp busybox.links .. && \
	cp _install/bin/busybox ..

clean:
	$(RM) -rf ${BUSYBOXTAR} ${BUSYBOXDIR}
	$(RM) busybox-dir busybox.links busybox
