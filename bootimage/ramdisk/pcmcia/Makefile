OS                      = linux
TARGET                  = arm-linux

# ----------------- busybox files     --------------------------------------

PCMCIAVER		= 3.2.7
PCMCIATAR		= pcmcia-cs-${PCMCIAVER}.tar.gz
PCMCIADIR		= pcmcia-cs-${PCMCIAVER}

# ------------- programs ---------------------------------------------------
LN                      = ln
MKDIR                   = mkdir
TEE                     = tee
TOUCH                   = touch
CP                      = cp -f
RM                      = rm -f
CANONICAL_PATH          = /bin:/usr/bin

# ---------------------------------------------------------------------------
default: pcmcia-install

${PCMCIATAR}:
	wget http://pcmcia-cs.sourceforge.net/ftp/${PCMCIATAR}

pcmcia-dir: ${PCMCIATAR}
	@tar zxf ${PCMCIATAR} && \
	touch $@


pcmcia-install: pcmcia-dir cardmgr cardctl
cardmgr cardctl:
	@-cd ${PCMCIADIR}/include/linux && \
	${LN} -s ../pcmcia/config.h . && \
	${LN} -s ../pcmcia/config.h version.h && \
	${LN} -s ../pcmcia/config.h compile.h

	@cd ${PCMCIADIR} && \
	${MAKE} oldconfig && \
	cd cardmgr && \
	${MAKE} CC=${TARGET}-gcc cardmgr cardctl && \
	${TARGET}-strip cardmgr cardctl && \
	cp cardctl cardmgr ../..

clean:
	$(RM) -rf ${PCMCIATAR} ${PCMCIADIR}
	$(RM) pcmcia-dir cardmgr cardctl
