#
# Makefile for the Freescale Ethernet controllers
#
EXTRA_CFLAGS           += -DVERSION=\"\"
#
#Include netcomm SW specific definitions
include $(srctree)/drivers/net/dpa/NetCommSw/ncsw_config.mk

EXTRA_CFLAGS += -I$(NET_DPA)

#Netcomm SW tree
obj-$(CONFIG_FSL_FMAN) += NetCommSw/
obj-$(CONFIG_DPA) += fsl-mac.o fsl-dpa.o

fsl-dpa-objs	:= dpa-ethtool.o dpa.o
fsl-mac-objs	:= mac.o mac-api.o
