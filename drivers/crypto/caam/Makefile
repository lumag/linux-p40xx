
#
# Makefile for the CAAM backend and dependent components, including:
#   Descriptor builder library (for inside the Kernel)
#   Datapath test module
#

obj-$(CONFIG_CRYPTO_DEV_FSL_CAAM) += caam.o

obj-$(CONFIG_CRYPTO_DEV_FSL_CAAM_DC_LIB) += dcl/
obj-$(CONFIG_CRYPTO_DEV_FSL_CAAM_CRYPTO_API) += algapi.o
obj-$(CONFIG_CRYPTO_DEV_FSL_CAAM_JQ_TEST) += jq_test/

caam-objs := ctrl.o jq.o
