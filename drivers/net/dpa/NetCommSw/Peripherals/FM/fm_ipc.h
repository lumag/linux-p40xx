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
 @File          fm_ipc.h

 @Description   FM Inter-Partition prototypes, structures and definitions.
*//***************************************************************************/
#ifndef __FM_IPC_H
#define __FM_IPC_H

#include "error_ext.h"
#include "std_ext.h"


/**************************************************************************//**
 @Group         FM_grp Frame Manager API

 @Description   FM API functions, definitions and enums

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Group         FM_IPC_grp FM Inter-Partition messaging Unit

 @Description   FM Inter-Partition messaging unit API definitions and enums.

 @{
*//***************************************************************************/

#define FM_IC_PHYS_ADDRESS_SIZE    6

/**************************************************************************//**
 @Description   FM physical Address
*//***************************************************************************/
//typedef uint8_t fmIpcPhysAddr_t[FM_IC_PHYS_ADDRESS_SIZE];

/**************************************************************************//**
 @Description   Structure for port-FM communication during FM_PORT_Init.
                Fields commented 'IN' are passed by the port module to be used
                by the FM module.
                Fields commented 'OUT' will be filled by FM before returning to port.
                Some fields are optional (depending on configuration) and
                will be analized by the port and FM modules accordingly.
*//***************************************************************************/
//typedef struct
//{
//    uint8_t             hardwarePortId;       /**< IN. port Id */
//    e_FmPortIcType      portType;           /**< IN. Port type */
//    uint32_t            timeStampPeriod;    /**< OUT. Time stamp period in NanoSec */
//    bool                independentMode;    /**< IN. TRUE if FM Port operates in independent mode */
//    uint8_t             portPartition;      /**< IN. Port's requested resource */
//    uint8_t             numOfTasks;         /**< IN. Port's requested resource */
//    uint8_t             numOfExtraTasks;    /**< IN. Port's requested resource */
//    uint8_t             numOfOpenDmas;      /**< IN. Port's requested resource */
//    uint8_t             numOfExtraOpenDmas; /**< IN. Port's requested resource */
//    uint32_t            sizeOfFifo;         /**< IN. Port's requested resource */
//    uint32_t            extraSizeOfFifo;    /**< IN. Port's requested resource */
//    fmIcPhysAddr_t      fmMuramPhysBaseAddr;/**< OUT. FM-MURAM physical address*/
//} t_FmIcPortInitParams;

/**************************************************************************//**
 @Description   Structure for finding out whether a port is stalled.
                Fields commented 'IN' are passed by the port module to be used
                by the FM module.
                Fields commented 'OUT' will be filled by FM before returning to port.
*//***************************************************************************/
typedef struct
{
    uint8_t             hardwarePortId;       /**< IN. port Id */
    bool                isStalled;          /**< OUT. TRUE if FM PORT is stalled */
} t_FmIpcPortIsStalled;



/**************************************************************************//**
 @Description   enum for defining MAC types
*//***************************************************************************/
typedef enum
{
    e_FM_IPC_MAC_10G,
    e_FM_IPC_MAC_1G
} e_FmIpcMacType;

/**************************************************************************//**
 @Description   A structure of parameters for specifying a MAC.
*//***************************************************************************/
typedef struct
{
    uint8_t         id;
    e_FmIpcMacType   type;
} t_FmIpcMacReset;


/**************************************************************************//**
 @Function      FM_RESET_MAC

 @Description   Used by MAC module to reset the MAC registers

 @Param[in]     t_FmIpcMacReset  .

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
//#define FM_RESET_MAC                10

/**************************************************************************//**
 @Function      FM_GET_SET_PORT_PARAMS

 @Description   Used by FM PORT module in order to set and get parameters in/from
                FM module on FM PORT initialization time.

 @Param[in/out] t_FmIcPortInitParams

*//***************************************************************************/
//#define FM_GET_SET_PORT_PARAMS      11

/**************************************************************************//**
 @Function      FM_IS_PORT_STALLED

 @Description   Used by FM PORT module in order to check whether
                an FM port is stalled.

 @Param[in/out] t_FmIcPortIsStalled

*//***************************************************************************/
//#define FM_IS_PORT_STALLED          12

/**************************************************************************//**
 @Function      FM_RESUME_STALLED_PORT

 @Description   Used by FM PORT module in order to release a stalled
                FM Port.

 @Param[in]     hardwarePortId

*//***************************************************************************/
//#define FM_RESUME_STALLED_PORT     13

/**************************************************************************//**
 @Function      FM_DUMP_PORT_REGS

 @Description   Used by FM PORT module in order to dump all port registers
                that are a part of the FM module.

 @Param[in]     hardwarePortId

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
//#define FM_DUMP_PORT_REGS           14


/**************************************************************************//**
 @Function      FM_FREE_PORT

 @Description   Used by FM PORT module when a port is freed to free all FM resources.

 @Param[in]     hardwarePortId

 @Return        None.
*//***************************************************************************/
//#define FM_FREE_PORT                15



/*=============================*/
/**************************************************************************//**
 @Description   enum for defining port types
*//***************************************************************************/

/**************************************************************************//**
 @Description   enum for defining port types
*//***************************************************************************/
typedef enum e_FmIpcPortType {
    e_FM_IPC_PORT_TYPE_OFFLINE_PARSING, /**< Offline parsing port (id's: 0-6, share id's with
                                             host command, so must have exclusive id) */
    e_FM_IPC_PORT_TYPE_HOST_COMMAND,    /**< Host command port (id's: 0-6, share id's with
                                             offline parsing ports, so must have exclusive id) */
    e_FM_IPC_PORT_TYPE_RX,              /**< 1G Rx port (id's: 0-3) */
    e_FM_IPC_PORT_TYPE_RX_10G,          /**< 10G Rx port (id's: 0) */
    e_FM_IPC_PORT_TYPE_TX,              /**< 1G Tx port (id's: 0-3) */
    e_FM_IPC_PORT_TYPE_TX_10G,          /**< 10G Tx port (id's: 0) */
    e_FM_IPC_PORT_TYPE_DUMMY
} e_FmIpcPortType;

#ifdef __MWERKS__
#pragma pack(push,1)
#endif /*__MWERKS__ */
#define MEM_MAP_START

/**************************************************************************//**
 @Description   FM physical Address
*//***************************************************************************/
typedef _Packed struct t_FmIpcPhysAddr
{
    volatile uint16_t high;
    volatile uint32_t low;
}_PackedType t_FmIpcPhysAddr;

#define MEM_MAP_END
#ifdef __MWERKS__
#pragma pack(pop)
#endif /* __MWERKS__ */

#define FM_GET_TIMESTAMP            1
#define FM_GET_TIMESTAMP_PERIOD     2
#define FM_GET_COUNTER              3
#define FM_DUMP_REGS                4
#define FM_GET_SET_PORT_PARAMS      5
#define FM_FREE_PORT                6
#define FM_RESET_MAC                7
#define FM_RESUME_STALLED_PORT      8
#define FM_IS_PORT_STALLED          9
#define FM_DUMP_PORT_REGS           10
#define FM_GET_REV                  11
/**************************************************************************//**
 @Description   Structure for IPC communication during FM_PORT_Init.
                Fields commented 'IN' are passed by the port module to be used
                by the FM module.
                Fields commented 'OUT' will be filled by FM before returning to port.
                Some fields are optional (depending on configuration) and
                will be analized by the port and FM modules accordingly.
*//***************************************************************************/
typedef struct t_FmIpcPortInitParams {
    uint8_t             hardwarePortId;       /**< IN. port Id */
    e_FmIpcPortType     portType;           /**< IN. Port type */
    uint32_t            timeStampPeriod;    /**< OUT. Time stamp period in NanoSec */
    bool                independentMode;    /**< IN. TRUE if FM Port operates in independent mode */
    uint8_t             portPartition;      /**< IN. Port's requested resource */
    uint8_t             numOfTasks;         /**< IN. Port's requested resource */
    uint8_t             numOfExtraTasks;    /**< IN. Port's requested resource */
    uint8_t             numOfOpenDmas;      /**< IN. Port's requested resource */
    uint8_t             numOfExtraOpenDmas; /**< IN. Port's requested resource */
    uint32_t            sizeOfFifo;         /**< IN. Port's requested resource */
    uint32_t            extraSizeOfFifo;    /**< IN. Port's requested resource */
    uint8_t             deqPipelineDepth;   /**< IN. Port's requested resource */
    t_FmIpcPhysAddr     fmMuramPhysBaseAddr;/**< OUT. FM-MURAM physical address*/
} t_FmIpcPortInitParams;

/**************************************************************************//**
 @Description   Structure for port-FM communication during FM_PORT_Free.
*//***************************************************************************/
typedef struct t_FmIpcPortFreeParams {
    uint8_t             hardwarePortId;         /**< IN. port Id */
    e_FmIpcPortType     portType;               /**< IN. Port type */
    uint8_t             deqPipelineDepth;       /**< IN. Port's requested resource */
} t_FmIpcPortFreeParams;


/**************************************************************************//**
 @Description   enum for defining FM counters
*//***************************************************************************/
typedef enum e_FmIpcCounters {
    e_FM_IPC_COUNTERS_ENQ_TOTAL_FRAME,              /**< QMI total enqueued frames counter */
    e_FM_IPC_COUNTERS_DEQ_TOTAL_FRAME,              /**< QMI total dequeued frames counter */
    e_FM_IPC_COUNTERS_DEQ_0,                        /**< QMI 0 frames from QMan counter */
    e_FM_IPC_COUNTERS_DEQ_1,                        /**< QMI 1 frames from QMan counter */
    e_FM_IPC_COUNTERS_DEQ_2,                        /**< QMI 2 frames from QMan counter */
    e_FM_IPC_COUNTERS_DEQ_3,                        /**< QMI 3 frames from QMan counter */
    e_FM_IPC_COUNTERS_DEQ_FROM_DEFAULT,             /**< QMI dequeue from default queue counter */
    e_FM_IPC_COUNTERS_DEQ_FROM_CONTEXT,             /**< QMI dequeue from FQ context counter */
    e_FM_IPC_COUNTERS_DEQ_FROM_FD,                  /**< QMI dequeue from FD command field counter */
    e_FM_IPC_COUNTERS_DEQ_CONFIRM,                  /**< QMI dequeue confirm counter */
    e_FM_IPC_COUNTERS_SEMAPHOR_ENTRY_FULL_REJECT,   /**< DMA semaphor reject due to full entry counter */
    e_FM_IPC_COUNTERS_SEMAPHOR_QUEUE_FULL_REJECT,   /**< DMA semaphor reject due to full CAM queue counter */
    e_FM_IPC_COUNTERS_SEMAPHOR_SYNC_REJECT          /**< DMA semaphor reject due to sync counter */
} e_FmIpcCounters;

typedef struct t_FmIpcGetCounter
{
    e_FmIpcCounters id;         /* IN */
    uint32_t        val;        /* OUT */
} t_FmIpcGetCounter;

/** @} */ /* end of FM_IPC_grp group */
/** @} */ /* end of FM_grp group */


#endif /* __FM_IPC_H */
