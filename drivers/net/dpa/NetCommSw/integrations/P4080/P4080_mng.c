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

#include "string_ext.h"
#include "error_ext.h"
#include "std_ext.h"
#include "xx_ext.h"

#include "P4080.h"


/*****************************************************************************/
t_Error P4080_MngInit(t_P4080 *p_P4080, uint32_t baseAddress)
{
    uint32_t tmpBaseAddr = baseAddress;

    /* Initialize base addresses to ILLEGAL_BASE */
    memset(p_P4080->baseAddresses, (~0), NUM_OF_MODULES * sizeof(uint32_t));

    /* init base addresses for all part's modules */
    p_P4080->baseAddresses[e_MODULE_ID_DUART_1]   = tmpBaseAddr + DUART0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_DUART_2]   = tmpBaseAddr + DUART1_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_DUART_3]   = tmpBaseAddr + DUART2_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_DUART_4]   = tmpBaseAddr + DUART3_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_QM]        = tmpBaseAddr + QM_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_BM]        = tmpBaseAddr + BM_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_MPIC]      = tmpBaseAddr + MPIC_OFFSET;

    tmpBaseAddr = baseAddress + FM1_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1]               = tmpBaseAddr;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_MURAM]         = tmpBaseAddr + FM_MURAM_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_BMI]           = tmpBaseAddr + FM_BMI_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_QMI]           = tmpBaseAddr + FM_QMI_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PRS]           = tmpBaseAddr + FM_PRS_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_HO0]      = tmpBaseAddr + FM_PORT_HO0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_HO1]      = tmpBaseAddr + FM_PORT_HO1_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_HO2]      = tmpBaseAddr + FM_PORT_HO2_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_HO3]      = tmpBaseAddr + FM_PORT_HO3_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_HO4]      = tmpBaseAddr + FM_PORT_HO4_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_HO5]      = tmpBaseAddr + FM_PORT_HO5_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_HO6]      = tmpBaseAddr + FM_PORT_HO6_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_1GRx0]    = tmpBaseAddr + FM_PORT_1GRX0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_1GRx1]    = tmpBaseAddr + FM_PORT_1GRX1_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_1GRx2]    = tmpBaseAddr + FM_PORT_1GRX2_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_1GRx3]    = tmpBaseAddr + FM_PORT_1GRX3_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_10GRx0]   = tmpBaseAddr + FM_PORT_10GRX0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_1GTx0]    = tmpBaseAddr + FM_PORT_1GTX0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_1GTx1]    = tmpBaseAddr + FM_PORT_1GTX1_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_1GTx2]    = tmpBaseAddr + FM_PORT_1GTX2_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_1GTx3]    = tmpBaseAddr + FM_PORT_1GTX3_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PORT_10GTx0]   = tmpBaseAddr + FM_PORT_10GTX0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PLCR]          = tmpBaseAddr + FM_PLCR_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_KG]            = tmpBaseAddr + FM_KG_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_DMA]           = tmpBaseAddr + FM_DMA_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_FPM]           = tmpBaseAddr + FM_FPM_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_IRAM]          = tmpBaseAddr + FM_IRAM_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_1GMDIO0]       = tmpBaseAddr + FM_1GMDIO0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_1GMDIO1]       = tmpBaseAddr + FM_1GMDIO1_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_1GMDIO2]       = tmpBaseAddr + FM_1GMDIO2_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_1GMDIO3]       = tmpBaseAddr + FM_1GMDIO3_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_10GMDIO]       = tmpBaseAddr + FM_10GMDIO_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_PRS_IRAM]      = tmpBaseAddr + FM_PRS_IRAM_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_RISC0]         = tmpBaseAddr + FM_RISC0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_RISC1]         = tmpBaseAddr + FM_RISC1_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_1GMAC0]        = tmpBaseAddr + FM_1GMAC0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_1GMAC1]        = tmpBaseAddr + FM_1GMAC1_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_1GMAC2]        = tmpBaseAddr + FM_1GMAC2_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_1GMAC3]        = tmpBaseAddr + FM_1GMAC3_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM1_10GMAC0]       = tmpBaseAddr + FM_10GMAC0_OFFSET;

    tmpBaseAddr = baseAddress + FM2_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2]               = tmpBaseAddr;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_MURAM]         = tmpBaseAddr + FM_MURAM_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_BMI]           = tmpBaseAddr + FM_BMI_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_QMI]           = tmpBaseAddr + FM_QMI_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PRS]           = tmpBaseAddr + FM_PRS_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_HO0]      = tmpBaseAddr + FM_PORT_HO0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_HO1]      = tmpBaseAddr + FM_PORT_HO1_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_HO2]      = tmpBaseAddr + FM_PORT_HO2_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_HO3]      = tmpBaseAddr + FM_PORT_HO3_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_HO4]      = tmpBaseAddr + FM_PORT_HO4_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_HO5]      = tmpBaseAddr + FM_PORT_HO5_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_HO6]      = tmpBaseAddr + FM_PORT_HO6_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_1GRx0]    = tmpBaseAddr + FM_PORT_1GRX0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_1GRx1]    = tmpBaseAddr + FM_PORT_1GRX1_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_1GRx2]    = tmpBaseAddr + FM_PORT_1GRX2_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_1GRx3]    = tmpBaseAddr + FM_PORT_1GRX3_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_10GRx0]   = tmpBaseAddr + FM_PORT_10GRX0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_1GTx0]    = tmpBaseAddr + FM_PORT_1GTX0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_1GTx1]    = tmpBaseAddr + FM_PORT_1GTX1_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_1GTx2]    = tmpBaseAddr + FM_PORT_1GTX2_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_1GTx3]    = tmpBaseAddr + FM_PORT_1GTX3_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PORT_10GTx0]   = tmpBaseAddr + FM_PORT_10GTX0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PLCR]          = tmpBaseAddr + FM_PLCR_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_KG]            = tmpBaseAddr + FM_KG_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_DMA]           = tmpBaseAddr + FM_DMA_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_FPM]           = tmpBaseAddr + FM_FPM_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_IRAM]          = tmpBaseAddr + FM_IRAM_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_1GMDIO0]       = tmpBaseAddr + FM_1GMDIO0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_1GMDIO1]       = tmpBaseAddr + FM_1GMDIO1_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_1GMDIO2]       = tmpBaseAddr + FM_1GMDIO2_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_1GMDIO3]       = tmpBaseAddr + FM_1GMDIO3_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_10GMDIO]       = tmpBaseAddr + FM_10GMDIO_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_PRS_IRAM]      = tmpBaseAddr + FM_PRS_IRAM_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_RISC0]         = tmpBaseAddr + FM_RISC0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_RISC1]         = tmpBaseAddr + FM_RISC1_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_1GMAC0]        = tmpBaseAddr + FM_1GMAC0_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_1GMAC1]        = tmpBaseAddr + FM_1GMAC1_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_1GMAC2]        = tmpBaseAddr + FM_1GMAC2_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_1GMAC3]        = tmpBaseAddr + FM_1GMAC3_OFFSET;
    p_P4080->baseAddresses[e_MODULE_ID_FM2_10GMAC0]       = tmpBaseAddr + FM_10GMAC0_OFFSET;

    return E_OK;
}

/*****************************************************************************/
t_Error P4080_MngFree(t_P4080 *p_P4080)
{
    UNUSED(p_P4080);
    return E_OK;
}
