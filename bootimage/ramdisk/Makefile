#
#
.PHONY: busybox pcmcia

OS                      = linux
TARGET                  = arm-linux

# ------------- locations ------------------------------------------------
JP_PCMCIADIR		= pcmcia-cs-3.2.7


# ------------- programs ---------------------------------------------------
LN                      = ln
MKDIR                   = mkdir
TEE                     = tee
TOUCH                   = touch
CP                      = cp -f
RM                      = rm -f
MKE2FS		 	= mke2fs
DEBUGFS		 	= debugfs
E2FSCK		 	= e2fsck
CANONICAL_PATH          = /bin:/usr/bin

# ---------------------------------------------------------------------------

RAMDISKIMAGE=ramdiskimage

all: ramdiskimage.gz

ramdiskimage: cmdfile
	dd if=/dev/zero of=ramdiskimage count=3072 bs=1024
	$(MKE2FS) -F ramdiskimage
	$(DEBUGFS) -f cmdfile
	$(E2FSCK) -f -y ramdiskimage ; :

ramdiskimage.gz: ramdiskimage
	gzip -9 < $< > $@

ramdisk26: ramdiskimage26.gz
ramdiskimage26: cmdfile26
	dd if=/dev/zero of=ramdiskimage26 count=3072 bs=1024
	$(MKE2FS) -F ramdiskimage26
	$(DEBUGFS) -f cmdfile26
	$(E2FSCK) -f -y ramdiskimage26 ; :
ramdiskimage26.gz: ramdiskimage26
	gzip -9 < $< > $@

# ----------------- ramdiskimage command file ------------------------------

cmdfile: cmdfile.in busybox pcmcia
	( echo open -w ramdiskimage ; \
	cat cmdfile.in ; \
	echo "cd /" ; \
	echo write modules.tar.bz2 modules.tar.bz2 ; \
	for d in `find etc -type d | grep -v CVS` ; do \
	  echo "mkdir /$$d" ; \
	done ; \
	for f in `find etc -type f | grep -v CVS` ; do \
	  dir=`dirname $$f`; \
	  file=`basename $$f`; \
	  echo "cd /$$dir" ; \
	  echo "write $$f $$file" ; \
	done ; \
	echo "cd /" ) > $@
	#for d in `find lib -type d` ; do \
	#  echo "mkdir /$$d" ; \
	#done
	#for f in `find lib -type f | grep -vi cvs` ; do \
	#  dir=`dirname $$f`; \
	#  file=`basename $$f`; \
	#  echo "cd /$$dir" ; \
	#  echo "write $$f $$file" ; \
	#done


cmdfile26: cmdfile.in busybox pcmcia
	( echo open -w ramdiskimage26 ; \
	cat cmdfile.in ; \
	echo "cd /" ; \
	echo write modules26.tar.bz2 modules.tar.bz2 ; \
	for d in `find etc -type d | grep -v CVS` ; do \
	  echo "mkdir /$$d" ; \
	done ; \
	for f in `find etc -type f | grep -v CVS` ; do \
	  dir=`dirname $$f`; \
	  file=`basename $$f`; \
	  echo "cd /$$dir" ; \
	  echo "write $$f $$file" ; \
	done ; \
	echo "cd /" ) > $@

# ---------------------------  busybox  -------------------------------------

busybox:
	${MAKE} -C busybox TARGET=${TARGET} OS=${OS}

pcmcia:
	${MAKE} -C pcmcia TARGET=${TARGET} OS=${OS}

# ---------------------------------------------------------------------------

localclean:
	$(RM) ramdiskimage* *~ cmdfile modules.tar.bz2
	$(RM) -rf lib/modules

clean: localclean
	${MAKE} -C busybox clean
	${MAKE} -C pcmcia clean

distclean mrproper: localclean
	${MAKE} -C busybox distclean
	${MAKE} -C pcmcia distclean
