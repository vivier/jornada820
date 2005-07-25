# Jornada820 version based on Makefile.boot from linux 2.6.11.9
# $Id: Makefile.boot,v 1.2 2005/07/25 09:09:12 fare Exp $
# grep for 820 for modifications.

   zreladdr-y	:= 0xc0008000
ifeq ($(CONFIG_ARCH_SA1100),y)
   zreladdr-$(CONFIG_SA1111)		:= 0xc0208000
endif
params_phys-y	:= 0xc0000100
initrd_phys-y	:= 0xc0800000

   zreladdr-$(CONFIG_SA1100_JORNADA820)	:= 0xc0208000
  ztextaddr-$(CONFIG_SA1100_JORNADA820)	:= 0xc0800000
params_phys-$(CONFIG_SA1100_JORNADA820)	:= 0xc0200100
initrd_phys-$(CONFIG_SA1100_JORNADA820)	:= 0xc0c00000
