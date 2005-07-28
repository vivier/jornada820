#!/bin/sh

### Make a debugfs command file so as to create the ramdisk
### Note: for 2.6, we'll have to migrate to early-userspace

RAMDISKIMAGE=$1 RAMDISKDIR=$2 BUILDDIR=$3 TARGETDIR=$4

# list of directories to mirror on the ramdiskimage
DIRECTORIES=" etc "

cat <<END
open -w ${RAMDISKIMAGE}
mkdir /bin
mkdir /sbin
mkdir /lib
mkdir /lib/modules
mkdir /usr
mkdir /usr/sbin
mkdir /usr/bin
mkdir /dev
mkdir /proc
mkdir /var
mkdir /var/run
mkdir /tmp
mkdir /mnt
mkdir /mnt/dos
mkdir /mnt/linux

cd /bin
write ${BUILDDIR}/busybox.bin busybox
cd /sbin
write ${BUILDDIR}/cardmgr cardmgr
write ${BUILDDIR}/cardctl cardctl

END

cd ${RAMDISKDIR}

for d in `find . -type d | grep -vi cvs | cut -c3-` ; do
	echo "mkdir /$d"
done
for f in `find . -type f | grep -vi cvs | cut -c3-` ; do
	dir=`dirname $f`
	file=`basename $f`
	echo "cd /$dir"
	echo "write $f $file"
done

cat <<END
cd /
write ${TARGETDIR}/modules.tar.bz2 modules.tar.bz2
END
