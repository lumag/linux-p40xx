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

/**************************************************************************//**
 @File          FM_ext.h

 @Description   FM Application Programming Interface.
*//***************************************************************************/
#ifndef __FM_EXT
#define __FM_EXT

#include "error_ext.h"
#include "std_ext.h"


/**************************************************************************//**
 @Group         DPAA_grp Data Path Acceleration Architecture API

 @Description   DPAA API functions, definitions and enums.

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Group         FM_grp Frame Manager API

 @Description   FM API functions, definitions and enums.

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Group         FM_lib_grp FM library

 @Description   FM API functions, definitions and enums
                The FM module is the main driver module and is a mandatory module
                for FM driver users. Before any further module initialization,
                this module must be initialized.
                The FM is a "singletone" module. It is responsible of the common
                HW modules: FPM, DMA, common QMI, common BMI initializations and
                run-time control routines. This module must be initialized always
                when working with any of the FM modules.
                NOTE - We assumes that the FML will be initialize only by core No. 0!

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Description   enum for defining port types
*//***************************************************************************/
typedef enum e_FmPortType {
    e_FM_PORT_TYPE_OH_OFFLINE_PARSING,  /**< Offline parsing port (id's: 0-6, share id's with
                                             host command, so must have exclusive id) */
    e_FM_PORT_TYPE_OH_HOST_COMMAND,     /**< Host command port (id's: 0-6, share id's with
                                             offline parsing ports, so must have exclusive id) */
    e_FM_PORT_TYPE_RX,                  /**< 1G Rx port (id's: 0-3) */
    e_FM_PORT_TYPE_RX_10G,              /**< 10G Rx port (id's: 0) */
    e_FM_PORT_TYPE_TX,                  /**< 1G Tx port (id's: 0-3) */
    e_FM_PORT_TYPE_TX_10G,              /**< 10G Tx port (id's: 0) */
    e_FM_PORT_TYPE_DUMMY
} e_FmPortType;

/**************************************************************************//**
 @Collection    General FM defines
*//***************************************************************************/
#define FM_MAX_NUM_OF_PARTITIONS    64      /**< Maximim number of partitions */
#define FM_PHYS_ADDRESS_SIZE        6       /**< FM Physical address size */
/* @} */

/**************************************************************************//**
 @Collection   FM Frame error
*//***************************************************************************/
typedef uint32_t    fmPortFrameErrSelect_t;                         /**< typedef for defining Frame Descriptor errors */

#define FM_PORT_FRM_ERR_UNSUPPORTED_FORMAT              0x04000000  /**< Offline parsing only! Unsupported Format */
#define FM_PORT_FRM_ERR_LENGTH                          0x02000000  /**< Offline parsing only! Length Error */
#define FM_PORT_FRM_ERR_DMA                             0x01000000  /**< DMA Data error */
#define FM_PORT_FRM_ERR_PHYSICAL                        0x00080000  /**< Rx FIFO overflow, FCS error, code error, running disparity
                                                                         error (SGMII and TBI modes), FIFO parity error. PHY
                                                                         Sequence error, PHY error control character detected. */
#define FM_PORT_FRM_ERR_SIZE                            0x00040000  /**< Frame too long OR Frame size exceeds max_length_frame  */
#define FM_PORT_FRM_ERR_CLS_DISCARD                     0x00020000  /**< classification discard */
#define FM_PORT_FRM_ERR_EXTRACTION                      0x00008000  /**< Extract Out of Frame */
#define FM_PORT_FRM_ERR_NO_SCHEME                       0x00004000  /**< No Scheme Selected */
#define FM_PORT_FRM_ERR_KEYSIZE_OVERFLOW                0x00002000  /**< No Scheme Selected */
#define FM_PORT_FRM_ERR_COLOR_YELLOW                    0x00000400  /**< */
#define FM_PORT_FRM_ERR_COLOR_RED                       0x00000800  /**< */
#define FM_PORT_FRM_ERR_ILL_PLCR                        0x00000200  /**< Illegal Policer Profile selected */
#define FM_PORT_FRM_ERR_PLCR_FRAME_LEN                  0x00000100  /**< Illegal Policer Profile selected */
#define FM_PORT_FRM_ERR_PRS_TIMEOUT                     0x00000080  /**< Parser Time out Exceed */
#define FM_PORT_FRM_ERR_PRS_ILL_INSTRUCT                0x00000040  /**< Invalid Soft Parser instruction */
#define FM_PORT_FRM_ERR_PRS_HDR_ERR                     0x00000020  /**< Header error was identified during parsing */
#define FM_PORT_FRM_ERR_PROCESS_TIMEOUT                 0x00000001  /**< FPT Frame Processing Timeout Exceeded */
/* @} */

#if defined(__MWERKS__) && !defined(__GNUC__)
#pragma pack(push,1)
#endif /* defined(__MWERKS__) && ... */
#define MEM_MAP_START

/**************************************************************************//**
 @Description   FM physical Address
*//***************************************************************************/
typedef _Packed struct t_FmPhysAddr {
    volatile uint8_t  high;         /**< High part of the physical address */
    volatile uint32_t low;          /**< Low part of the physical address */
} _PackedType t_FmPhysAddr;

/**************************************************************************//**
 @Description   Parse results memory layout
*//***************************************************************************/
typedef _Packed struct t_FmPrsResult {
    uint8_t     lpid;               /**< Logical port id */
    uint8_t     shimr;              /**< Shim header result  */
    uint16_t    l2r;                /**< Layer 2 result */
    uint16_t    l3r;                /**< Layer 3 result */
    uint8_t     l4r;                /**< Layer 4 result */
    uint8_t     cplan;              /**< Classification plan id */
    uint16_t    nxthdr;             /**< Next Header  */
    uint16_t    cksum;              /**< Checksum */
    uint32_t    lcv;                /**< LCV */
    uint8_t     shim_off[3];        /**< Shim offset */
    uint8_t     eth_off;            /**< ETH offset */
    uint8_t     llc_snap_off;       /**< LLC_SNAP offset */
    uint8_t     vlan_off[2];        /**< VLAN offset */
    uint8_t     etype_off;          /**< ETYPE offset */
    uint8_t     pppoe_off;          /**< PPP offset */
    uint8_t     mpls_off[2];        /**< MPLS offset */
    uint8_t     ip_off[2];          /**< IP offset */
    uint8_t     gre_off;            /**< GRE offset */
    uint8_t     l4_off;             /**< Layer 4 offset */
    uint8_t     nxthdr_off;         /**< Parser end point */
} _PackedType t_FmPrsResult;

/**************************************************************************//**
 @Description   Parse results
*//***************************************************************************/
#define FM_PR_L2_VLAN_STACK       0x0100    /**< */
#define FM_PR_L2_ETHERNET         0x8000    /**< */
#define FM_PR_L2_VLAN             0x4000    /**< */
#define FM_PR_L2_LLC_SNAP         0x2000    /**< */
#define FM_PR_L2_MPLS             0x1000    /**< */
#define FM_PR_L2_PPPoE            0x0800    /**< */

/**************************************************************************//**
 @Description   Frame descriptor
*//***************************************************************************/
typedef _Packed struct t_FmFD {
    volatile uint32_t    id;                 /**< FD id */
    volatile uint32_t    addrl;              /**< Data Address */
    volatile uint32_t    length;             /**< Frame length */
    volatile uint32_t    status;             /**< FD status */
} _PackedType t_FmFD;

/**************************************************************************//**
 @Description   enum for defining frame format
*//***************************************************************************/
typedef enum e_FmFDFormatType {
    e_FM_FD_FORMAT_TYPE_SHORT_SBSF  = 0x0,   /**< Simple frame Single buffer; Offset and
                                                  small length (9b OFFSET, 20b LENGTH) */
    e_FM_FD_FORMAT_TYPE_LONG_SBSF   = 0x2,   /**< Simple frame, single buffer; big length
                                                  (29b LENGTH ,No OFFSET) */
    e_FM_FD_FORMAT_TYPE_SHORT_MBSF  = 0x4,   /**< Simple frame, Scatter Gather table; Offset
                                                  and small length (9b OFFSET, 20b LENGTH) */
    e_FM_FD_FORMAT_TYPE_LONG_MBSF   = 0x6,   /**< Simple frame, Scatter Gather table;
                                                  big length (29b LENGTH ,No OFFSET) */
    e_FM_FD_FORMAT_TYPE_COMPOUND    = 0x1,   /**< Compound Frame (29b CONGESTION-WEIGHT
                                                  No LENGTH or OFFSET) */
    e_FM_FD_FORMAT_TYPE_DUMMY
} e_FmFDFormatType;

/**************************************************************************//**
 @Collection   Frame descriptor macros
*//***************************************************************************/
#define FM_FD_DD_MASK       0xc0000000           /**< FD DD field mask */
#define FM_FD_PID_MASK      0x3f000000           /**< FD PID field mask */
#define FM_FD_ELIODN_MASK   0x0000f000           /**< FD ELIODN field mask */
#define FM_FD_BPID_MASK     0x00ff0000           /**< FD BPID field mask */
#define FM_FD_ADDRH_MASK    0x000000ff           /**< FD ADDRH field mask */
#define FM_FD_ADDRL_MASK    0xffffffff           /**< FD ADDRL field mask */
#define FM_FD_FORMAT_MASK   0xe0000000           /**< FD FORMAT field mask */
#define FM_FD_OFFSET_MASK   0x1ff00000           /**< FD OFFSET field mask */
#define FM_FD_LENGTH_MASK   0x000fffff           /**< FD LENGTH field mask */

#define FM_FD_GET_DD(fd)            ((((t_FmFD *)fd)->id & FM_FD_DD_MASK) >> (31-1))            /**< Macro to get FD DD field */
#define FM_FD_GET_PID(fd)           (((((t_FmFD *)fd)->id & FM_FD_PID_MASK) >> (31-7)) | \
                                        ((((t_FmFD *)fd)->id & FM_FD_LIODN_MASK) >> (31-19-6)))            /**< Macro to get FD PID field */
#define FM_FD_GET_BPID(fd)          ((((t_FmFD *)fd)->id & FM_FD_BPID_MASK) >> (31-15))         /**< Macro to get FD BPID field */
#define FM_FD_GET_ADDRH(fd)         (((t_FmFD *)fd)->id & FM_FD_ADDRH_MASK)                     /**< Macro to get FD ADDRH field */
#define FM_FD_GET_ADDRL(fd)         ((t_FmFD *)fd)->addrl                                       /**< Macro to get FD ADDRL field */
#define FM_FD_GET_PHYS_ADDR(fd)     ((physAddress_t)(((uint64_t)FM_FD_GET_ADDRH(fd) << 32) | (uint64_t)FM_FD_GET_ADDRL(fd))) /**< Macro to get FD ADDR field */
#define FM_FD_GET_FORMAT(fd)        ((((t_FmFD *)fd)->length & FM_FD_FORMAT_MASK) >> (31-2))    /**< Macro to get FD FORMAT field */
#define FM_FD_GET_OFFSET(fd)        ((((t_FmFD *)fd)->length & FM_FD_OFFSET_MASK) >> (31-11))   /**< Macro to get FD OFFSET field */
#define FM_FD_GET_LENGTH(fd)        (((t_FmFD *)fd)->length & FM_FD_LENGTH_MASK)                /**< Macro to get FD LENGTH field */
#define FM_FD_GET_STATUS(fd)        ((t_FmFD *)fd)->status                                      /**< Macro to get FD STATUS field */
#define FM_FD_GET_ADDR(fd)          XX_PhysToVirt(FM_FD_GET_PHYS_ADDR(fd))

#define FM_FD_SET_DD(fd,val)        (((t_FmFD *)fd)->id = ((((t_FmFD *)fd)->id & ~FM_FD_DD_MASK) | ((val << (31-1)) & FM_FD_DD_MASK )))      /**< Macro to set FD DD field */
/**< Macro to set FD PID field or LIODN offset*/
#define FM_FD_SET_PID(fd,val)       (((t_FmFD *)fd)->id = ((((t_FmFD *)fd)->id & ~(FM_FD_PID_MASK|FM_FD_ELIODN_MASK)) | (((val << (31-7)) & FM_FD_PID_MASK) | (((val>>6) << (31-19)) & FM_FD_ELIODN_MASK))))
#define FM_FD_SET_BPID(fd,val)      (((t_FmFD *)fd)->id = ((((t_FmFD *)fd)->id & ~FM_FD_BPID_MASK) | ((val  << (31-15)) & FM_FD_BPID_MASK))) /**< Macro to set FD BPID field */
#define FM_FD_SET_ADDRH(fd,val)     (((t_FmFD *)fd)->id = ((((t_FmFD *)fd)->id & ~FM_FD_ADDRH_MASK) | (val & FM_FD_ADDRH_MASK)))            /**< Macro to set FD ADDRH field */
#define FM_FD_SET_ADDRL(fd,val)     ((t_FmFD *)fd)->addrl = val                                 /**< Macro to set FD ADDRL field */
#define FM_FD_SET_ADDR(fd,val)                                      \
do {                                                                \
    uint64_t physAddr = (uint64_t)(XX_VirtToPhys(val)); \
    FM_FD_SET_ADDRH(fd, ((uint32_t)(physAddr >> 32)));              \
    FM_FD_SET_ADDRL(fd, (uint32_t)physAddr);                        \
} while (0)                                                                                     /**< Macro to set FD ADDR field */
#define FM_FD_SET_FORMAT(fd,val)    (((t_FmFD *)fd)->length = ((((t_FmFD *)fd)->length & ~FM_FD_FORMAT_MASK) | ((val  << (31-2))& FM_FD_FORMAT_MASK)))  /**< Macro to set FD FORMAT field */
#define FM_FD_SET_OFFSET(fd,val)    (((t_FmFD *)fd)->length = ((((t_FmFD *)fd)->length & ~FM_FD_OFFSET_MASK) | ((val << (31-11))& FM_FD_OFFSET_MASK) )) /**< Macro to set FD OFFSET field */
#define FM_FD_SET_LENGTH(fd,val)    (((t_FmFD *)fd)->length = (((t_FmFD *)fd)->length & ~FM_FD_LENGTH_MASK) | (val & FM_FD_LENGTH_MASK))                /**< Macro to set FD LENGTH field */
#define FM_FD_SET_STATUS(fd,val)    ((t_FmFD *)fd)->status = val                                /**< Macro to set FD STATUS field */

#define FM_FD_CMD_FCO  0x80000000      /* Frame queue Context Override */
#define FM_FD_CMD_RPD  0x40000000      /* Read Prepended Data */
#define FM_FD_CMD_UDP  0x20000000      /* Update Prepended Data */
#define FM_FD_CMD_DTC  0x10000000      /* Do TCP Checksum */

#define FM_FD_CMD_CFQ  0x00ffffff      /* Confirmation Frame Queue */

#define FM_FD_TX_STATUS_ERR_MASK    0x07000000
/* @} */

/**************************************************************************//**
 @Description   Frame Scatter/Gather Table Entry
*//***************************************************************************/
typedef _Packed struct t_FmSGTE {
    volatile uint32_t    addrh;        /**< Buffer Address high */
    volatile uint32_t    addrl;        /**< Buffer Address low */
    volatile uint32_t    length;       /**< Buffer length */
    volatile uint32_t    offset;       /**< SGTE offset */
} _PackedType t_FmSGTE;

#define FM_NUM_OF_SG_TABLE_ENTRY 16

/**************************************************************************//**
 @Description   Frame Scatter/Gather Table
*//***************************************************************************/
typedef _Packed struct t_FmSGT {
    t_FmSGTE    tableEntry[FM_NUM_OF_SG_TABLE_ENTRY];
} _PackedType t_FmSGT;

/**************************************************************************//**
 @Collection   Frame Scatter/Gather Table Entry macros
*//***************************************************************************/
#define FM_SGTE_ADDRH_MASK    0x000000ff           /**< SGTE ADDRH field mask */
#define FM_SGTE_ADDRL_MASK    0xffffffff           /**< SGTE ADDRL field mask */
#define FM_SGTE_E_MASK        0x80000000           /**< SGTE Extension field mask */
#define FM_SGTE_F_MASK        0x40000000           /**< SGTE Final field mask */
#define FM_SGTE_LENGTH_MASK   0x3fffffff           /**< SGTE LENGTH field mask */
#define FM_SGTE_BPID_MASK     0x00ff0000           /**< SGTE BPID field mask */
#define FM_SGTE_OFFSET_MASK   0x00001fff           /**< SGTE OFFSET field mask */

#define FM_SGTE_GET_ADDRH(sgte)         (((t_FmSGTE *)sgte)->addrh & FM_SGTE_ADDRH_MASK)                /**< Macro to get SGTE ADDRH field */
#define FM_SGTE_GET_ADDRL(sgte)         ((t_FmSGTE *)sgte)->addrl                                       /**< Macro to get SGTE ADDRL field */
#define FM_SGTE_GET_PHYS_ADDR(sgte)     ((physAddress_t)(((uint64_t)FM_SGTE_GET_ADDRH(sgte) << 32) | (uint64_t)FM_SGTE_GET_ADDRL(sgte))) /**< Macro to get FD ADDR field */
#define FM_SGTE_GET_EXTENSION(sgte)     ((((t_FmSGTE *)sgte)->length & FM_SGTE_E_MASK) >> (31-0))       /**< Macro to get SGTE EXTENSION field */
#define FM_SGTE_GET_FINAL(sgte)         ((((t_FmSGTE *)sgte)->length & FM_SGTE_F_MASK) >> (31-1))       /**< Macro to get SGTE FINAL field */
#define FM_SGTE_GET_LENGTH(sgte)        (((t_FmSGTE *)sgte)->length & FM_SGTE_LENGTH_MASK)              /**< Macro to get SGTE LENGTH field */
#define FM_SGTE_GET_BPID(sgte)          ((((t_FmSGTE *)sgte)->offset & FM_SGTE_BPID_MASK) >> (31-15))   /**< Macro to get SGTE BPID field */
#define FM_SGTE_GET_OFFSET(sgte)        (((t_FmSGTE *)sgte)->offset & FM_SGTE_OFFSET_MASK)              /**< Macro to get SGTE OFFSET field */
#define FM_SGTE_GET_ADDR(sgte)          XX_PhysToVirt(FM_SGTE_GET_PHYS_ADDR(sgte))

#define FM_SGTE_SET_ADDRH(sgte,val)     (((t_FmSGTE *)sgte)->addrh = ((((t_FmSGTE *)sgte)->addrh & ~FM_SGTE_ADDRH_MASK) | (val & FM_SGTE_ADDRH_MASK))) /**< Macro to set SGTE ADDRH field */
#define FM_SGTE_SET_ADDRL(sgte,val)     ((t_FmSGTE *)sgte)->addrl = val                                 /**< Macro to set SGTE ADDRL field */
#define FM_SGTE_SET_ADDR(sgte,val)                                      \
do {                                                                    \
    uint64_t physAddr = (uint64_t)(XX_VirtToPhys(val));     \
    FM_SGTE_SET_ADDRH(sgte, ((uint32_t)(physAddr >> 32)));              \
    FM_SGTE_SET_ADDRL(sgte, (uint32_t)physAddr);                        \
} while (0)                                                                                     /**< Macro to set SGTE ADDR field */
#define FM_SGTE_SET_EXTENSION(sgte,val) (((t_FmSGTE *)sgte)->length = ((((t_FmSGTE *)sgte)->length & ~FM_SGTE_E_MASK) | ((val  << (31-0))& FM_SGTE_E_MASK)))            /**< Macro to set SGTE EXTENSION field */
#define FM_SGTE_SET_FINAL(sgte,val)     (((t_FmSGTE *)sgte)->length = ((((t_FmSGTE *)sgte)->length & ~FM_SGTE_F_MASK) | ((val  << (31-1))& FM_SGTE_F_MASK)))            /**< Macro to set SGTE FINAL field */
#define FM_SGTE_SET_LENGTH(sgte,val)    (((t_FmSGTE *)sgte)->length = (((t_FmSGTE *)sgte)->length & ~FM_SGTE_LENGTH_MASK) | (val & FM_SGTE_LENGTH_MASK))                /**< Macro to set SGTE LENGTH field */
#define FM_SGTE_SET_BPID(sgte,val)      (((t_FmSGTE *)sgte)->offset = ((((t_FmSGTE *)sgte)->offset & ~FM_SGTE_BPID_MASK) | ((val  << (31-15))& FM_SGTE_BPID_MASK)))     /**< Macro to set SGTE BPID field */
#define FM_SGTE_SET_OFFSET(sgte,val)    (((t_FmSGTE *)sgte)->offset = ((((t_FmSGTE *)sgte)->offset & ~FM_SGTE_OFFSET_MASK) | ((val << (31-11))& FM_SGTE_OFFSET_MASK) )) /**< Macro to set SGTE OFFSET field */

#define MEM_MAP_END
#if defined(__MWERKS__) && !defined(__GNUC__)
#pragma pack(pop)
#endif /* defined(__MWERKS__) && ... */

/* @} */


/**************************************************************************//**
 @Description   FM Exceptions
*//***************************************************************************/
typedef enum e_FmExceptions {
    e_FM_EX_DMA_BUS_ERROR,              /**< DMA bus error. */
    e_FM_EX_DMA_READ_ECC,               /**< Read Buffer ECC error */
    e_FM_EX_DMA_SYSTEM_WRITE_ECC,       /**< Write Buffer ECC error on system side */
    e_FM_EX_DMA_FM_WRITE_ECC,           /**< Write Buffer ECC error on FM side */
    e_FM_EX_FPM_STALL_ON_TASKS,         /**< Stall of tasks on FPM */
    e_FM_EX_FPM_SINGLE_ECC,             /**< Single ECC on FPM. */
    e_FM_EX_FPM_DOUBLE_ECC,             /**< Double ECC error on FPM ram access */
    e_FM_EX_QMI_SINGLE_ECC,             /**< Single ECC on QMI. */
    e_FM_EX_QMI_DOUBLE_ECC,             /**< Double bit ECC occured on QMI */
    e_FM_EX_QMI_DEQ_FROM_UNKNOWN_PORTID,/**< Dequeu from unknown port id */
    e_FM_EX_BMI_LIST_RAM_ECC,           /**< Linked List RAM ECC error */
    e_FM_EX_BMI_PIPELINE_ECC,           /**< Pipeline Table ECC Error */
    e_FM_EX_BMI_STATISTICS_RAM_ECC,     /**< Statistics Count RAM ECC Error Enable */
    e_FM_EX_IRAM_ECC,                   /**< Double bit ECC occured on IRAM*/
    e_FM_EX_MURAM_ECC                   /**< Double bit ECC occured on MURAM*/
} e_FmExceptions;

/**************************************************************************//**
 @Group         FM_init_grp FM Initialization Unit

 @Description   FM Initialization Unit

                Initialization Flow
                Initialization of the FM Module will be carried out by the application
                according to the following sequence:
                a.  Calling the configuration routine with basic parameters.
                b.  Calling the advance initialization routines to change driver's defaults.
                c.  Calling the initialization routine.

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Function      t_FmExceptionsCallback

 @Description   Exceptions user callback routine, will be called upon an
                exception passing the exception identification.

 @Param[in]     h_App      - User's application descriptor.
 @Param[in]     exception  - The exception.
*//***************************************************************************/
typedef void (t_FmExceptionsCallback) ( t_Handle              h_App,
                                        e_FmExceptions        exception);

/**************************************************************************//**
 @Function      t_FmBusErrorCallback

 @Description   Bus error user callback routine, will be called upon a
                bus error, passing parameters describing the errors and the owner.

 @Param[in]     h_App       - User's application descriptor.
 @Param[in]     portType    - Port type (e_FmPortType)
 @Param[in]     portId      - Port id - relative to type.
 @Param[in]     addr        - Address that caused the error
 @Param[in]     tnum        - Owner of error
 @Param[in]     partition   - memory partition
*//***************************************************************************/
typedef void (t_FmBusErrorCallback) ( t_Handle        h_App,
                                      e_FmPortType    portType,
                                      uint8_t         portId,
                                      uint64_t        addr,
                                      uint8_t         tnum,
                                      uint8_t         partition);

/**************************************************************************//**
 @Description   structure for defining Ucode patch for loading.
*//***************************************************************************/
typedef struct t_FmPcdFirmwareParams {
    uint32_t                size;                   /**< Size of uCode */
    uint32_t                *p_Code;                /**< A pointer to the uCode */
} t_FmPcdFirmwareParams;

/**************************************************************************//**
 @Description   structure representing FM initialization parameters
*//***************************************************************************/
typedef struct t_FmParams {
    uint8_t                 fmId;                   /**< Index of the FM */
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    uint8_t                 partitionId;            /**< FM Partition Id */
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
#ifndef CONFIG_GUEST_PARTITION
    uint64_t                baseAddr;               /**< A pointer to base of memory mapped FM registers (virtual).*/
    t_Handle                h_FmMuram;              /**< A handle of an initialized MURAM object,
                                                         to be used by the FM */
    uint16_t                fmClkFreq;              /**< In Mhz */
    uint16_t                liodnPerPartition[FM_MAX_NUM_OF_PARTITIONS]; /**< For each partition, LIODN should be configured here. */
    t_FmExceptionsCallback  *f_Exception ;          /**< An application callback routine to
                                                         handle exceptions.*/
    t_FmBusErrorCallback    *f_BusError;            /**< An application callback routine to
                                                         handle exceptions.*/
    t_Handle                h_App;                  /**< A handle to an application layer object; This handle will
                                                         be passed by the driver upon calling the above callbacks */
    int                     irq;                    /**< FM interrupt source for normal events */
    int                     errIrq;                 /**< FM interrupt source for errors */
    t_FmPcdFirmwareParams   firmware;               /**< Ucode */
#endif /* CONFIG_GUEST_PARTITION */
} t_FmParams;


/**************************************************************************//**
 @Function      FM_Config

 @Description   Creates descriptor for the FM module.

                The routine returns a handle (descriptor) to the FM object.
                This descriptor must be passed as first parameter to all other
                FM function calls.

                No actual initialization or configuration of FM hardware is
                done by this routine.

 @Param[in]     p_FmParams  - A pointer to data structure of parameters

 @Return        Handle to FM object, or NULL for Failure.
*//***************************************************************************/
t_Handle FM_Config(t_FmParams *p_FmParams);

/**************************************************************************//**
 @Function      FM_Init

 @Description   Initializes the FM module

 @Param[in]     h_Fm - FM module descriptor

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
t_Error FM_Init(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FM_Free

 @Description   Frees all resources that were assigned to FM module.

                Calling this routine invalidates the descriptor.

 @Param[in]     h_Fm - FM module descriptor

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
t_Error FM_Free(t_Handle h_Fm);


/**************************************************************************//**
 @Group         FM_advanced_init_grp    FM Advanced Configuration Unit

 @Description   Configuration functions used to change default values.

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Description   DMA debug mode
*//***************************************************************************/
typedef enum e_FmDmaDbgCntMode {
    e_FM_DMA_DBG_NO_CNT             = 0,    /**< No counting */
    e_FM_DMA_DBG_CNT_DONE           = 1,    /**< Count DONE commands */
    e_FM_DMA_DBG_CNT_COMM_Q_EM      = 2,    /**< count command queue emergency signals */
    e_FM_DMA_DBG_CNT_INT_READ_EM    = 3,    /**< Count Internal Read buffer emergency signal */
    e_FM_DMA_DBG_CNT_INT_WRITE_EM   = 4,    /**< Count Internal Write buffer emergency signal */
    e_FM_DMA_DBG_CNT_FPM_WAIT       = 5,    /**< Count FPM WAIT signal */
    e_FM_DMA_DBG_CNT_SIGLE_BIT_ECC  = 6,    /**< Single bit ECC errors. */
    e_FM_DMA_DBG_CNT_RAW_WAR_PROT   = 7     /**< Number of times there was a need for RAW & WAR protection. */
} e_FmDmaDbgCntMode;

/**************************************************************************//**
 @Description   DMA Cache Override
*//***************************************************************************/
typedef enum e_FmDmaCacheOverride {
    e_FM_DMA_NO_CACHE_OR = 0,               /**< No override of the Cache field */
    e_FM_DMA_NO_STASH_DATA,                 /**< Data should not be stashed in system level cache */
    e_FM_DMA_MAY_STASH_DATA,                /**< Data may be stashed in system level cache */
    e_FM_DMA_STASH_DATA                     /**< Data should be stashed in system level cache */
} e_FmDmaCacheOverride;

/**************************************************************************//**
 @Description   DMA External Bus Priority
*//***************************************************************************/
typedef enum e_FmDmaExtBusPri {
    e_FM_DMA_EXT_BUS_NORMAL = 0,            /**< Normal priority */
    e_FM_DMA_EXT_BUS_EBS,                   /**< AXI extended bus service priority */
    e_FM_DMA_EXT_BUS_SOS,                   /**< AXI sos priority */
    e_FM_DMA_EXT_BUS_EBS_AND_SOS            /**< AXI ebs + sos priority */
} e_FmDmaExtBusPri;

/**************************************************************************//**
 @Description   enum for choosing the field that will be output on AID
*//***************************************************************************/
typedef enum e_FmDmaAidMode {
    e_FM_DMA_AID_OUT_PORT_ID = 0,           /**< 4 LSB of PORT_ID */
    e_FM_DMA_AID_OUT_TNUM                   /**< 4 LSB of TNUM */
} e_FmDmaAidMode;

/**************************************************************************//**
 @Description   DMA AXI Bus protection
*//***************************************************************************/
typedef enum e_FmDmaBusProtectionType {
    e_FM_DMA_DATA_BUS_PROT = 0,             /**< AXI data bus protection */
    e_FM_DMA_INSTRUCTION_BUS_PROT           /**< AXI instruction bus protection */
} e_FmDmaBusProtectionType;

/**************************************************************************//**
 @Description   FPM Catasrophic error behaviour
*//***************************************************************************/
typedef enum e_FmCatastrophicErr {
    e_FM_CATASTROPHIC_ERR_STALL_PORT = 0,   /**< Port_ID is stalled (only reset can release it) */
    e_FM_CATASTROPHIC_ERR_STALL_TASK        /**< Only errornous task is stalled */
} e_FmCatastrophicErr;

/**************************************************************************//**
 @Description   FPM DMA error behaviour
*//***************************************************************************/
typedef enum e_FmDmaErr {
    e_FM_DMA_ERR_CATASTROPHIC = 0,          /**< Dma error is treated as a catastrophic error */
    e_FM_DMA_ERR_REPORT                     /**< Dma error is just reported */
} e_FmDmaErr;

/**************************************************************************//**
 @Description   DMA Emergency level by BMI emergency signal
*//***************************************************************************/
typedef enum e_FmDmaEmergencyLevel {
    e_FM_DMA_EM_EBS = 0,                    /**< EBS emergency */
    e_FM_DMA_EM_SOS                         /**< SOS emergency */
} e_FmDmaEmergencyLevel;

/**************************************************************************//**
 @Collection   DMA emergency options
*//***************************************************************************/
typedef uint32_t fmEmergencyBus_t;          /**< DMA emergency options */

#define  FM_DMA_MURAM_READ_EMERGENCY        0x00800000    /**< Enable emergency for MURAM1 */
#define  FM_DMA_MURAM_WRITE_EMERGENCY       0x00400000    /**< Enable emergency for MURAM2 */
#define  FM_DMA_EXT_BUS_EMERGENCY           0x00100000    /**< Enable emergency for external bus */
/* @} */

/**************************************************************************//**
 @Description   A structure for defining DMA emergency level
*//***************************************************************************/
typedef struct t_FmDmaEmergency {
    fmEmergencyBus_t        emergencyBusSelect;             /**< An OR of the busses where emergency
                                                                 should be enabled */
    e_FmDmaEmergencyLevel   emergencyLevel;                 /**< EBS/SOS */
} t_FmDmaEmergency;

/**************************************************************************//**
 @Description   structure for defining FM threshold
*//***************************************************************************/
typedef struct t_FmThresholds {
    uint8_t                 dispLimit;                      /**< The number of times a frames may
                                                                 be passed in the FM before assumed to
                                                                 be looping. */
    uint8_t                 prsDispTh;                      /**< This is the number pf packets that may be
                                                                 queued in the parser dispatch queue*/
    uint8_t                 plcrDispTh;                     /**< This is the number pf packets that may be
                                                                 queued in the policer dispatch queue*/
    uint8_t                 kgDispTh;                       /**< This is the number pf packets that may be
                                                                 queued in the keygen dispatch queue*/
    uint8_t                 bmiDispTh;                      /**< This is the number pf packets that may be
                                                                 queued in the BMI dispatch queue*/
    uint8_t                 qmiEnqDispTh;                   /**< This is the number pf packets that may be
                                                                 queued in the QMI enqueue dispatch queue*/
    uint8_t                 qmiDeqDispTh;                   /**< This is the number pf packets that may be
                                                                 queued in the QMI dequeue dispatch queue*/
    uint8_t                 fmCtl1DispTh;                    /**< This is the number pf packets that may be
                                                                 queued in fmCtl1 dispatch queue*/
    uint8_t                 fmCtl2DispTh;                    /**< This is the number pf packets that may be
                                                                 queued in fmCtl2 dispatch queue*/
} t_FmThresholds;

/**************************************************************************//**
 @Description   structure for defining DMA mode parameters
*//***************************************************************************/
typedef struct t_FmDmaBusProtect {
    bool                        privilegeBusProtect;        /**< TRUE to select privilage bus protection */
    e_FmDmaBusProtectionType    busProtectType;             /**< Data/Instruction protect select. */
} t_FmDmaBusProtect;

/**************************************************************************//**
 @Description   structure for defining DMA thresholds
*//***************************************************************************/
typedef struct t_FmDmaThresholds {
    uint8_t                     assertEmergency;            /**< When this value is reached,
                                                                 assert emergency (Threshold)*/
    uint8_t                     clearEmergency;             /**< After emergency is asserted, it is held
                                                                 until this value is reached (Hystheresis) */
} t_FmDmaThresholds;


/**************************************************************************//**
 @Function      FM_ConfigResetOnInit

 @Description   Tell the driver whether to reset the FM before initialization or
                not. It changes the default configuration [DEFAULT_resetOnInit].

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[in]     enable              When TRUE, FM will be reset before any initialization.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigResetOnInit(t_Handle h_Fm, bool enable);

/**************************************************************************//**
 @Function      FM_ConfigTotalNumOfTasks

 @Description   Change the total number of tasks from its default
                configuration [DEFAULT_totalNumOfTasks]

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[in]     totalNumOfTasks     The selected new value.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigTotalNumOfTasks(t_Handle h_Fm, uint8_t totalNumOfTasks);

/**************************************************************************//**
 @Function      FM_ConfigTotalFifoSize

 @Description   Change the total Fifo size from its default
                configuration [DEFAULT_totalFifoSize]

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[in]     totalFifoSize       The selected new value.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigTotalFifoSize(t_Handle h_Fm, uint32_t totalFifoSize);

/**************************************************************************//**
 @Function      FM_ConfigMaxNumOfOpenDmas

 @Description   Change the maximum allowed open DMA's for this FM from its default
                configuration [DEFAULT_maxNumOfOpenDmas]

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[in]     maxNumOfOpenDmas    The selected new value.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigMaxNumOfOpenDmas(t_Handle h_Fm, uint8_t maxNumOfOpenDmas);

/**************************************************************************//**
 @Function      FM_ConfigThresholds

 @Description   Calling this routine changes the internal driver data base
                from its default FM threshold configuration:
                                          dispLimit:    [DEFAULT_dispLimit]
                                          prsDispTh:    [DEFAULT_prsDispTh]
                                          plcrDispTh:   [DEFAULT_plcrDispTh]
                                          kgDispTh:     [DEFAULT_kgDispTh]
                                          bmiDispTh:    [DEFAULT_bmiDispTh]
                                          qmiEnqDispTh: [DEFAULT_qmiEnqDispTh]
                                          qmiDeqDispTh: [DEFAULT_qmiDeqDispTh]
                                          fmCtl1DispTh:  [DEFAULT_fmCtl1DispTh]
                                          fmCtl2DispTh:  [DEFAULT_fmCtl2DispTh]

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     p_FmThresholds  A structure of threshold parameters.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigThresholds(t_Handle h_Fm, t_FmThresholds *p_FmThresholds);

/**************************************************************************//**
 @Function      FM_ConfigDmaBusProtect

 @Description   Calling this routine changes the internal driver data base
                from its default FM threshold configuration
                                  privilegeBusProtect:      [DEFAULT_privilegeBusProtect]
                                  busProtectType:           [DEFAULT_busProtectionType]

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[in]     p_FmDmaBusProtect   A structure of bus protection parameters.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaBusProtect(t_Handle h_Fm, t_FmDmaBusProtect *p_FmDmaBusProtect);

 /**************************************************************************//**
 @Function      FM_ConfigDmaCacheOverride

 @Description   Calling this routine changes the internal driver data base
                from its default configuration of cache override mode [DEFAULT_cacheOverride]

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     cacheOverride   The selected new value.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaCacheOverride(t_Handle h_Fm, e_FmDmaCacheOverride cacheOverride);

/**************************************************************************//**
 @Function      FM_ConfigDmaAidOverride

 @Description   Calling this routine changes the internal driver data base
                from its default configuration of aid override mode [DEFAULT_aidOverride]

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     aidOverride     The selected new value.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaAidOverride(t_Handle h_Fm, bool aidOverride);

/**************************************************************************//**
 @Function      FM_ConfigDmaAidMode

 @Description   Calling this routine changes the internal driver data base
                from its default configuration of aid mode [DEFAULT_aidMode]

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     aidMode         The selected new value.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaAidMode(t_Handle h_Fm, e_FmDmaAidMode aidMode);

/**************************************************************************//**
 @Function      FM_ConfigDmaAxiDbgNumOfBeats

 @Description   Calling this routine changes the internal driver data base
                from its default configuration of axi debug [DEFAULT_axiDbgNumOfBeats]

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[in]     axiDbgNumOfBeats    The selected new value.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaAxiDbgNumOfBeats(t_Handle h_Fm, uint8_t axiDbgNumOfBeats);

/**************************************************************************//**
 @Function      FM_ConfigDmaCamNumOfEntries

 @Description   Calling this routine changes the internal driver data base
                from its default configuration of number of CAM entries [DEFAULT_dmaCamNumOfEntries]

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     numOfEntries    The selected new value.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaCamNumOfEntries(t_Handle h_Fm, uint8_t numOfEntries);

/**************************************************************************//**
 @Function      FM_ConfigDmaWatchdog

 @Description   Calling this routine changes the internal driver data base
                from its default watchdog configuration, which is the maximum
                possible value.

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     watchDogValue   The selected new value - in microseconds.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaWatchdog(t_Handle h_Fm, uint32_t watchDogValue);

/**************************************************************************//**
 @Function      FM_ConfigDmaWriteBufThresholds

 @Description   Calling this routine changes the internal driver data base
                from its default configuration of DMA write buffer threshold
                assertEmergency: [DEFAULT_dmaWriteIntBufLow]
                clearEmergency:  [DEFAULT_dmaWriteIntBufHigh]

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[in]     p_FmDmaThresholds   A structure of thresholds to define emegrency behavior -
                                    When 'assertEmergency' value is reached, emergency is asserted,
                                    then it is held until 'clearEmergency' value is reached.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaWriteBufThresholds(t_Handle h_Fm, t_FmDmaThresholds *p_FmDmaThresholds);

 /**************************************************************************//**
 @Function      FM_ConfigDmaCommQThresholds

 @Description   Calling this routine changes the internal driver data base
                from its default configuration of DMA command queue threshold
                assertEmergency: [DEFAULT_dmaCommQLow]
                clearEmergency:  [DEFAULT_dmaCommQHigh]

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[in]     p_FmDmaThresholds   A structure of thresholds to define emegrency behavior -
                                    When 'assertEmergency' value is reached, emergency is asserted,
                                    then it is held until 'clearEmergency' value is reached..

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaCommQThresholds(t_Handle h_Fm, t_FmDmaThresholds *p_FmDmaThresholds);

/**************************************************************************//**
 @Function      FM_ConfigDmaReadBufThresholds

 @Description   Calling this routine changes the internal driver data base
                from its default configuration of DMA read buffer threshold
                assertEmergency: [DEFAULT_dmaReadIntBufLow]
                clearEmergency:  [DEFAULT_dmaReadIntBufHigh]

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[in]     p_FmDmaThresholds   A structure of thresholds to define emegrency behavior -
                                    When 'assertEmergency' value is reached, emergency is asserted,
                                    then it is held until 'clearEmergency' value is reached..

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaReadBufThresholds(t_Handle h_Fm, t_FmDmaThresholds *p_FmDmaThresholds);

/**************************************************************************//**
 @Function      FM_ConfigDmaSosEmergencyThreshold

 @Description   Calling this routine changes the internal driver data base
                from its default dma SOS emergency configuration [DEFAULT_dmaSosEmergency]

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[in]     dmaSosEmergency     The selected new value.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaSosEmergencyThreshold(t_Handle h_Fm, uint32_t dmaSosEmergency);

/**************************************************************************//**
 @Function      FM_ConfigEnableCounters

 @Description   Calling this routine changes the internal driver data base
                from its default counters configuration where counters are disabled.

 @Param[in]     h_Fm    A handle to an FM Module.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigEnableCounters(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FM_ConfigDmaDbgCounter

 @Description   Calling this routine changes the internal driver data base
                from its default DMA debug counters configuration [DEFAULT_dmaDbgCntMode]

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[in]     fmDmaDbgCntMode     An enum selecting the debug counter mode.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaDbgCounter(t_Handle h_Fm, e_FmDmaDbgCntMode fmDmaDbgCntMode);

/**************************************************************************//**
 @Function      FM_ConfigDmaStopOnBusErr

 @Description   Calling this routine changes the internal driver data base
                from its default selection of bus error behavior [DEFAULT_dmaStopOnBusError]


 @Param[in]     h_Fm    A handle to an FM Module.
 @Param[in]     stop    TRUE to stop on bus error, FALSE to continue.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
                Only if bus error is enabled.
*//***************************************************************************/
t_Error FM_ConfigDmaStopOnBusErr(t_Handle h_Fm, bool stop);

/**************************************************************************//**
 @Function      FM_ConfigDmaEmergency

 @Description   Calling this routine changes the internal driver data base
                from its default selection of DMA emergency where's it's disabled.

 @Param[in]     h_Fm        A handle to an FM Module.
 @Param[in]     p_Emergency An OR mask of all required options.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaEmergency(t_Handle h_Fm, t_FmDmaEmergency *p_Emergency);

/**************************************************************************//**
 @Function      FM_ConfigDmaEmergencySmoother

 @Description   sets the minimum amount of DATA beats transferred on the AXI
                READ and WRITE ports before lowering the emergency level.
                By default smother is disabled.

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     emergencyCnt    emergency switching counter.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaEmergencySmoother(t_Handle h_Fm, uint32_t emergencyCnt);

/**************************************************************************//**
 @Function      FM_ConfigDmaErr

 @Description   Calling this routine changes the internal driver data base
                from its default DMA error treatment [DEFAULT_dmaErr]

 @Param[in]     h_Fm    A handle to an FM Module.
 @Param[in]     dmaErr  The selected new choice.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigDmaErr(t_Handle h_Fm, e_FmDmaErr dmaErr);

/**************************************************************************//**
 @Function      FM_ConfigCatastrophicErr

 @Description   Calling this routine changes the internal driver data base
                from its default behavior on catastrophic error [DEFAULT_catastrophicErr]

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[in]     catastrophicErr     The selected new choice.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigCatastrophicErr(t_Handle h_Fm, e_FmCatastrophicErr catastrophicErr);

/**************************************************************************//**
 @Function      FM_ConfigEnableMuramTestMode

 @Description   Calling this routine changes the internal driver data base
                from its default selection of test mode where it's disabled.

 @Param[in]     h_Fm    A handle to an FM Module.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigEnableMuramTestMode(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FM_ConfigEnableIramTestMode

 @Description   Calling this routine changes the internal driver data base
                from its default selection of test mode where it's disabled.

 @Param[in]     h_Fm    A handle to an FM Module.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigEnableIramTestMode(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FM_ConfigHaltOnExternalActivation

 @Description   Calling this routine changes the internal driver data base
                from its default selection of FM behaviour on external halt
                activation [DEFAULT_haltOnExternalActivation].

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     enable          TRUE to enable halt on external halt
                                activation.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigHaltOnExternalActivation(t_Handle h_Fm, bool enable);

/**************************************************************************//**
 @Function      FM_ConfigHaltOnUnrecoverableEccError

 @Description   Calling this routine changes the internal driver data base
                from its default selection of FM behaviour on unrecoverable
                Ecc error [DEFAULT_haltOnUnrecoverableEccError].

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     enable          TRUE to enable halt on unrecoverable Ecc error

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigHaltOnUnrecoverableEccError(t_Handle h_Fm, bool enable);

/**************************************************************************//**
 @Function      FM_ConfigException

 @Description   Calling this routine changes the internal driver data base
                from its default selection of exceptions enablement.
                By default all exceptions are enabled.

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     exception       The exception to be selected.
 @Param[in]     enable          TRUE to enable interrupt, FALSE to mask it.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigException(t_Handle h_Fm, e_FmExceptions exception, bool enable);

/**************************************************************************//**
 @Function      FM_ConfigExternalEccRamsEnable

 @Description   Calling this routine changes the internal driver data base
                from its default [DEFAULT_externalEccRamsEnable].
                When this option is enabled Rams ECC enable is not effected
                by the FPM RCR bit, but by a JTAG.

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     enable          TRUE to enable this option.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_ConfigExternalEccRamsEnable(t_Handle h_Fm, bool enable);

/** @} */ /* end of FM_advanced_init_grp group */
/** @} */ /* end of FM_init_grp group */


/**************************************************************************//**
 @Group         FM_runtime_control_grp FM Runtime Control Unit

 @Description   FM Runtime control unit API functions, definitions and enums.
                The FM driver provides a set of control routines for each module.
                These routines may only be called after the module was fully
                initialized (both configuration and initialization routines were
                called). They are typically used to get information from hardware
                (status, counters/statistics, revision etc.), to modify a current
                state or to force/enable a required action. Run-time control may
                be called whenever necessary and as many times as needed.
 @{
*//***************************************************************************/

/**************************************************************************//**
 @Collection   General FM defines.
*//***************************************************************************/
#define FM_NUM_OF_PORT_TYPES               e_FM_PORT_TYPE_DUMMY    /**< Number of port types */
#define FM_MAX_NUM_OF_PORTS_PER_TYPE       7                       /**< Max number of ports of the same type */
/* @} */

/**************************************************************************//**
 @Description   Port id by type and relative id.
                This type is a 2 dimentional array of port id's according
                to port types.used to pass per-port parameters.
                Note that not all places in the array are valid e.g
                array[e_FM_PORT_TYPE_TX_10G][1] is not a valid indexes pair.
*//***************************************************************************/
typedef uint8_t t_PortsParam[FM_NUM_OF_PORT_TYPES][FM_MAX_NUM_OF_PORTS_PER_TYPE];

/**************************************************************************//**
 @Description   DMA Emergency control on MURAM
*//***************************************************************************/
typedef enum e_FmDmaMuramPort {
    e_FM_DMA_MURAM_PORT_WRITE,              /**< MURAM write port */
    e_FM_DMA_MURAM_PORT_READ                /**< MURAM read port */
} e_FmDmaMuramPort;

/**************************************************************************//**
 @Description   enum for defining FM counters
*//***************************************************************************/
typedef enum e_FmCounters {
    e_FM_COUNTERS_ENQ_TOTAL_FRAME,              /**< QMI total enqueued frames counter */
    e_FM_COUNTERS_DEQ_TOTAL_FRAME,              /**< QMI total dequeued frames counter */
    e_FM_COUNTERS_DEQ_0,                        /**< QMI 0 frames from QMan counter */
    e_FM_COUNTERS_DEQ_1,                        /**< QMI 1 frames from QMan counter */
    e_FM_COUNTERS_DEQ_2,                        /**< QMI 2 frames from QMan counter */
    e_FM_COUNTERS_DEQ_3,                        /**< QMI 3 frames from QMan counter */
    e_FM_COUNTERS_DEQ_FROM_DEFAULT,             /**< QMI dequeue from default queue counter */
    e_FM_COUNTERS_DEQ_FROM_CONTEXT,             /**< QMI dequeue from FQ context counter */
    e_FM_COUNTERS_DEQ_FROM_FD,                  /**< QMI dequeue from FD command field counter */
    e_FM_COUNTERS_DEQ_CONFIRM,                  /**< QMI dequeue confirm counter */
    e_FM_COUNTERS_SEMAPHOR_ENTRY_FULL_REJECT,   /**< DMA semaphor reject due to full entry counter */
    e_FM_COUNTERS_SEMAPHOR_QUEUE_FULL_REJECT,   /**< DMA semaphor reject due to full CAM queue counter */
    e_FM_COUNTERS_SEMAPHOR_SYNC_REJECT          /**< DMA semaphor reject due to sync counter */
} e_FmCounters;

/**************************************************************************//**
 @Description   structure for returning revision information
*//***************************************************************************/
typedef struct t_FmRevisionInfo {
    uint8_t         majorRev;               /**< Major revision */
    uint8_t         minorRev;               /**< Minor revision */
} t_FmRevisionInfo;

/**************************************************************************//**
 @Description   struct for defining DMA status
*//***************************************************************************/
typedef struct t_FmDmaStatus {
    bool    cmqNotEmpty;            /**< Command queue is not empty */
    bool    busError;               /**< Bus error occured */
    bool    readBufEccError;        /**< Double ECC error on buffer Read */
    bool    writeBufEccSysError;    /**< Double ECC error on buffer write from system side */
    bool    writeBufEccFmError;     /**< Double ECC error on buffer write from FM side */
} t_FmDmaStatus;


#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
/**************************************************************************//**
 @Function      FM_DumpRegs

 @Description   Dumps all FM registers

 @Param[in]     h_Fm      A handle to an FM Module.

 @Return        E_OK on success;

 @Cautions      Allowed only FM_Init().
*//***************************************************************************/
t_Error FM_DumpRegs(t_Handle h_Fm);
#endif /* (defined(DEBUG_ERRORS) && ... */

/**************************************************************************//**
 @Function      FM_SetException

 @Description   Calling this routine enables/disables the specified exception.

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     exception       The exception to be selected.
 @Param[in]     enable          TRUE to enable interrupt, FALSE to mask it.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
t_Error FM_SetException(t_Handle h_Fm, e_FmExceptions exception, bool enable);

/**************************************************************************//**
 @Function      FM_SetPortsBandwidth

 @Description   Sets relative weights between ports when accessing common resources.

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[in]     p_PortsBandwidth    A table of ports bandwidth in percentage, i.e.
                                    total must equal 100.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
t_Error FM_SetPortsBandwidth(t_Handle h_Fm, t_PortsParam *p_PortsBandwidth);

/**************************************************************************//**
 @Function      FM_EnableRamsEcc

 @Description   Enables ECC mechanism for all the different FM RAM's; E.g. IRAM,
                MURAM, Parser, Keygen, Policer, etc.
                Note:
                If FM_ConfigExternalEccRamsEnable was called to enable external
                setting of ECC, this routine effects IRAM ECC only.
                This routine is also called by the driver if an ECC exception is
                enabled.

 @Param[in]     h_Fm            A handle to an FM Module.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_EnableRamsEcc(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FM_DisableRamsEcc

 @Description   Disables ECC mechanism for all the different FM RAM's; E.g. IRAM,
                MURAM, Parser, Keygen, Policer, etc.
                Note:
                If FM_ConfigExternalEccRamsEnable was called to enable external
                setting of ECC, this routine effects IRAM ECC only.
                In opposed to FM_EnableRamsEcc, this routine must be called
                explicitly to disable all Rams ECC.


 @Param[in]     h_Fm            A handle to an FM Module.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Config() and before FM_Init().
*//***************************************************************************/
t_Error FM_DisableRamsEcc(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FM_GetRevision

 @Description   Returns the FM revision

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[out]    p_FmRevisionInfo    A structure of revision information parameters.

 @Return        None.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
void  FM_GetRevision(t_Handle h_Fm, t_FmRevisionInfo *p_FmRevisionInfo);

/**************************************************************************//**
 @Function      FM_GetCounter

 @Description   Reads one of the FM counters.

 @Param[in]     h_Fm        A handle to an FM Module.
 @Param[in]     counter     The requested counter.

 @Return        Counter's current value.

 @Cautions      Allowed only following FM_Init().
                Note that it is user's responsibilty to call this routine only
                for enabled counters, and there will be no indication if a
                disabled counter is accessed.
*//***************************************************************************/
uint32_t  FM_GetCounter(t_Handle h_Fm, e_FmCounters counter);

/**************************************************************************//**
 @Function      FM_ModifyCounter

 @Description   Sets a value to an enabled counter. Use "0" to reset the counter.

 @Param[in]     h_Fm        A handle to an FM Module.
 @Param[in]     counter     The requested counter.
 @Param[in]     val         The requested value to be written into the counter.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
t_Error  FM_ModifyCounter(t_Handle h_Fm, e_FmCounters counter, uint32_t val);

/**************************************************************************//**
 @Function      FM_Resume

 @Description   Release FM after halt FM command or after unrecoverable ECC error.

 @Param[in]     h_Fm        A handle to an FM Module.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
void FM_Resume(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FM_SetDmaEmergency

 @Description   Manual emergency set

 @Param[in]     h_Fm        A handle to an FM Module.
 @Param[in]     muramPort   MURAM direction select.
 @Param[in]     enable      TRUE to manualy enable emergency, FALSE to disable.

 @Return        None.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
void FM_SetDmaEmergency(t_Handle h_Fm, e_FmDmaMuramPort muramPort, bool enable);

/**************************************************************************//**
 @Function      FM_SetDmaExtBusPri

 @Description   Manual emergency set

 @Param[in]     h_Fm    A handle to an FM Module.
 @Param[in]     pri     External bus priority select

 @Return        None.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
void FM_SetDmaExtBusPri(t_Handle h_Fm, e_FmDmaExtBusPri pri);

/**************************************************************************//**
 @Function      FM_ForceIntr

 @Description   Causes an interrupt event on the requested source.

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     exception       An exception to be forced.

 @Return        E_OK on success; Error code if the exception is not enabled,
                or is not able to create interrupt.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
t_Error FM_ForceIntr (t_Handle h_Fm, e_FmExceptions exception);

/**************************************************************************//**
 @Function      FM_GetDmaStatus

 @Description   Reads the DMA current status

 @Param[in]     h_Fm                A handle to an FM Module.
 @Param[out]     p_FmDmaStatus      A structure of DMA status parameters.

 @Return        None

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
void FM_GetDmaStatus(t_Handle h_Fm, t_FmDmaStatus *p_FmDmaStatus);

/**************************************************************************//**
 @Function      FM_GetPcdHandle

 @Description   Used by FMC in order to get PCD handle

 @Param[in]     h_Fm     A handle to an FM Module.

 @Return        A handle to the PCD module, NULL if uninitialized.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
t_Handle FM_GetPcdHandle(t_Handle h_Fm);

#ifndef CONFIG_GUEST_PARTITION
/**************************************************************************//**
 @Function      FM_ErrorIsr

 @Description   FM interrupt-service-routine for errors.

 @Param[in]     h_Fm            A handle to an FM Module.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
void FM_ErrorIsr(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FM_EventIsr

 @Description   FM interrupt-service-routine for normal events.

 @Param[in]     h_Fm            A handle to an FM Module.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
void FM_EventIsr(t_Handle h_Fm);
#endif /* !CONFIG_GUEST_PARTITION */

/** @} */ /* end of FM_runtime_control_grp group */

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
/**************************************************************************//**
 @Function      FmDumpPortRegs

 @Description   Dumps FM port registers which are part of FM common registers

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     hardwarePortId    HW port id.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only FM_Init().
*//***************************************************************************/
t_Error FmDumpPortRegs(t_Handle h_Fm,uint8_t hardwarePortId);
#endif /* (defined(DEBUG_ERRORS) && ... */

/** @} */ /* end of FM_lib_grp group */
/** @} */ /* end of FM_grp group */
/** @} */ /* end of DPAA_grp group */


#endif /* __FM_EXT */
