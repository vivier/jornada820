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
DEBUGFS		 	= debugfs
CANONICAL_PATH          = /bin:/usr/bin

# ---------------------------------------------------------------------------

RAMDISKIMAGE=ramdiskimage

all: ramdiskimage

ramdiskimage: cmdfile
	dd if=/dev/zero of=ramdiskimage count=3072 bs=1024
	mke2fs -F ramdiskimage
	debugfs -f cmdfile

# ----------------- ramdiskimage command file ------------------------------

cmdfile: cmdfile.in busybox pcmcia
	cat cmdfile.in > $@
	sed "s'.*'ln /bin/busybox &'" < busybox/busybox.links >> $@
	echo "cd /" >> $@
	for d in `find etc -type d | grep -v CVS` ; do \
	  echo "mkdir /$$d" >> $@; \
	done
	for f in `find etc -type f | grep -v CVS` ; do \
          dir=`dirname $$f`; \
          file=`basename $$f`; \
	  echo "cd /$$dir" >> $@; \
	  echo "write $$f $$file" >> $@; \
	done
	echo "cd /" >> $@
	for d in `find lib -type d` ; do \
	  echo "mkdir /$$d" >> $@; \
	done
	for f in `find lib -type f | grep -v CVS` ; do \
          dir=`dirname $$f`; \
          file=`basename $$f`; \
          echo "cd /$$dir" >> $@; \
          echo "write $$f $$file" >> $@; \
        done

# ---------------------------  busybox  -------------------------------------

busybox:
	${MAKE} -C busybox TARGET=${TARGET} OS=${OS}

pcmcia:
	${MAKE} -C pcmcia TARGET=${TARGET} OS=${OS}

# ---------------------------------------------------------------------------

clean:
	$(RM) ramdiskimage *~ cmdfile
	${MAKE} -C busybox clean
	${MAKE} -C pcmcia clean
	$(RM) -rf lib/modules
