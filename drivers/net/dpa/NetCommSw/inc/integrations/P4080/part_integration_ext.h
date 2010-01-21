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

 @File          part_integration_ext.h

 @Description   P4080 external definitions and structures.
*//***************************************************************************/
#ifndef __PART_INTEGRATION_EXT_H
#define __PART_INTEGRATION_EXT_H

#include "std_ext.h"
#include "dpaa_integration_ext.h"
/**************************************************************************//**
 @Group         P4080_chip_id P4080 Application Programming Interface

 @Description   P4080 Chip functions,definitions and enums.

 @{
*//***************************************************************************/

#define CORE_E500MC

#define INTG_MAX_NUM_OF_CORES   8


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
    e_MODULE_ID_QM_CE_PORTAL_0,
    e_MODULE_ID_QM_CE_PORTAL_1,
    e_MODULE_ID_QM_CE_PORTAL_2,
    e_MODULE_ID_QM_CE_PORTAL_3,
    e_MODULE_ID_QM_CE_PORTAL_4,
    e_MODULE_ID_QM_CE_PORTAL_5,
    e_MODULE_ID_QM_CE_PORTAL_6,
    e_MODULE_ID_QM_CE_PORTAL_7,
    e_MODULE_ID_QM_CE_PORTAL_8,
    e_MODULE_ID_QM_CI_PORTAL_0,
    e_MODULE_ID_QM_CI_PORTAL_1,
    e_MODULE_ID_QM_CI_PORTAL_2,
    e_MODULE_ID_QM_CI_PORTAL_3,
    e_MODULE_ID_QM_CI_PORTAL_4,
    e_MODULE_ID_QM_CI_PORTAL_5,
    e_MODULE_ID_QM_CI_PORTAL_6,
    e_MODULE_ID_QM_CI_PORTAL_7,
    e_MODULE_ID_QM_CI_PORTAL_8,
    e_MODULE_ID_BM_CE_PORTAL_0,
    e_MODULE_ID_BM_CE_PORTAL_1,
    e_MODULE_ID_BM_CE_PORTAL_2,
    e_MODULE_ID_BM_CE_PORTAL_3,
    e_MODULE_ID_BM_CE_PORTAL_4,
    e_MODULE_ID_BM_CE_PORTAL_5,
    e_MODULE_ID_BM_CE_PORTAL_6,
    e_MODULE_ID_BM_CE_PORTAL_7,
    e_MODULE_ID_BM_CE_PORTAL_8,
    e_MODULE_ID_BM_CI_PORTAL_0,
    e_MODULE_ID_BM_CI_PORTAL_1,
    e_MODULE_ID_BM_CI_PORTAL_2,
    e_MODULE_ID_BM_CI_PORTAL_3,
    e_MODULE_ID_BM_CI_PORTAL_4,
    e_MODULE_ID_BM_CI_PORTAL_5,
    e_MODULE_ID_BM_CI_PORTAL_6,
    e_MODULE_ID_BM_CI_PORTAL_7,
    e_MODULE_ID_BM_CI_PORTAL_8,
    e_MODULE_ID_FM1,                /**< Frame manager #1 module */
    e_MODULE_ID_FM1_RTC,            /**< FM Real-Time-Clock */
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
    e_MODULE_ID_FM2_RTC,            /**< FM Real-Time-Clock */
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
    e_MODULE_ID_GPIO,               /**< GPIO */
    e_MODULE_ID_SERDES,             /**< SERDES */
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
    e_P4080E_REV_1_0      = 0x82080010,   /**< P4080E with security, revision 1.0 */
    e_P4080E_REV_2_0      = 0x82080020    /**< P4080E with security, revision 2.0 */
} e_P4080DeviceName;

/**************************************************************************//**
 @Description   structure representing P4080 initialization parameters
*//***************************************************************************/
typedef struct
{
    uint64_t        ccsrBaseAddress;        /**< CCSR base address (virtual) */
    uint64_t        bmPortalsBaseAddress;   /**< Portals base address (virtual) */
    uint64_t        qmPortalsBaseAddress;   /**< Portals base address (virtual) */
    bool            (*f_BoardIsValidSerDesConfigurationCB) (uint8_t val);
} t_P4080Params;

/**************************************************************************//**
 @Description   SERDES lanes activation
*//***************************************************************************/
typedef enum e_SerDesLaneActivation
{
    e_SERDES_LANE_F,                /**<  */
    e_SERDES_LANE_H,                /**<  */
    e_SERDES_LANE_A_B,              /**<  */
    e_SERDES_LANE_C_D,              /**<  */
    e_SERDES_LANE_E_F,              /**<  */
    e_SERDES_LANE_G_H,              /**<  */
    e_SERDES_LANE_I_J,              /**<  */
    e_SERDES_LANE_A_B_C_D,          /**<  */
    e_SERDES_LANE_E_F_G_H,          /**<  */
    e_SERDES_LANE_A_B_C_D_E_F_G_H,  /**<  */
} e_SerDesLaneActivation;

/**************************************************************************//**
 @Description   structure representing P4080 devices configuration
*//***************************************************************************/
typedef struct
{
    struct
    {
        struct
        {
            bool                    en;
            uint8_t                 serdesBank;
            e_SerDesLaneActivation  serdesLane;
            bool                    sgmii;
        } dtsecs[4];
        struct
        {
            bool                    en;
            uint8_t                 serdesBank;
            e_SerDesLaneActivation  serdesLane;
        } tgec;
    } fms[2];
} t_P4080DevParams;

/**************************************************************************//**
 @Function      P4080_ConfigAndInit

 @Description   General initiation of the chip registers.

 @Param[in]     p_P4080Params  - A pointer to data structure of parameters

 @Return        A handle to the P4080 data structure.
*//***************************************************************************/
t_Handle P4080_ConfigAndInit(t_P4080Params *p_P4080Params);

/**************************************************************************//**
 @Function      P4080_Free

 @Description   Free all resources.

 @Param[in]     h_P4080 - The handle of the initialized P4080 object.

 @Return        E_OK on success; Other value otherwise.
*//***************************************************************************/
t_Error P4080_Free(t_Handle h_P4080);

/**************************************************************************//**
 @Function      P4080_GetModulePhysBase

 @Description   returns the base address of a P4080 module's
                memory mapped registers.

 @Param[in]     h_P4080   - The handle of the initialized P4080 object.
 @Param[in]     module    - The module ID.

 @Return        Base address of module's memory mapped registers.
                ILLEGAL_BASE in case of non-existent module
*//***************************************************************************/
uint64_t P4080_GetModulePhysBase(t_Handle h_P4080, e_ModuleId module);

/**************************************************************************//**
 @Function      P4080_GetRevInfo

 @Description   This routine enables access to chip and revision information.

 @Param[in]     h_P4080 - The handle of the initialized P4080 object.

 @Return        Part ID and revision.
*//***************************************************************************/
e_P4080DeviceName P4080_GetRevInfo(t_Handle h_P4080);

/**************************************************************************//**
 @Function      P4080_GetE500Factor

 @Description   returns system multiplication factor.

 @Param[in]     h_P4080         - A handle to the P4080 object.
 @Param[in]     coreIndex       - core index.
 @Param[out]    p_E500MulFactor - returns E500 to CCB multification factor.
 @Param[out]    p_E500DivFactor - returns E500 to CCB division factor.

 @Return        E_OK on success; Other value otherwise.
*
*//***************************************************************************/
t_Error P4080_GetE500Factor(t_Handle h_P4080, uint8_t coreIndex, uint32_t *p_E500MulFactor, uint32_t *p_E500DivFactor);

/**************************************************************************//**
 @Function      P4080_GetCcbFactor

 @Description   returns system multiplication factor.

 @Param[in]     h_P4080 - The handle of the initialized P4080 object.

 @Return        System multiplication factor.
*//***************************************************************************/
uint32_t P4080_GetCcbFactor(t_Handle h_P4080);

/**************************************************************************//**
 @Function      P4080_GetDdrFactor

 @Description   returns the multiplication factor of the clock in for the DDR clock.

 @Param[in]     h_P080 - a handle to the P4080 object.

 @Return        System multiplication factor.
*//***************************************************************************/
uint32_t P4080_GetDdrFactor(t_Handle h_P4080);

/**************************************************************************//**
 @Function      P4080_GetDdrType

 @Description   returns the multiplication factor of the clock in for the DDR clock .

 @Param[in]     h_P080 - a handle to the P4080 object.
 @Param[out]    p_DdrType - returns DDR type DDR2/DDR3.

 @Return        E_OK on success; Other value otherwise.
*//***************************************************************************/
//t_Error P4080_GetDdrType(t_Handle h_P4080, e_DdrType *p_DdrType);

/**************************************************************************//**
 @Function      P4080_GetFmFactor

 @Description   returns FM multiplication factors. (This value is returned using
                two parameters to avoid using float parameter).

 @Param[in]     h_P4080         - The handle of the initialized P4080 object.
 @Param[in]     fmIndex         - fm index.
 @Param[out]    p_FmMulFactor   - returns E500 to CCB multification factor.
 @Param[out]    p_FmDivFactor   - returns E500 to CCB division factor.

 @Return        E_OK on success; Other value otherwise.
*//***************************************************************************/
t_Error  P4080_GetFmFactor(t_Handle h_P4080, uint8_t fmIndex, uint32_t *p_FmMulFactor, uint32_t *p_FmDivFactor);

/**************************************************************************//**
 @Function      P4080_Reset

 @Description   Reset the chip.

 @Param[in]     h_P4080 - The handle of the initialized P4080 object.
*//***************************************************************************/
void P4080_Reset(t_Handle h_P4080);

t_Error P4080_CoreTimeBaseEnable(t_Handle h_P4080);
t_Error P4080_CoreTimeBaseDisable(t_Handle h_P4080);

t_P4080DevParams * P4080_GetDevicesConfiguration(t_Handle h_P4080);
t_Error P4080_DeviceEnable(t_Handle h_P4080,e_ModuleId module, bool enable);

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
#define MODULE_DUART            0x00090000
#define MODULE_SERDES           0x000a0000
#define MODULE_PIO              0x000b0000
#define MODULE_QM               0x000c0000
#define MODULE_BM               0x000d0000
#define MODULE_FM               0x000e0000
#define MODULE_FM_MURAM         0x000f0000
#define MODULE_FM_PCD           0x00100000
#define MODULE_FM_RTC           0x00110000
#define MODULE_FM_MAC           0x00120000
#define MODULE_FM_PORT          0x00130000

/*****************************************************************************
 LA INTEGRATION-SPECIFIC DEFINITIONS
******************************************************************************/
#define LA_NUM_OF_WINDOWS       32                      /**< Number of local access windows */

/**************************************************************************//**
 @Description   Local Access Window Target interface ID
*//***************************************************************************/
typedef enum e_LaTgtId
{
    e_LA_ID_PCIE1   = 0x00,  /**< PCI Express 1 target interface ID */
    e_LA_ID_PCIE2   = 0x01,  /**< PCI Express 2 target interface ID */
    e_LA_ID_PCIE3   = 0x02,  /**< PCI Express 3 target interface ID */
    e_LA_ID_RIO1    = 0x08,  /**< RapidIO 1 target interface ID */
    e_LA_ID_RIO2    = 0x09,  /**< RapidIO 2 target interface ID */
    e_LA_ID_LS      = 0x0F,  /**< Local Space target interface ID */
    e_LA_ID_MC1     = 0x10,  /**< DDR controller 1 or CPC1 SRAM target interface ID */
    e_LA_ID_MC2     = 0x11,  /**< DDR controller 2 or CPC2 SRAM target interface ID */
    e_LA_ID_IM      = 0x14,  /**< Interleaved DDR controllers or CPC SRAM target interface ID */
    e_LA_ID_BMAN    = 0x18,  /**< BMAN target interface ID */
    e_LA_ID_DCSR    = 0x1D,  /**< DCSR target interface ID */
    e_LA_ID_LBC     = 0x1F,  /**< Local Bus target interface ID */
    e_LA_ID_QMAN    = 0x3C,  /**< QMAN target interface ID */
    e_LA_ID_NONE    = 0xFF  /**< None */
} e_LaTgtId;

/*****************************************************************************
 GPIO INTEGRATION-SPECIFIC DEFINITIONS
******************************************************************************/
#define GPIO_NUM_OF_PORTS   1   /**< Number of ports in GPIO module;
                                     Each port contains up to 32 i/O pins. */

#define GPIO_VALID_PIN_MASKS  \
    { /* Port A */ 0xFFFFFFFF }

#define GPIO_VALID_INTR_MASKS \
    { /* Port A */ 0xFFFFFFFF }


#ifdef SIMULATOR
#define CORE_8BIT_ACCESS_ERRATA
#endif /* SIMULATOR */

/*****************************************************************************
 MPIC INTEGRATION-SPECIFIC DEFINITIONS
******************************************************************************/
#define EPIC_MODE_MPIC

#define EPIC_NUM_OF_EXT_INTRS           12
#define EPIC_NUM_OF_INT_INTRS           112
#define EPIC_NUM_OF_TIMERS              8
#define EPIC_NUM_OF_MSG_INTRS           8
#define EPIC_NUM_OF_SMSG_INTRS          24

/*****************************************************************************
 SerDes INTEGRATION-SPECIFIC DEFINITIONS
******************************************************************************/
#ifndef SIMULATOR
#define SERDES_ERRATA
#endif /* !SIMULATOR */

#endif /* __PART_INTEGRATION_EXT_H */
