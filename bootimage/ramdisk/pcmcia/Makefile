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

CC			= ${TARGET}-gcc
CFLAGS			= -Os -I../include/static -I../../../../linux/kernel/include -I../include -I../modules -static -s


# ---------------------------------------------------------------------------
default: pcmcia-install

${PCMCIATAR}:
	wget http://pcmcia-cs.sourceforge.net/ftp/${PCMCIATAR}

pcmcia-dir: ${PCMCIATAR}
	tar zxf ${PCMCIATAR} && \
	touch $@


pcmcia-install: pcmcia-dir cardmgr cardctl
cardmgr cardctl:
	@-cd ${PCMCIADIR}/include/linux && \
	${LN} -s ../pcmcia/config.h . && \
	${LN} -s ../pcmcia/config.h version.h && \
	${LN} -s ../pcmcia/config.h compile.h
	touch ${PCMCIADIR}/include/pcmcia/autoconf.h
	cd ${PCMCIADIR}/cardmgr && \
	$(CC) $(CFLAGS) cardctl.c -o cardctl && \
	$(CC) $(CFLAGS) cardmgr.c lex_config.c yacc_config.c -o cardmgr && \
	cp cardctl cardmgr ../..

localclean:
	$(RM) -rf ${PCMCIADIR}
	$(RM) pcmcia-dir cardmgr cardctl

clean:	localclean

distclean veryclean mrproper:: localclean
	$(RM) ${PCMCIATAR}
