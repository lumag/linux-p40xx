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

 /**************************************************************************//**

 @File          P4080.h

 @Description   P4080 object data structure declaration, definitions and internal prototypes.
*//***************************************************************************/

#ifndef __P4080_H
#define __P4080_H

#include "std_ext.h"
#include "part_ext.h"


#define __ERR_MODULE__  MODULE_P4080

#ifdef __MWERKS__
#pragma pack(push,1)
#endif /* __MWERKS__ */
#define MEM_MAP_START

typedef _Packed struct
{
    /* system configuration module */
    volatile uint32_t tbd;    /**< tbd */
} _PackedType t_P4080SysCfgMemMap;

#define MEM_MAP_END
#ifdef __MWERKS__
#pragma pack(pop)
#endif /* __MWERKS__ */


/* Offsets relative to larger memory map, with base IMMRBAR */
#define DUART0_OFFSET           0x0011c500
#define DUART1_OFFSET           0x0011c600
#define DUART2_OFFSET           0x0011d500
#define DUART3_OFFSET           0x0011d600
#define QM_OFFSET               0x00318000
#define BM_OFFSET               0x0031a000
#define FM1_OFFSET              0x00400000
#define FM2_OFFSET              0x00500000
#define MPIC_OFFSET             0x00040000

/* Offsets relative to FM_BASE_OFFSET off of base IMMRBAR */
#define FM_MURAM_OFFSET         0x00000000
#define FM_BMI_OFFSET           0x00080000
#define FM_QMI_OFFSET           0x00080400
#define FM_PRS_OFFSET           0x00080800
#define FM_PORT_HO0_OFFSET      0x00081000
#define FM_PORT_HO1_OFFSET      0x00082000
#define FM_PORT_HO2_OFFSET      0x00083000
#define FM_PORT_HO3_OFFSET      0x00084000
#define FM_PORT_HO4_OFFSET      0x00085000
#define FM_PORT_HO5_OFFSET      0x00086000
#define FM_PORT_HO6_OFFSET      0x00087000
#define FM_PORT_1GRX0_OFFSET    0x00088000
#define FM_PORT_1GRX1_OFFSET    0x00089000
#define FM_PORT_1GRX2_OFFSET    0x0008a000
#define FM_PORT_1GRX3_OFFSET    0x0008b000
#define FM_PORT_10GRX0_OFFSET   0x00090000
#define FM_PORT_1GTX0_OFFSET    0x000a8000
#define FM_PORT_1GTX1_OFFSET    0x000a9000
#define FM_PORT_1GTX2_OFFSET    0x000aa000
#define FM_PORT_1GTX3_OFFSET    0x000ab000
#define FM_PORT_10GTX0_OFFSET   0x000b0000
#define FM_PLCR_OFFSET          0x000c0000
#define FM_KG_OFFSET            0x000c1000
#define FM_DMA_OFFSET           0x000c2000
#define FM_FPM_OFFSET           0x000c3000
#define FM_IRAM_OFFSET          0x000c4000
#define FM_PRS_IRAM_OFFSET      0x000c7000
#define FM_RISC0_OFFSET         0x000d0000
#define FM_RISC1_OFFSET         0x000d1000
#define FM_1GMAC0_OFFSET        0x000e0000
#define FM_1GMDIO0_OFFSET       0x000e1000
#define FM_1GMAC1_OFFSET        0x000e2000
#define FM_1GMDIO1_OFFSET       0x000e3000
#define FM_1GMAC2_OFFSET        0x000e4000
#define FM_1GMDIO2_OFFSET       0x000e5000
#define FM_1GMAC3_OFFSET        0x000e6000
#define FM_1GMDIO3_OFFSET       0x000e7000
#define FM_10GMAC0_OFFSET       0x000f0000
#define FM_10GMDIO_OFFSET       0x000f1000

//#define FM_MURAM_SIZE 0x40000

#define BM_PORTALS_CE_OFFSET    0x000000
#define BM_PORTALS_CI_OFFSET    0x100000
#define QM_PORTALS_CE_OFFSET    0x200000
#define QM_PORTALS_CI_OFFSET    0x300000

#define QM_PORTALS_OFFSET_CE(portal)    (0x4000 * portal)
#define QM_PORTALS_OFFSET_CI(portal)    (0x1000 * portal)
#define BM_PORTALS_OFFSET_CE(portal)    (0x4000 * portal)
#define BM_PORTALS_OFFSET_CI(portal)    (0x1000 * portal)


/*--------------------------------------*/
/* Structure for the P4080 object.    */
/*--------------------------------------*/
typedef struct
{
    t_Part                  part;               /**< Common parameters for all parts */
    uint32_t                baseAddresses[NUM_OF_MODULES];
                                                /**< Modules offsets in memory map */
} t_P4080;


/**************************************************************************//**
 @Function      P4080_MngInit

 @Description   Initializes the P4080 module's managment unit.

 @Param         p_P4080   - (in) Pointer to the P4080 control structure.
 @Param         baseAddress - (in) Base address of the memory-map.

 @Return        E_OK on success, other value otherwise.
*//***************************************************************************/
t_Error P4080_MngInit(t_P4080 *p_P4080, uint32_t baseAddress);

/**************************************************************************//**
 @Function      P4080_MngFree

 @Description   Free the P4080 module's managment unit.

 @Param         p_P4080 - (in) Pointer to the P4080 control structure.

 @Return        E_OK on success, other value otherwise.
*//***************************************************************************/
t_Error P4080_MngFree(t_P4080 *p_P4080);


#endif /* __P4080_H */
