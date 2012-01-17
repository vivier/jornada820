# This is a Makefile for integrating the linux kernel
# as a component of the uclibc buildroot system

linux-compilation:
	make -C $(BUILD_DIR)/../../.. all-kernel-compilation

linux: toolchain linux-compilation

linux-source:

linux-clean:

linux-dirclean:

#############################################################
#
# Toplevel Makefile options
#
#############################################################
ifeq ($(strip $(BR2_PACKAGE_LINUX)),y)
TARGETS+=linux
endif
