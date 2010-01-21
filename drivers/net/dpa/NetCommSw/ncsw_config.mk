#
# Makefile config for the Freescale NetcommSW
#
NET_DPA     = $(srctree)/drivers/net
DRV_DPA     = $(srctree)/drivers/net/dpa
NCSW        = $(srctree)/drivers/net/dpa/NetCommSw

EXTRA_CFLAGS +=-include $(NCSW)/dflags.h

EXTRA_CFLAGS += -I$(DRV_DPA)/
EXTRA_CFLAGS += -I$(NCSW)/inc
EXTRA_CFLAGS += -I$(NCSW)/inc/cores
EXTRA_CFLAGS += -I$(NCSW)/inc/etc
EXTRA_CFLAGS += -I$(NCSW)/inc/Peripherals
EXTRA_CFLAGS += -I$(NCSW)/inc/integrations
EXTRA_CFLAGS += -I$(NCSW)/inc/integrations/P4080
EXTRA_CFLAGS += -I$(NCSW)/user/env/linux/kernel/2.6/inc
EXTRA_CFLAGS += -I$(NCSW)/user/env/linux/kernel/2.6/inc/system
EXTRA_CFLAGS += -I$(NCSW)/user/env/linux/kernel/2.6/inc/wrappers/Peripherals
EXTRA_CFLAGS += -I$(NCSW)/user/env/linux/kernel/2.6/inc/ioctl
EXTRA_CFLAGS += -I$(NCSW)/user/env/linux/kernel/2.6/inc/ioctl/Peripherals
EXTRA_CFLAGS += -I$(NCSW)/user/env/linux/kernel/2.6/inc/ioctl/integrations/P4080
