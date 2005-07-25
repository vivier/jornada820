# Jornada820 version based on Makefile from linux 2.6.11.9
# $Id: Makefile,v 1.2 2005/07/25 09:09:11 fare Exp $

#
# Makefile for the linux kernel.
#

obj-y				+= rtctime.o
obj-$(CONFIG_ARM_AMBA)		+= amba.o
obj-$(CONFIG_ICST525)		+= icst525.o
obj-$(CONFIG_ICST307)		+= icst307.o
obj-$(CONFIG_SA1101)		+= sa1101.o ### ONLY JORNADA820 MODIFICATION
obj-$(CONFIG_SA1111)		+= sa1111.o
obj-$(CONFIG_PCI_HOST_VIA82C505) += via82c505.o
obj-$(CONFIG_DMABOUNCE)		+= dmabounce.o
obj-$(CONFIG_TIMER_ACORN)	+= time-acorn.o
obj-$(CONFIG_SHARP_LOCOMO)	+= locomo.o
obj-$(CONFIG_SHARP_SCOOP)	+= scoop.o
