/* Copyright (c) 2008-2009 Freescale Semiconductor, Inc.
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

 @File          part_integration_ext.h

 @Description   P4080 external definitions and structures.
*//***************************************************************************/
#ifndef __PART_INTEGRATION_EXT_H
#define __PART_INTEGRATION_EXT_H


/**************************************************************************//**
 @Group         P4080_chip_id P4080 Application Programming Interface

 @Description   P4080 Chip functions,definitions and enums.

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Description   Module types.
*//***************************************************************************/
typedef enum e_ModuleId
{
    e_MODULE_ID_DUART_1 = 0,
    e_MODULE_ID_DUART_2,
    e_MODULE_ID_DUART_3,
    e_MODULE_ID_DUART_4,
    e_MODULE_ID_QM,                 /**< Queue manager module */
    e_MODULE_ID_BM,                 /**< Buffer manager module */

    e_MODULE_ID_FM1,                /**< Frame manager #1 module */
    e_MODULE_ID_FM1_MURAM,          /**< FM Multi-User-RAM */
    e_MODULE_ID_FM1_BMI,            /**< FM BMI block */
    e_MODULE_ID_FM1_QMI,            /**< FM QMI block */
    e_MODULE_ID_FM1_PRS,            /**< FM parser block */
    e_MODULE_ID_FM1_PORT_HO0,       /**< FM Host-command/offline-parsing port block */
    e_MODULE_ID_FM1_PORT_HO1,       /**< FM Host-command/offline-parsing port block */
    e_MODULE_ID_FM1_PORT_HO2,       /**< FM Host-command/offline-parsing port block */
    e_MODULE_ID_FM1_PORT_HO3,       /**< FM Host-command/offline-parsing port block */
    e_MODULE_ID_FM1_PORT_HO4,       /**< FM Host-command/offline-parsing port block */
    e_MODULE_ID_FM1_PORT_HO5,       /**< FM Host-command/offline-parsing port block */
    e_MODULE_ID_FM1_PORT_HO6,       /**< FM Host-command/offline-parsing port block */
    e_MODULE_ID_FM1_PORT_1GRx0,     /**< FM Rx 1G MAC port block */
    e_MODULE_ID_FM1_PORT_1GRx1,     /**< FM Rx 1G MAC port block */
    e_MODULE_ID_FM1_PORT_1GRx2,     /**< FM Rx 1G MAC port block */
    e_MODULE_ID_FM1_PORT_1GRx3,     /**< FM Rx 1G MAC port block */
    e_MODULE_ID_FM1_PORT_10GRx0,    /**< FM Rx 10G MAC port block */
    e_MODULE_ID_FM1_PORT_1GTx0,     /**< FM Tx 1G MAC port block */
    e_MODULE_ID_FM1_PORT_1GTx1,     /**< FM Tx 1G MAC port block */
    e_MODULE_ID_FM1_PORT_1GTx2,     /**< FM Tx 1G MAC port block */
    e_MODULE_ID_FM1_PORT_1GTx3,     /**< FM Tx 1G MAC port block */
    e_MODULE_ID_FM1_PORT_10GTx0,    /**< FM Tx 10G MAC port block */
    e_MODULE_ID_FM1_PLCR,           /**< FM Policer */
    e_MODULE_ID_FM1_KG,             /**< FM Keygen */
    e_MODULE_ID_FM1_DMA,            /**< FM DMA */
    e_MODULE_ID_FM1_FPM,            /**< FM FPM */
    e_MODULE_ID_FM1_IRAM,           /**< FM Instruction-RAM */
    e_MODULE_ID_FM1_1GMDIO0,        /**< FM 1G MDIO MAC 0*/
    e_MODULE_ID_FM1_1GMDIO1,        /**< FM 1G MDIO MAC 1*/
    e_MODULE_ID_FM1_1GMDIO2,        /**< FM 1G MDIO MAC 2*/
    e_MODULE_ID_FM1_1GMDIO3,        /**< FM 1G MDIO MAC 3*/
    e_MODULE_ID_FM1_10GMDIO,        /**< FM 10G MDIO */
    e_MODULE_ID_FM1_PRS_IRAM,       /**< FM SW-parser Instruction-RAM */
    e_MODULE_ID_FM1_RISC0,          /**< FM risc #0 */
    e_MODULE_ID_FM1_RISC1,          /**< FM risc #1 */
    e_MODULE_ID_FM1_1GMAC0,         /**< FM 1G MAC #0 */
    e_MODULE_ID_FM1_1GMAC1,         /**< FM 1G MAC #1 */
    e_MODULE_ID_FM1_1GMAC2,         /**< FM 1G MAC #2 */
    e_MODULE_ID_FM1_1GMAC3,         /**< FM 1G MAC #3 */
    e_MODULE_ID_FM1_10GMAC0,        /**< FM 10G MAC #0 */

    e_MODULE_ID_FM2,                /**< Frame manager #2 module */
    e_MODULE_ID_FM2_MURAM,          /**< FM Multi-User-RAM */
    e_MODULE_ID_FM2_BMI,            /**< FM BMI block */
    e_MODULE_ID_FM2_QMI,            /**< FM QMI block */
    e_MODULE_ID_FM2_PRS,            /**< FM parser block */
    e_MODULE_ID_FM2_PORT_HO0,       /**< FM Host-command/offline-parsing port block */
    e_MODULE_ID_FM2_PORT_HO1,       /**< FM Host-command/offline-parsing port block */
    e_MODULE_ID_FM2_PORT_HO2,       /**< FM Host-command/offline-parsing port block */
    e_MODULE_ID_FM2_PORT_HO3,       /**< FM Host-command/offline-parsing port block */
    e_MODULE_ID_FM2_PORT_HO4,       /**< FM Host-command/offline-parsing port block */
    e_MODULE_ID_FM2_PORT_HO5,       /**< FM Host-command/offline-parsing port block */
    e_MODULE_ID_FM2_PORT_HO6,       /**< FM Host-command/offline-parsing port block */
    e_MODULE_ID_FM2_PORT_1GRx0,     /**< FM Rx 1G MAC port block */
    e_MODULE_ID_FM2_PORT_1GRx1,     /**< FM Rx 1G MAC port block */
    e_MODULE_ID_FM2_PORT_1GRx2,     /**< FM Rx 1G MAC port block */
    e_MODULE_ID_FM2_PORT_1GRx3,     /**< FM Rx 1G MAC port block */
    e_MODULE_ID_FM2_PORT_10GRx0,    /**< FM Rx 10G MAC port block */
    e_MODULE_ID_FM2_PORT_1GTx0,     /**< FM Tx 1G MAC port block */
    e_MODULE_ID_FM2_PORT_1GTx1,     /**< FM Tx 1G MAC port block */
    e_MODULE_ID_FM2_PORT_1GTx2,     /**< FM Tx 1G MAC port block */
    e_MODULE_ID_FM2_PORT_1GTx3,     /**< FM Tx 1G MAC port block */
    e_MODULE_ID_FM2_PORT_10GTx0,    /**< FM Tx 10G MAC port block */
    e_MODULE_ID_FM2_PLCR,           /**< FM Policer */
    e_MODULE_ID_FM2_KG,             /**< FM Keygen */
    e_MODULE_ID_FM2_DMA,            /**< FM DMA */
    e_MODULE_ID_FM2_FPM,            /**< FM FPM */
    e_MODULE_ID_FM2_IRAM,           /**< FM Instruction-RAM */
    e_MODULE_ID_FM2_1GMDIO0,        /**< FM 1G MDIO MAC 0*/
    e_MODULE_ID_FM2_1GMDIO1,        /**< FM 1G MDIO MAC 1*/
    e_MODULE_ID_FM2_1GMDIO2,        /**< FM 1G MDIO MAC 2*/
    e_MODULE_ID_FM2_1GMDIO3,        /**< FM 1G MDIO MAC 3*/
    e_MODULE_ID_FM2_10GMDIO,        /**< FM 10G MDIO */
    e_MODULE_ID_FM2_PRS_IRAM,       /**< FM SW-parser Instruction-RAM */
    e_MODULE_ID_FM2_RISC0,          /**< FM risc #0 */
    e_MODULE_ID_FM2_RISC1,          /**< FM risc #1 */
    e_MODULE_ID_FM2_1GMAC0,         /**< FM 1G MAC #0 */
    e_MODULE_ID_FM2_1GMAC1,         /**< FM 1G MAC #1 */
    e_MODULE_ID_FM2_1GMAC2,         /**< FM 1G MAC #2 */
    e_MODULE_ID_FM2_1GMAC3,         /**< FM 1G MAC #3 */
    e_MODULE_ID_FM2_10GMAC0,        /**< FM 10G MAC #0 */

    e_MODULE_ID_MPIC,               /**< MPIC */
    e_MODULE_ID_DUMMY_LAST
} e_ModuleId;

#define NUM_OF_MODULES  e_MODULE_ID_DUMMY_LAST

/**************************************************************************//**
 @Description   Transaction source id (for memory conrollers error reporting DDR,LBC,ECM).
*//***************************************************************************/
typedef enum e_TransSrc
{
    e_TRANS_SRC_LBC             = 0x4,  /**< Enhanced local bus      */
    e_TRANS_SRC_BOOTS           = 0xA,  /**< Boot sequencer          */
    e_TRANS_SRC_DDR1            = 0xF,  /**< DDR controller 1        */
    e_TRANS_SRC_CORE_INS_FETCH  = 0x10, /**< Processor (instruction) */
    e_TRANS_SRC_CORE_DATA       = 0x11, /**< Processor (data)        */
    e_TRANS_SRC_DMA             = 0x15, /**< DMA                     */
} e_TransSrc;


/***************************************************************
    P4080 general routines
****************************************************************/
/**************************************************************************//**
 @Group         P4080_init_grp P4080 Initialization Unit

 @Description   P4080 initialization unit API functions, definitions and enums

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Description   Part ID and revision number
*//***************************************************************************/
typedef enum e_P4080DeviceName
{
    e_P4080_REV_INVALID   = 0x00000000,   /**< Invalid revision      */
    e_P4080E_REV_1_0      = 0x80230010    /**< P4080E with security, revision 1.0 */
} e_P4080DeviceName;


/**************************************************************************//**
 @Function      P4080_ConfigAndInit

 @Description   General initiation of the chip registers.

 @Param         baseAddress  - (In) memory map start

 @Return        A handle to the P4080 data structure.
*//***************************************************************************/
t_Handle P4080_ConfigAndInit(uint32_t baseAddress);

/**************************************************************************//**
 @Function      P4080_Free

 @Description   Free all resources.

 @Param         h_P4080 - (In) The handle of the initialized P4080 object.

 @Return        E_OK on success; Other value otherwise.
*//***************************************************************************/
t_Error P4080_Free(t_Handle h_P4080);

/**************************************************************************//**
 @Function      P4080_GetModuleBase

 @Description   returns the base address of a P4080 module's
                memory mapped registers.

 @Param         h_P4080   - (In) The handle of the initialized P4080 object.
 @Param         module      - (In) The module ID.

 @Return        Base address of module's memory mapped registers.
                ILLEGAL_BASE in case of non-existent module
*//***************************************************************************/
uint32_t P4080_GetModuleBase(t_Handle h_P4080, e_ModuleId module);

/**************************************************************************//**
 @Function      P4080_GetRevInfo

 @Description   This routine enables access to chip and revision information.

 @Param         h_P4080 - (In) The handle of the initialized P4080 object.

 @Return        Part ID and revision.
*//***************************************************************************/
e_P4080DeviceName P4080_GetRevInfo(t_Handle h_P4080);

/**************************************************************************//**
 @Function      P4080_GetE500Factor

 @Description   returns system multiplication factor.

 @Param         h_P4080 - (In) a handle to the P4080 object.
 @Param         p_E500MulFactor   - (Out) returns E500 to CCB multification factor.
 @Param         p_E500DivFactor   - (Out) returns E500 to CCB division factor.

 @Return        E_OK on success; Other value otherwise.
*
*//***************************************************************************/
t_Error P4080_GetE500Factor(t_Handle h_P4080, uint32_t *p_E500MulFactor, uint32_t *p_E500DivFactor);

/**************************************************************************//**
 @Function      P4080_GetCcbFactor

 @Description   returns system multiplication factor.

 @Param         h_P4080 - (In) The handle of the initialized P4080 object.

 @Return        System multiplication factor.
*//***************************************************************************/
uint32_t P4080_GetCcbFactor(t_Handle h_P4080);

/** @} */ /* end of P4080_init_grp group */
/** @} */ /* end of P4080_grp group */


/*****************************************************************************
 INTEGRATION-SPECIFIC MODULE CODES
******************************************************************************/
#define MODULE_UNKNOWN          0x00000000
#define MODULE_MEM              0x00010000
#define MODULE_MM               0x00020000
#define MODULE_CORE             0x00030000
#define MODULE_PM               0x00040000
#define MODULE_P4080            0x00050000
#define MODULE_P4080_PLTFRM     0x00060000
#define MODULE_MMU              0x00070000
#define MODULE_EPIC             0x00080000
#define MODULE_FM               0x00090000
#define MODULE_QM               0x000a0000
#define MODULE_BM               0x000b0000
#define MODULE_DUART            0x000c0000

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
#define FM_MAX_NUM_OF_OP_PORTS      6
#define FM_MAX_NUM_OF_HC_PORTS      1
#define FM_MAX_NUM_OF_1G_MACS       (FM_MAX_NUM_OF_1G_RX_PORTS)
#define FM_MAX_NUM_OF_10G_MACS      (FM_MAX_NUM_OF_10G_RX_PORTS)
#define FM_MAX_NUM_OF_MACS          (FM_MAX_NUM_OF_1G_MACS+FM_MAX_NUM_OF_10G_MACS)
#define FM_MAX_NUM_OF_PCD_PORTS     FM_MAX_NUM_OF_RX_PORTS+FM_MAX_NUM_OF_OP_PORTS

#define FM_MURAM_SIZE               (160*KILOBYTE)
#define FM_PCD_PLCR_NUM_ENTRIES     256                 /**< Total number of policer profiles */
#define FM_PCD_KG_NUM_OF_SCHEMES    32                  /**< Total number of KG schemes */
#define FM_PCD_MAX_NUM_OF_CLS_PLANS 256                 /**< Number of classification plan entries. */

/* FM erratas */
#define FM_OP_PARTITION_ERRATA
#define CORE_8BIT_ACCESS_ERRATA
#define FM_ENET_SGMII_1000_ERRATA


#endif /* __PART_INTEGRATION_EXT_H */
