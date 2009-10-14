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
 @File          fm_pcd_ipc.h

 @Description   FM PCD Inter-Partition prototypes, structures and definitions.
*//***************************************************************************/
#ifndef __FM_PCD_IPC_H
#define __FM_PCD_IPC_H

#include "std_ext.h"


/**************************************************************************//**
 @Group         FM_grp Frame Manager API

 @Description   FM API functions, definitions and enums

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Group         FM_PCD_IPC_grp FM PCD Inter-Partition messaging Unit

 @Description   FM PCD Inter-Partition messaging unit API definitions and enums.

 @{
*//***************************************************************************/
#define FM_PCD_PLCR_NUM_ENTRIES                     256
#define FM_PCD_IPC_MAX_NUM_OF_DISTINCTION_UNITS     32
#define FM_PCD_IPC_KG_NUM_OF_SCHEMES                32

/**************************************************************************//**
 @Collection    General PCD defines
*//***************************************************************************/
typedef uint32_t fmPcdIpcEngines_t; /**< options as defined below: */

#define FM_PCD_IPC_NONE                 0                   /**< No PCD Engine indicated */
#define FM_PCD_IPC_PRS                  0x80000000          /**< Parser indicated */
#define FM_PCD_IPC_KG                   0x40000000          /**< Keygen indicated */
#define FM_PCD_IPC_CC                   0x20000000          /**< Coarse classification indicated */
#define FM_PCD_IPC_PLCR                 0x10000000          /**< Policer indicated */
/* @} */


/**************************************************************************//**
 @Description   Structure for getting a sw parser address according to a label
                Fields commented 'IN' are passed by the port module to be used
                by the FM module.
                Fields commented 'OUT' will be filled by FM before returning to port.
*//***************************************************************************/
typedef struct
{
    e_NetHeaderType         hdr;                            /**< IN. The existance of this header will envoke
                                                                 the sw parser code. */
    uint8_t                 indexPerHdr;                    /**< IN. Normally 0, if more than one sw parser
                                                                 attachments for the same header, use this
                                                                 index to distinguish between them. */
    uint32_t                offset;                         /**< OUT. MURAM offset for the labeled code. */
}t_FmPcdIpcSwPrsLable;


/**************************************************************************//**
 @Description   Structure for port-PCD communication.
                Fields commented 'IN' are passed by the port module to be used
                by the FM module.
                Fields commented 'OUT' will be filled by FM before returning to port.
                Some fields are optional (depending on configuration) and
                will be analized by the port and FM modules accordingly.
*//***************************************************************************/
typedef struct
{
    uint8_t     partitionId;                                /**< IN */
    uint8_t     numOfSchemes;                               /**< IN */
    uint8_t     schemesIds[FM_PCD_IPC_KG_NUM_OF_SCHEMES];   /**< OUT */
    uint16_t    numOfClsPlanEntries;                        /**< IN */
    uint8_t     clsPlanBase;                                /**< OUT */
    bool        isDriverClsPlanGrp;                         /**< IN */
} t_FmPcdIpcKgAllocParams;

typedef struct
{
    uint16_t num;
    uint8_t  hardwarePortId;
    uint16_t plcrProfilesBase;
} t_FmPcdIpcPlcrAllocParams;

typedef struct
{
    uint16_t    num;                                    /**< IN */
    uint16_t    profilesIds[FM_PCD_PLCR_NUM_ENTRIES];   /**< OUT */
} t_FmPcdIpcSharedPlcrAllocParams;

#ifdef __MWERKS__
#pragma pack(push,1)
#endif /*__MWERKS__ */
#define MEM_MAP_START

typedef _Packed struct t_FmPcdIcPhysAddr
{
    volatile uint16_t high;
    volatile uint32_t low;
} _PackedType t_FmPcdIcPhysAddr;

#define MEM_MAP_END
#ifdef __MWERKS__
#pragma pack(pop)
#endif /* __MWERKS__ */

/**************************************************************************//**
 @Function      FM_PCD_GET_SET_PORT_PARAMS

 @Description   Used by FM PORT module in order to check whether
                an FM port is stalled.

 @Param[in/out] t_FmPcdIcPortInitParams

*//***************************************************************************/
#define FM_PCD_GET_SET_PORT_PARAMS              20

/**************************************************************************//**
 @Function      FM_PCD_CLEAR_PORT_PARAMS

 @Description   Used by FM PORT module in order to free port's PCD resources

 @Param[in/out] t_FmPcdIcPortInitParams

*//***************************************************************************/
#define FM_PCD_CLEAR_PORT_PARAMS                21

#define FM_PCD_ALLOC_KG_RSRC                    22
#define FM_PCD_FREE_KG_RSRC                     23
#define FM_PCD_ALLOC_PROFILES                   24
#define FM_PCD_FREE_PROFILES                    25
#define FM_PCD_GET_PHYS_MURAM_BASE              26

/**************************************************************************//**
 @Function      FM_PCD_GET_SW_PRS_OFFSET

 @Description   Used by FM PORT module to get the SW parser offset of the start of
                code relevant to a given label..

 @Param[in/out] t_FmPcdIcSwPrsLable

*//***************************************************************************/
#define FM_PCD_GET_SW_PRS_OFFSET                27

#define FM_PCD_ALLOC_SHARED_PROFILES            28
#define FM_PCD_FREE_SHARED_PROFILES             29

/**************************************************************************//**
 @Function      FM_PCD_GET_SET_KG_SCHEME_HC_PARAMS

 @Description   Used by FM HC module to get and set keygen scheme parameters

 @Param[in/out] t_FmPcdIcKgSchemeParams

*//***************************************************************************/
#define FM_PCD_GET_SET_KG_SCHEME_HC_PARAMS      30

/**************************************************************************//**
 @Function      FM_PCD_FREE_KG_SCHEME_HC

 @Description   Used by FM HC  module in order to update owners counter
                if PCD Network environment.

 @Param[in]     scheme id

*//***************************************************************************/
#define FM_PCD_FREE_KG_SCHEME_HC                31

#define FM_PCD_MASTER_IS_ALIVE                  32

#define FM_PCD_FREE_PLCR_PROFILE_HC             33

#define FM_PCD_ALLOC_CLS_PLAN_EMPTY_GRP         34

#define FM_PCD_MASTER_IS_ENABLED                35

/** @} */ /* end of FM_PCD_IPC_grp group */
/** @} */ /* end of FM_grp group */


#endif /* __FM_PCD_IPC_H */
