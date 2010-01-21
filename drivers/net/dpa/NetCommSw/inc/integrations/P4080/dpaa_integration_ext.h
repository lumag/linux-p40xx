/* Copyright (c) 2008-2010 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**

 @File          dpaa_integration_ext.h

 @Description   P4080 FM external definitions and structures.
*//***************************************************************************/
#ifndef __DPAA_INTEGRATION_EXT_H
#define __DPAA_INTEGRATION_EXT_H

#include "std_ext.h"


/*****************************************************************************
 QMan INTEGRATION-SPECIFIC DEFINITIONS
******************************************************************************/
#define QMAN_WQ_CS_CFG_ERRATA
#define QMAN_SFDR_LEAK_ERRATA_5

#define QM_MAX_NUM_OF_PORTALS   10
#define QM_MAX_NUM_OF_WQ        8

typedef enum {
    e_QM_FQ_CHANNEL_SWPORTAL0 = 0,
    e_QM_FQ_CHANNEL_SWPORTAL1,
    e_QM_FQ_CHANNEL_SWPORTAL2,
    e_QM_FQ_CHANNEL_SWPORTAL3,
    e_QM_FQ_CHANNEL_SWPORTAL4,
    e_QM_FQ_CHANNEL_SWPORTAL5,
    e_QM_FQ_CHANNEL_SWPORTAL6,
    e_QM_FQ_CHANNEL_SWPORTAL7,
    e_QM_FQ_CHANNEL_SWPORTAL8,
    e_QM_FQ_CHANNEL_SWPORTAL9,

    e_QM_FQ_CHANNEL_POOL1 = 0x21,
    e_QM_FQ_CHANNEL_POOL2,
    e_QM_FQ_CHANNEL_POOL3,
    e_QM_FQ_CHANNEL_POOL4,
    e_QM_FQ_CHANNEL_POOL5,
    e_QM_FQ_CHANNEL_POOL6,
    e_QM_FQ_CHANNEL_POOL7,
    e_QM_FQ_CHANNEL_POOL8,
    e_QM_FQ_CHANNEL_POOL9,
    e_QM_FQ_CHANNEL_POOL10,
    e_QM_FQ_CHANNEL_POOL11,
    e_QM_FQ_CHANNEL_POOL12,
    e_QM_FQ_CHANNEL_POOL13,
    e_QM_FQ_CHANNEL_POOL14,
    e_QM_FQ_CHANNEL_POOL15,

    e_QM_FQ_CHANNEL_FMAN0_SP0 = 0x40,
    e_QM_FQ_CHANNEL_FMAN0_SP1,
    e_QM_FQ_CHANNEL_FMAN0_SP2,
    e_QM_FQ_CHANNEL_FMAN0_SP3,
    e_QM_FQ_CHANNEL_FMAN0_SP4,
    e_QM_FQ_CHANNEL_FMAN0_SP5,
    e_QM_FQ_CHANNEL_FMAN0_SP6,
    e_QM_FQ_CHANNEL_FMAN0_SP7,
    e_QM_FQ_CHANNEL_FMAN0_SP8,
    e_QM_FQ_CHANNEL_FMAN0_SP9,
    e_QM_FQ_CHANNEL_FMAN0_SP10,
    e_QM_FQ_CHANNEL_FMAN0_SP11,

    e_QM_FQ_CHANNEL_FMAN1_SP0 = 0x60,
    e_QM_FQ_CHANNEL_FMAN1_SP1,
    e_QM_FQ_CHANNEL_FMAN1_SP2,
    e_QM_FQ_CHANNEL_FMAN1_SP3,
    e_QM_FQ_CHANNEL_FMAN1_SP4,
    e_QM_FQ_CHANNEL_FMAN1_SP5,
    e_QM_FQ_CHANNEL_FMAN1_SP6,
    e_QM_FQ_CHANNEL_FMAN1_SP7,
    e_QM_FQ_CHANNEL_FMAN1_SP8,
    e_QM_FQ_CHANNEL_FMAN1_SP9,
    e_QM_FQ_CHANNEL_FMAN1_SP10,
    e_QM_FQ_CHANNEL_FMAN1_SP11,

    e_QM_FQ_CHANNEL_CAAM = 0x80,
    e_QM_FQ_CHANNEL_PME = 0xa0
} e_QmFQChannel;

/*****************************************************************************
 BMan INTEGRATION-SPECIFIC DEFINITIONS
******************************************************************************/
#define BM_MAX_NUM_OF_POOLS     64
#define BM_MAX_NUM_OF_PORTALS   10

/*****************************************************************************
 FM INTEGRATION-SPECIFIC DEFINITIONS
******************************************************************************/
#define INTG_MAX_NUM_OF_FM          2

#define FM_MAX_NUM_OF_1G_RX_PORTS   4
#define FM_MAX_NUM_OF_10G_RX_PORTS  1
#define FM_MAX_NUM_OF_RX_PORTS      (FM_MAX_NUM_OF_10G_RX_PORTS+FM_MAX_NUM_OF_1G_RX_PORTS)
#define FM_MAX_NUM_OF_1G_TX_PORTS   4
#define FM_MAX_NUM_OF_10G_TX_PORTS  1
#define FM_MAX_NUM_OF_TX_PORTS      (FM_MAX_NUM_OF_10G_TX_PORTS+FM_MAX_NUM_OF_1G_TX_PORTS)
#define FM_MAX_NUM_OF_OH_PORTS      7
#define FM_MAX_NUM_OF_1G_MACS       (FM_MAX_NUM_OF_1G_RX_PORTS)
#define FM_MAX_NUM_OF_10G_MACS      (FM_MAX_NUM_OF_10G_RX_PORTS)
#define FM_MAX_NUM_OF_MACS          (FM_MAX_NUM_OF_1G_MACS+FM_MAX_NUM_OF_10G_MACS)
#define FM_MAX_NUM_OF_PCD_PORTS     (FM_MAX_NUM_OF_RX_PORTS+FM_MAX_NUM_OF_OH_PORTS)
#define FM_MAX_NUM_OF_PORTS         64

#define FM_MURAM_SIZE               (160*KILOBYTE)
#define FM_IRAM_SIZE                (4*KILOBYTE)
#define FM_PCD_PLCR_NUM_ENTRIES     256                 /**< Total number of policer profiles */
#define FM_PCD_KG_NUM_OF_SCHEMES    32                  /**< Total number of KG schemes */
#define FM_PCD_MAX_NUM_OF_CLS_PLANS 256                 /**< Number of classification plan entries. */

#define FM_RTC_NUM_OF_ALARMS            2
#define FM_RTC_NUM_OF_PERIODIC_PULSES   2
#define FM_RTC_NUM_OF_EXT_TRIGGERS      2

/**************************************************************************//**
 @Description   Enum for inter-module interrupts registration
*//***************************************************************************/
typedef enum e_FmEventModules{
    e_FM_MOD_PRS,                   /**< Parser event */
    e_FM_MOD_KG,                    /**< Keygen event */
    e_FM_MOD_PLCR,                  /**< Policer event */
    e_FM_MOD_10G_MAC,               /**< 10G MAC  error event */
    e_FM_MOD_1G_MAC,                /**< 1G MAC  error event */
    e_FM_MOD_TMR,                   /**< Timer event */
    e_FM_MOD_1G_MAC_TMR,            /**< 1G MAC  Timer event */
    e_FM_MOD_DUMMY_LAST
} e_FmEventModules;

/**************************************************************************//**
 @Description   Enum for interrupts types
*//***************************************************************************/
typedef enum e_FmIntrType {
    e_FM_INTR_TYPE_ERR,
    e_FM_INTR_TYPE_NORMAL
} e_FmIntrType;

/**************************************************************************//**
 @Description   Enum for inter-module interrupts registration
*//***************************************************************************/
typedef enum e_FmInterModuleEvent {
    e_FM_EV_PRS,                    /**< Parser event */
    e_FM_EV_ERR_PRS,                /**< Parser error event */
    e_FM_EV_KG,                     /**< Keygen event */
    e_FM_EV_ERR_KG,                 /**< Keygen error event */
    e_FM_EV_PLCR,                   /**< Policer event */
    e_FM_EV_ERR_PLCR,               /**< Policer error event */
    e_FM_EV_ERR_10G_MAC0,           /**< 10G MAC 0 error event */
    e_FM_EV_ERR_1G_MAC0,            /**< 1G MAC 0 error event */
    e_FM_EV_ERR_1G_MAC1,            /**< 1G MAC 1 error event */
    e_FM_EV_ERR_1G_MAC2,            /**< 1G MAC 2 error event */
    e_FM_EV_ERR_1G_MAC3,            /**< 1G MAC 3 error event */
    e_FM_EV_TMR,                    /**< Timer event */
    e_FM_EV_1G_MAC1,                /**< 1G MAC 1 event */
    e_FM_EV_1G_MAC2,                /**< 1G MAC 2 event */
    e_FM_EV_1G_MAC3,                /**< 1G MAC 3 event */
    e_FM_EV_1G_MAC0_TMR,            /**< 1G MAC 0 Timer event */
    e_FM_EV_1G_MAC1_TMR,            /**< 1G MAC 1 Timer event */
    e_FM_EV_1G_MAC2_TMR,            /**< 1G MAC 2 Timer event */
    e_FM_EV_1G_MAC3_TMR,            /**< 1G MAC 3 Timer event */
    e_FM_EV_DUMMY_LAST
} e_FmInterModuleEvent;

#define GET_FM_MODULE_EVENT(mod, id, intrType, event)                                               \
    switch(mod){                                                                                    \
        case e_FM_MOD_PRS:                                                                          \
            if (id) event = e_FM_EV_DUMMY_LAST;                                                     \
            else event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_PRS:e_FM_EV_PRS;            \
            break;                                                                                  \
        case e_FM_MOD_KG:                                                                           \
            if (id) event = e_FM_EV_DUMMY_LAST;                                                     \
            else event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_KG:e_FM_EV_DUMMY_LAST;      \
            break;                                                                                  \
        case e_FM_MOD_PLCR:                                                                         \
            if (id) event = e_FM_EV_DUMMY_LAST;                                                     \
            else event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_PLCR:e_FM_EV_PLCR;          \
            break;                                                                                  \
        case e_FM_MOD_10G_MAC:                                                                      \
            if (id) event = e_FM_EV_DUMMY_LAST;                                                     \
            else event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_10G_MAC0:e_FM_EV_DUMMY_LAST;\
            break;                                                                                  \
        case e_FM_MOD_1G_MAC:                                                                       \
            switch(id){                                                                             \
                 case(0): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_1G_MAC0:e_FM_EV_DUMMY_LAST; break; \
                 case(1): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_1G_MAC1:e_FM_EV_1G_MAC1; break;    \
                 case(2): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_1G_MAC2:e_FM_EV_1G_MAC2; break;    \
                 case(3): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_1G_MAC3:e_FM_EV_1G_MAC3; break;    \
                 }                                                                                  \
            break;                                                                                  \
        case e_FM_MOD_TMR:                                                                          \
            if (id) event = e_FM_EV_DUMMY_LAST;                                                     \
            else event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_DUMMY_LAST:e_FM_EV_TMR;         \
            break;                                                                                  \
        case e_FM_MOD_1G_MAC_TMR:                                                                   \
            switch(id){                                                                             \
                 case(0): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_DUMMY_LAST:e_FM_EV_1G_MAC0_TMR; break; \
                 case(1): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_DUMMY_LAST:e_FM_EV_1G_MAC1_TMR; break; \
                 case(2): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_DUMMY_LAST:e_FM_EV_1G_MAC2_TMR; break; \
                 case(3): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_DUMMY_LAST:e_FM_EV_1G_MAC3_TMR; break; \
                 }                                                                                  \
            break;                                                                                  \
        default:event = e_FM_EV_DUMMY_LAST;                                                         \
        break;}

#ifndef SIMULATOR
#define FM_10G_MAC_NO_CTRL_LOOPBACK
#define BUP_FM_10G_TX_ECC_FRMS_ERRATA
#endif /* !SIMULATOR */

/* FM erratas */
#define FM_OP_PARTITION_ERRATA_FMAN16
#define FM_RAM_LIST_ERR_IRQ_ERRATA_FMAN11
#define FM_IRAM_ECC_ERR_IRQ_ERRATA
#define FM_PORT_SYNC_ERRATA_FMAN6
#define FM_PRS_MEM_ERRATA
#define FM_IEEE_BAD_TS_ERRATA_IPG28055
#define FM_HALT_SIG_ERRATA_GEN_CCB310
#define FM_PRS_L4_SHELL_ERRATA
#define FM_1G_SHORT_PAUSE_TIME_ERRATA_DTSEC1
#define BUP_FM_LEN_CHECK_ERRATA
#define BUP_FM_10G_REM_N_LCL_FLT_EX_ERRATA
#define BUP_FM_MDIO_ERRATA


#endif /* __DPAA_INTEGRATION_EXT_H */
