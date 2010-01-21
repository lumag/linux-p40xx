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

/******************************************************************************
 @File          fm_common.h

 @Description   FM internal structures and definitions.
*//***************************************************************************/
#ifndef __FM_COMMON_H
#define __FM_COMMON_H

#include "error_ext.h"
#include "std_ext.h"
#include "fm_pcd_ext.h"
#include "fm_port_ext.h"


#define CLS_PLAN_NUM_PER_GRP                        8
#define DRIVER_PRIVATE_NET_ENV_ID                   PCD_MAX_NUM_OF_PORTS


#if defined(__MWERKS__) && !defined(__GNUC__)
#pragma pack(push,1)
#endif /* defined(__MWERKS__) && ... */
#define MEM_MAP_START

/**************************************************************************//**
 @Description   PCD KG scheme registers
*//***************************************************************************/
typedef _Packed struct t_FmPcdPlcrInterModuleProfileRegs {
    volatile uint32_t fmpl_pemode;      /* 0x090 FMPL_PEMODE - FM Policer Profile Entry Mode*/
    volatile uint32_t fmpl_pegnia;      /* 0x094 FMPL_PEGNIA - FM Policer Profile Entry GREEN Next Invoked Action*/
    volatile uint32_t fmpl_peynia;      /* 0x098 FMPL_PEYNIA - FM Policer Profile Entry YELLOW Next Invoked Action*/
    volatile uint32_t fmpl_pernia;      /* 0x09C FMPL_PERNIA - FM Policer Profile Entry RED Next Invoked Action*/
    volatile uint32_t fmpl_pecir;       /* 0x0A0 FMPL_PECIR  - FM Policer Profile Entry Committed Information Rate*/
    volatile uint32_t fmpl_pecbs;       /* 0x0A4 FMPL_PECBS  - FM Policer Profile Entry Committed Burst Size*/
    volatile uint32_t fmpl_pepepir_eir; /* 0x0A8 FMPL_PEPIR_EIR - FM Policer Profile Entry Peak/Excess Information Rate*/
    volatile uint32_t fmpl_pepbs_ebs;   /* 0x0AC FMPL_PEPBS_EBS - FM Policer Profile Entry Peak/Excess Information Rate*/
    volatile uint32_t fmpl_pelts;       /* 0x0B0 FMPL_PELTS  - FM Policer Profile Entry Last TimeStamp*/
    volatile uint32_t fmpl_pects;       /* 0x0B4 FMPL_PECTS  - FM Policer Profile Entry Committed Token Status*/
    volatile uint32_t fmpl_pepts_ets;   /* 0x0B8 FMPL_PEPTS_ETS - FM Policer Profile Entry Peak/Excess Token Status*/
    volatile uint32_t fmpl_pegpc;       /* 0x0BC FMPL_PEGPC  - FM Policer Profile Entry GREEN Packet Counter*/
    volatile uint32_t fmpl_peypc;       /* 0x0C0 FMPL_PEYPC  - FM Policer Profile Entry YELLOW Packet Counter*/
    volatile uint32_t fmpl_perpc;       /* 0x0C4 FMPL_PERPC  - FM Policer Profile Entry RED Packet Counter */
    volatile uint32_t fmpl_perypc;      /* 0x0C8 FMPL_PERYPC - FM Policer Profile Entry Recolored YELLOW Packet Counter*/
    volatile uint32_t fmpl_perrpc;      /* 0x0CC FMPL_PERRPC - FM Policer Profile Entry Recolored RED Packet Counter*/
    volatile uint32_t fmpl_res1[12];    /* 0x0D0-0x0FF Reserved */
} _PackedType t_FmPcdPlcrInterModuleProfileRegs;

/**************************************************************************//**
 @Description   PCD KG scheme registers
*//***************************************************************************/
typedef _Packed struct t_FmPcdKgInterModuleSchemeRegs {
    volatile uint32_t kgse_mode;    /**< MODE */
    volatile uint32_t kgse_ekfc;    /**< Extract Known Fields Command */
    volatile uint32_t kgse_ekdv;    /**< Extract Known Default Value */
    volatile uint32_t kgse_bmch;    /**< Bit Mask Command High */
    volatile uint32_t kgse_bmcl;    /**< Bit Mask Command Low */
    volatile uint32_t kgse_fqb;     /**< Frame Queue Base */
    volatile uint32_t kgse_hc;      /**< Hash Command */
    volatile uint32_t kgse_ppc;     /**< Policer Profile Command */
    volatile uint32_t kgse_gec[FM_PCD_KG_NUM_OF_GENERIC_REGS];
                                   /**< Generic Extract Command */
    volatile uint32_t kgse_spc;     /**< KeyGen Scheme Entry Statistic Packet Counter */
    volatile uint32_t kgse_dv0;     /**< KeyGen Scheme Entry Default Value 0 */
    volatile uint32_t kgse_dv1;     /**< KeyGen Scheme Entry Default Value 1 */
    volatile uint32_t kgse_ccbs;    /**< KeyGen Scheme Entry Coarse Classification Bit*/
    volatile uint32_t kgse_mv;      /**< KeyGen Scheme Entry Match vector */
} _PackedType t_FmPcdKgInterModuleSchemeRegs;

#define MEM_MAP_END
#if defined(__MWERKS__) && !defined(__GNUC__)
#pragma pack(pop)
#endif /* defined(__MWERKS__) && ... */

typedef struct {
    uint8_t             baseEntry;
    uint16_t            numOfClsPlanEntries;
    uint32_t            vectors[FM_PCD_MAX_NUM_OF_CLS_PLANS];
} t_FmPcdKgInterModuleClsPlanSet;

/**************************************************************************//**
 @Description   Structure for binding a port to keygen schemes.
*//***************************************************************************/
typedef struct t_FmPcdKgInterModuleBindPortToSchemes {
    uint8_t     hardwarePortId;
    uint8_t     netEnvId;
    bool        useClsPlan;                 /**< TRUE if this port uses the clsPlan mechanism */
    uint8_t     numOfSchemes;
    uint8_t     schemesIds[FM_PCD_KG_NUM_OF_SCHEMES];
} t_FmPcdKgInterModuleBindPortToSchemes;


#define CC_NEXT_NODE_F_OBJECT(ptr)  LIST_OBJECT(ptr, t_CcNodeInfo, h_Node)

typedef struct {
    uint32_t nextCcNodeInfo;
    t_List   h_Node;
} t_CcNodeInfo;


#define TRY_LOCK_RET_ERR(lock)                                  \
do {                                                            \
    unsigned long intFlags;                                          \
    intFlags = XX_DisableAllIntr();                             \
    if (lock)                                                   \
    {                                                           \
        XX_RestoreAllIntr(intFlags);                            \
        return ERROR_CODE(E_BUSY);                              \
    }                                                           \
    lock = TRUE;                                                \
    XX_RestoreAllIntr(intFlags);                                \
} while (0)

#define TRY_LOCK_RET_NULL(lock)                                 \
do {                                                            \
    unsigned long intFlags;                                          \
    intFlags = XX_DisableAllIntr();                             \
    if (lock)                                                   \
    {                                                           \
        XX_RestoreAllIntr(intFlags);                            \
        return NULL;                                            \
    }                                                           \
    lock = TRUE;                                                \
    XX_RestoreAllIntr(intFlags);                                \
} while (0)

#define RELEASE_LOCK(lock) lock = FALSE;

#if defined (CONFIG_GUEST_PARTITION) && !defined (CONFIG_MULTI_PARTITION_SUPPORT)
#error "CONFIG_MULTI_PARTITION_SUPPORT must be defined!"
#elif (defined(CONFIG_MULTI_PARTITION_SUPPORT) && !defined(CONFIG_GUEST_PARTITION))
#define FM_MASTER_PARTITION
#endif /* defined (CONFIG_GUEST_PARTITION) && ... */

/**************************************************************************//**
 @Collection   Defines used for enabling/disabling FM interrupts

 @{
*//***************************************************************************/

typedef uint32_t t_FmBlockErrIntrEnable;

#define ERR_INTR_EN_DMA         0x00010000
#define ERR_INTR_EN_FPM         0x80000000
#define ERR_INTR_EN_BMI         0x00800000
#define ERR_INTR_EN_QMI         0x00400000
#define ERR_INTR_EN_PRS         0x00200000
#define ERR_INTR_EN_KG          0x00100000
#define ERR_INTR_EN_PLCR        0x00080000
#define ERR_INTR_EN_MURAM       0x00040000
#define ERR_INTR_EN_IRAM        0x00020000
#define ERR_INTR_EN_10G_MAC0    0x00008000
#define ERR_INTR_EN_1G_MAC0     0x00004000
#define ERR_INTR_EN_1G_MAC1     0x00002000
#define ERR_INTR_EN_1G_MAC2     0x00001000
#define ERR_INTR_EN_1G_MAC3     0x00000800

typedef uint32_t t_FmBlockIntrEnable;

#define INTR_EN_BMI             0x80000000
#define INTR_EN_QMI             0x40000000
#define INTR_EN_PRS             0x20000000
#define INTR_EN_KG              0x10000000
#define INTR_EN_PLCR            0x08000000
#define INTR_EN_REV0            0x00008000
#define INTR_EN_REV1            0x00004000
#define INTR_EN_REV2            0x00002000
#define INTR_EN_REV3            0x00001000
#define INTR_EN_BRK             0x00000080
#define INTR_EN_TMR             0x01000000
#define INTR_EN_1G_MAC0_TMR     0x00080000
#define INTR_EN_1G_MAC1_TMR     0x00040000
#define INTR_EN_1G_MAC2_TMR     0x00020000
#define INTR_EN_1G_MAC3_TMR     0x00010000
#define INTR_EN_1G_MAC1         0x00400000
#define INTR_EN_1G_MAC2         0x00200000
#define INTR_EN_1G_MAC3         0x00100000

/* @} */


#define PCD_MAX_NUM_OF_PORTS        (FM_MAX_NUM_OF_OH_PORTS +       \
                                     FM_MAX_NUM_OF_1G_RX_PORTS +    \
                                     FM_MAX_NUM_OF_10G_RX_PORTS)
#define MODULE_NAME_SIZE            30
#define DUMMY_PORT_ID               0

#define FM_LIODN_OFFSET_MASK        0x3FF
/**************************************************************************//**
  @Description       NIA Description
*//***************************************************************************/

#define NIA_ORDER_RESTOR            0x00800000
#define NIA_ENG_FM_CTL              0x00000000
#define NIA_ENG_PRS                 0x00440000
#define NIA_ENG_KG                  0x00480000
#define NIA_ENG_PLCR                0x004C0000
#define NIA_ENG_BMI                 0x00500000
#define NIA_ENG_QMI_ENQ             0x00540000
#define NIA_ENG_QMI_DEQ             0x00580000
#define NIA_ENG_MASK                0x007C0000

#define NIA_FM_CTL_AC_CC            0x00000006
#define NIA_FM_CTL_AC_HC            0x0000000C
#define NIA_FM_CTL_AC_IND_MODE_TX   0x00000008
#define NIA_FM_CTL_AC_IND_MODE_RX   0x0000000A

#define NIA_BMI_AC_ENQ_FRAME        0x00000002
#define NIA_BMI_AC_TX_RELEASE       0x000002C0
#define NIA_BMI_AC_RELEASE          0x000000C0
#define NIA_BMI_AC_DISCARD          0x000000C1
#define NIA_BMI_AC_TX               0x00000274
#define NIA_BMI_AC_FETCH            0x00000208
#define NIA_BMI_AC_MASK             0x000003FF

#define NIA_KG_DIRECT               0x00000100
#define NIA_KG_CC_EN                0x00000200
#define NIA_PLCR_ABSOLUTE           0x00008000

/**************************************************************************//**
 @Description       Port Id defines
*//***************************************************************************/
#define BASE_HO_PORTID              1
#define BASE_RX_PORTID              8
#define BASE_RX10_PORTID            0x10
#define BASE_TX_PORTID              0x28
#define BASE_TX10_PORTID            0x30
#define LAST_HO_PORTID              7
#define LAST_RX_PORTID              0xB
#define LAST_TX_PORTID              0x2B
#define LAST_RX10_PORTID            0x10
#define LAST_TX10_PORTID            0x30

#define GET_GLOBAL_PORTID(port, type, id)               \
switch(type){                                           \
    case(e_FM_PORT_TYPE_OH_OFFLINE_PARSING):            \
    case(e_FM_PORT_TYPE_OH_HOST_COMMAND):               \
        port = (uint8_t)(BASE_HO_PORTID + id);          \
        if (id > (LAST_HO_PORTID-BASE_HO_PORTID))       \
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("Illegal port id")); \
        break;                                          \
    case(e_FM_PORT_TYPE_RX):                            \
        port = (uint8_t)(BASE_RX_PORTID + id);          \
        if (id > (LAST_RX_PORTID-BASE_RX_PORTID))       \
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("Illegal port id")); \
        break;                                          \
    case(e_FM_PORT_TYPE_RX_10G):                        \
        port = (uint8_t)(BASE_RX10_PORTID + id);        \
        if (id > (LAST_RX10_PORTID-BASE_RX10_PORTID))   \
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("Illegal port id")); \
        break;                                          \
    case(e_FM_PORT_TYPE_TX):                            \
        port = (uint8_t)(BASE_TX_PORTID + id);          \
        if (id > (LAST_TX_PORTID-BASE_TX_PORTID))       \
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("Illegal port id")); \
        break;                                          \
    case(e_FM_PORT_TYPE_TX_10G):                        \
        port = (uint8_t)(BASE_TX10_PORTID + id);        \
        if (id > (LAST_TX10_PORTID-BASE_TX10_PORTID))   \
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("Illegal port id")); \
        break;                                          \
    default:                                            \
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("Illegal port type")); \
        break;                                          \
}

#define GET_RELATIVE_PORTID(relativePortId, hardwarePortId)                           \
{   if ((hardwarePortId>=BASE_HO_PORTID) && (hardwarePortId<=LAST_HO_PORTID))           \
        relativePortId = (uint8_t)(hardwarePortId-BASE_HO_PORTID);                    \
    else if ((hardwarePortId>=BASE_TX10_PORTID) && (hardwarePortId<=LAST_TX10_PORTID))  \
        relativePortId = (uint8_t)(hardwarePortId-BASE_TX10_PORTID);                  \
    else if ((hardwarePortId>=BASE_TX_PORTID) && (hardwarePortId<=LAST_TX_PORTID))      \
        relativePortId = (uint8_t)(hardwarePortId-BASE_TX_PORTID);                    \
    else if ((hardwarePortId>=BASE_RX10_PORTID) && (hardwarePortId<=LAST_RX10_PORTID))  \
        relativePortId = (uint8_t)(hardwarePortId-BASE_RX10_PORTID);                  \
    else if ((hardwarePortId>=BASE_RX_PORTID) && (hardwarePortId<=LAST_RX_PORTID))      \
        relativePortId = (uint8_t)(hardwarePortId-BASE_RX_PORTID);                    \
    else {                                                                          \
        relativePortId = (uint8_t)DUMMY_PORT_ID;                                    \
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("Illegal port type"));                \
    }                                                                               \
}

#define BMI_FIFO_UNITS                      0x100
#define BMI_MAX_FIFO_SIZE                   (160*KILOBYTE)

typedef struct
{
    void        (*f_Isr) (t_Handle h_Arg);
    t_Handle    h_SrcHandle;
#ifdef FM_MASTER_PARTITION
    uint8_t     partitionId;
#endif
} t_FmIntrSrc;

#define ILLEGAL_HDR_NUM                         0xFF
#define NO_HDR_NUM                              FM_PCD_PRS_NUM_OF_HDRS

#define GET_PRS_HDR_NUM(num, hdr)                           \
switch(hdr)                                                 \
{   case(HEADER_TYPE_ETH):              num = 0;  break;    \
    case(HEADER_TYPE_LLC_SNAP):         num = 1;  break;    \
    case(HEADER_TYPE_VLAN):             num = 2;  break;    \
    case(HEADER_TYPE_PPPoE):            num = 3;  break;    \
    case(HEADER_TYPE_MPLS):             num = 4;  break;    \
    case(HEADER_TYPE_IPv4):             num = 5;  break;    \
    case(HEADER_TYPE_IPv6):             num = 6;  break;    \
    case(HEADER_TYPE_GRE):              num = 7;  break;    \
    case(HEADER_TYPE_MINENCAP):         num = 8;  break;    \
    case(HEADER_TYPE_USER_DEFINED_L3):  num = 9;  break;    \
    case(HEADER_TYPE_TCP):              num = 10; break;    \
    case(HEADER_TYPE_UDP):              num = 11; break;    \
    case(HEADER_TYPE_IPSEC_AH):                             \
    case(HEADER_TYPE_IPSEC_ESP):        num = 12; break;    \
    case(HEADER_TYPE_SCTP):             num = 13; break;    \
    case(HEADER_TYPE_DCCP):             num = 14; break;    \
    case(HEADER_TYPE_USER_DEFINED_L4):  num = 15; break;    \
    case(HEADER_TYPE_USER_DEFINED_SHIM1):                   \
    case(HEADER_TYPE_USER_DEFINED_SHIM2):                   \
    case(HEADER_TYPE_USER_DEFINED_SHIM3):                   \
        num = NO_HDR_NUM; break;                            \
    default:                                                \
        REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Unsupported header for parser"));\
        num = ILLEGAL_HDR_NUM; break;                       \
}

/***********************************************************************/
/*          Policer defines                                            */
/***********************************************************************/
#define FM_PCD_PLCR_PAR_GO                    0x80000000
#define FM_PCD_PLCR_PAR_PWSEL_MASK            0x0000FFFF
#define FM_PCD_PLCR_PAR_R                     0x40000000

/* shifts */
#define FM_PCD_PLCR_PAR_PNUM_SHIFT            16


/***********************************************************************/
/*          Keygen defines                                             */
/***********************************************************************/
/* maskes */
#define KG_SCH_PP_SHIFT_HIGH                    0x80000000
#define KG_SCH_PP_SHIFT_LOW                     0x0000F000
#define KG_SCH_MODE_NIA_PLCR                    0x40000000
#define KG_SCH_GEN_EXTRACT_TYPE                 0x00008000
#define KG_SCH_BITMASK_MASK                     0x000000FF
#define KG_SCH_GEN_VALID                        0x80000000
#define KG_SCH_GEN_MASK                         0x00FF0000
#define FM_PCD_KG_KGAR_ERR                      0x20000000
#define FM_PCD_KG_KGAR_SEL_CLS_PLAN_ENTRY       0x01000000
#define FM_PCD_KG_KGAR_SEL_PORT_ENTRY           0x02000000
#define FM_PCD_KG_KGAR_SEL_PORT_WSEL_SP         0x00008000
#define FM_PCD_KG_KGAR_SEL_PORT_WSEL_CPP        0x00004000
#define FM_PCD_KG_KGAR_WSEL_MASK                0x0000FF00

#define FM_PCD_KG_KGAR_GO                       0x80000000
#define FM_PCD_KG_KGAR_READ                     0x40000000
#define FM_PCD_KG_KGAR_WRITE                    0x00000000
#define FM_PCD_KG_KGAR_SEL_SCHEME_ENTRY         0x00000000
#define FM_PCD_KG_KGAR_SCHEME_WSEL_UPDATE_CNT   0x00008000


typedef uint32_t t_KnownFieldsMasks;

#define KG_SCH_KN_PORT_ID                   0x80000000
#define KG_SCH_KN_MACDST                    0x40000000
#define KG_SCH_KN_MACSRC                    0x20000000
#define KG_SCH_KN_TCI1                      0x10000000
#define KG_SCH_KN_TCI2                      0x08000000
#define KG_SCH_KN_ETYPE                     0x04000000
#define KG_SCH_KN_PPPSID                    0x02000000
#define KG_SCH_KN_PPPID                     0x01000000
#define KG_SCH_KN_MPLS1                     0x00800000
#define KG_SCH_KN_MPLS2                     0x00400000
#define KG_SCH_KN_MPLS_LAST                 0x00200000
#define KG_SCH_KN_IPSRC1                    0x00100000
#define KG_SCH_KN_IPDST1                    0x00080000
#define KG_SCH_KN_PTYPE1                    0x00040000
#define KG_SCH_KN_IPTOS_TC1                 0x00020000
#define KG_SCH_KN_IPV6FL1                   0x00010000
#define KG_SCH_KN_IPSRC2                    0x00008000
#define KG_SCH_KN_IPDST2                    0x00004000
#define KG_SCH_KN_PTYPE2                    0x00002000
#define KG_SCH_KN_IPTOS_TC2                 0x00001000
#define KG_SCH_KN_IPV6FL2                   0x00000800
#define KG_SCH_KN_GREPTYPE                  0x00000400
#define KG_SCH_KN_IPSEC_SPI                 0x00000200
#define KG_SCH_KN_IPSEC_NH                  0x00000100
#define KG_SCH_KN_L4PSRC                    0x00000004
#define KG_SCH_KN_L4PDST                    0x00000002
#define KG_SCH_KN_TFLG                      0x00000001

typedef uint8_t t_GenericCodes;

#define KG_SCH_GEN_SHIM1                       0x70
#define KG_SCH_GEN_DEFAULT                     0x10
#define KG_SCH_GEN_PARSE_RESULT                0x20
#define KG_SCH_GEN_START_OF_FRM                0x40
#define KG_SCH_GEN_SHIM2                       0x71
#define KG_SCH_GEN_SHIM3                       0x72
#define KG_SCH_GEN_ETH                         0x03
#define KG_SCH_GEN_ETH_NO_V                    0x73
#define KG_SCH_GEN_SNAP                        0x04
#define KG_SCH_GEN_SNAP_NO_V                   0x74
#define KG_SCH_GEN_VLAN1                       0x05
#define KG_SCH_GEN_VLAN1_NO_V                  0x75
#define KG_SCH_GEN_VLAN2                       0x06
#define KG_SCH_GEN_VLAN2_NO_V                  0x76
#define KG_SCH_GEN_ETH_TYPE                    0x07
#define KG_SCH_GEN_ETH_TYPE_NO_V               0x77
#define KG_SCH_GEN_PPP                         0x08
#define KG_SCH_GEN_PPP_NO_V                    0x78
#define KG_SCH_GEN_MPLS1                       0x09
#define KG_SCH_GEN_MPLS2                       0x19
#define KG_SCH_GEN_MPLS3                       0x29
#define KG_SCH_GEN_MPLS1_NO_V                  0x79
#define KG_SCH_GEN_MPLS_LAST                   0x0a
#define KG_SCH_GEN_MPLS_LAST_NO_V              0x7a
#define KG_SCH_GEN_IPV4                        0x0b
#define KG_SCH_GEN_IPV6                        0x1b
#define KG_SCH_GEN_L3_NO_V                     0x7b
#define KG_SCH_GEN_IPV4_TUNNELED               0x0c
#define KG_SCH_GEN_IPV6_TUNNELED               0x1c
#define KG_SCH_GEN_MIN_ENCAP                   0x2c
#define KG_SCH_GEN_IP2_NO_V                    0x7c
#define KG_SCH_GEN_GRE                         0x0d
#define KG_SCH_GEN_GRE_NO_V                    0x7d
#define KG_SCH_GEN_TCP                         0x0e
#define KG_SCH_GEN_UDP                         0x1e
#define KG_SCH_GEN_IPSEC_AH                    0x2e
#define KG_SCH_GEN_SCTP                        0x3e
#define KG_SCH_GEN_DCCP                        0x4e
#define KG_SCH_GEN_IPSEC_ESP                   0x6e
#define KG_SCH_GEN_L4_NO_V                     0x7e
#define KG_SCH_GEN_NEXTHDR                     0x7f

/* shifts */
#define KG_SCH_PP_SHIFT_HIGH_SHIFT          27
#define KG_SCH_PP_SHIFT_LOW_SHIFT           12
#define KG_SCH_PP_MASK_SHIFT                16
#define KG_SCH_MODE_CCOBASE_SHIFT           24
#define KG_SCH_DEF_MAC_ADDR_SHIFT           30
#define KG_SCH_DEF_TCI_SHIFT                28
#define KG_SCH_DEF_ENET_TYPE_SHIFT          26
#define KG_SCH_DEF_PPP_SESSION_ID_SHIFT     24
#define KG_SCH_DEF_PPP_PROTOCOL_ID_SHIFT    22
#define KG_SCH_DEF_MPLS_LABEL_SHIFT         20
#define KG_SCH_DEF_IP_ADDR_SHIFT            18
#define KG_SCH_DEF_PROTOCOL_TYPE_SHIFT      16
#define KG_SCH_DEF_IP_TOS_TC_SHIFT          14
#define KG_SCH_DEF_IPV6_FLOW_LABEL_SHIFT    12
#define KG_SCH_DEF_IPSEC_SPI_SHIFT          10
#define KG_SCH_DEF_L4_PORT_SHIFT            8
#define KG_SCH_DEF_TCP_FLAG_SHIFT           6
#define KG_SCH_HASH_CONFIG_SHIFT_SHIFT      24
#define KG_SCH_GEN_MASK_SHIFT               16
#define KG_SCH_GEN_HT_SHIFT                 8
#define KG_SCH_GEN_SIZE_SHIFT               24
#define KG_SCH_GEN_DEF_SHIFT                29
#define FM_PCD_KG_KGAR_NUM_SHIFT            16


/* others */
#define NUM_OF_SW_DEFAULTS                  3
#define MAX_PP_SHIFT                        15
#define MAX_KG_SCH_SIZE                     16
#define MASK_FOR_GENERIC_BASE_ID            0x20
#define MAX_HASH_SHIFT                      40
#define MAX_KG_SCH_BIT_OFFSET               23
#define MAX_DIST_FQID_SHIFT                 23

#define GET_MASK_SEL_SHIFT(shift,i)             \
switch(i) {                                     \
    case(0):shift = 26;break;                   \
    case(1):shift = 20;break;                   \
    case(2):shift = 10;break;                   \
    case(3):shift = 4;break;                    \
    default:                                    \
    RETURN_ERROR(MAJOR, E_INVALID_VALUE, NO_MSG);\
}

#define GET_MASK_OFFSET_SHIFT(shift,i)          \
switch(i) {                                     \
    case(0):shift = 16;break;                   \
    case(1):shift = 0;break;                    \
    case(2):shift = 28;break;                   \
    case(3):shift = 24;break;                   \
    default:                                    \
    RETURN_ERROR(MAJOR, E_INVALID_VALUE, NO_MSG);\
}

#define GET_MASK_SHIFT(shift,i)                 \
switch(i) {                                     \
    case(0):shift = 24;break;                   \
    case(1):shift = 16;break;                   \
    case(2):shift = 8;break;                    \
    case(3):shift = 0;break;                    \
    default:                                    \
    RETURN_ERROR(MAJOR, E_INVALID_VALUE, NO_MSG);\
}


typedef struct {
    uint16_t num;
    uint8_t  hardwarePortId;
    uint16_t plcrProfilesBase;
} t_FmPortPcdInterModulePlcrParams;


uint32_t    FmPcdGetSwPrsOffset(t_Handle h_FmPcd, e_NetHeaderType hdr, uint8_t  indexPerHdr);
uint32_t    FmPcdGetLcv(t_Handle h_FmPcd, uint32_t netEnvId, uint8_t hdrNum);
void        FmPcdIncNetEnvOwners(t_Handle h_FmPcd, uint8_t netEnvId);
void        FmPcdDecNetEnvOwners(t_Handle h_FmPcd, uint8_t netEnvId);
void        FmPcdPortRegister(t_Handle h_FmPcd, t_Handle h_FmPort, uint8_t hardwarePortId);
t_Error     FmPcdTryLock(t_Handle h_FmPcd);
void        FmPcdReleaseLock(t_Handle h_FmPcd);

t_Error     FmPcdCcReleaseModifiedOnlyNextEngine(t_Handle h_FmPcd, t_Handle h_FmPcdOldPointer, t_Handle h_FmPcdNewPointer, bool isAllGood);
t_Error     FmPcdCcReleaseModifiedKey(t_Handle h_FmPcd, t_List *h_FmPcdOldPointersLst, t_Handle h_FmPcdNewPointer, uint16_t numOfGoodChanges);
uint32_t    FmPcdCcGetNodeAddrOffset(t_Handle h_FmPcd, t_Handle h_Pointer);
t_Error     FmPcdCcRemoveKey(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex,t_List *h_OldLst, t_Handle *h_NewPointer);
t_Error     FmPcdCcAddKey(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex, t_FmPcdCcKeyParams *p_FmPCdCcKeyParams,  t_List *h_OldLst, t_Handle *h_NewPointer);
t_Error     FmPcdCcModifyKey(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex, uint8_t *p_Key, uint8_t *p_Mask, t_List *h_OldLst,  t_Handle *h_NewPointer);
t_Error     FmPcdCcModifyKeyAndNextEngine(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex, t_FmPcdCcKeyParams *p_FmPcdCcKeyParams, t_List *h_OldLst, t_Handle *h_NewPointer);
t_Error     FmPcdCcModifyMissNextEngineParamNode(t_Handle h_FmPcd, t_Handle h_CcNode, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams,t_Handle *h_OldPointer, t_Handle *h_NewPointer);
t_Error     FmPcdCcModifyNextEngineParamTree(t_Handle h_FmPcd, t_Handle h_CcTree, uint8_t grpId, uint8_t index, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams, t_Handle *h_OldPointer, t_Handle *h_NewPointer);
t_Error     FmPcdCcModiyNextEngineParamNode(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex,t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams,t_Handle *h_OldPointer, t_Handle *h_NewPointer);
uint32_t    FmPcdCcGetNodeAddrOffsetFromNodeInfo(t_Handle h_FmPcd, t_Handle h_Pointer);
t_Error     FmPcdCcTreeTryLock(t_Handle h_FmPcdCcTree);
t_Error     FmPcdCcNodeTreeTryLock(t_Handle h_FmPcd,t_Handle h_FmPcdCcNode, t_List *p_List);
void        FmPcdCcTreeReleaseLock(t_Handle h_FmPcdCcTree);
void        FmPcdCcNodeTreeReleaseLock(t_Handle h_FmPcd, t_List *p_List);

bool        FmPcdKgIsSchemeValidSw(t_Handle h_FmPcd, uint8_t schemeId);
uint8_t     FmPcdKgGetClsPlanGrpBase(t_Handle h_FmPcd, uint8_t clsPlanGrp);
uint16_t    FmPcdKgGetClsPlanGrpSize(t_Handle h_FmPcd, uint8_t clsPlanGrp);
t_Error     FmPcdKgSwBindPortToClsPlanGrp(t_Handle h_FmPcd, uint8_t netEnvId, uint8_t clsPlanGrpId, protocolOpt_t *p_OptArray);
void        FmPcdKgSwUnbindPortToClsPlanGrp(t_Handle h_FmPcd, uint8_t clsPlanGrpId);
bool        FmPcdKgIsEmptyClsPlanGrp(t_Handle h_FmPcd);
uint8_t     FmPcdKgGetEmptyClsPlanGrpId(t_Handle h_FmPcd);
t_Error     FmPcdKgBuildScheme(t_Handle h_FmPcd,  t_FmPcdKgSchemeParams *p_Scheme, t_FmPcdKgInterModuleSchemeRegs *p_SchemeRegs);
t_Handle    FmPcdKgBuildClsPlanGrp(t_Handle h_FmPcd, t_FmPcdKgClsPlanGrpParams *p_Grp, t_FmPcdKgInterModuleClsPlanSet *p_ClsPlanSet);
uint8_t     FmPcdKgGetNumOfPartitionSchemes(t_Handle h_FmPcd);
uint8_t     FmPcdKgGetPhysicalSchemeId(t_Handle h_FmPcd, uint8_t schemeId);
uint8_t     FmPcdKgGetRelativeSchemeId(t_Handle h_FmPcd, uint8_t schemeId);
void        FmPcdKgDestroyClsPlanGrp(t_Handle h_FmPcd, uint8_t grpId);
void        FmPcdKgValidateSchemeSw(t_Handle h_FmPcd, uint8_t schemeId);
void        FmPcdKgInvalidateSchemeSw(t_Handle h_FmPcd, uint8_t schemeId);
t_Error     FmPcdKgCheckInvalidateSchemeSw(t_Handle h_FmPcd, uint8_t schemeId);
t_Error     FmPcdKgBuildBindPortToSchemes(t_Handle h_FmPcd , t_FmPcdKgInterModuleBindPortToSchemes *p_BindPortToSchemes, uint32_t *p_SpReg, bool add);
void        FmPcdKgIncSchemeOwners(t_Handle h_FmPcd , t_FmPcdKgInterModuleBindPortToSchemes *p_BindPort);
void        FmPcdKgDecSchemeOwners(t_Handle h_FmPcd , t_FmPcdKgInterModuleBindPortToSchemes *p_BindPort);
bool        FmPcdKgIsDriverClsPlan(t_Handle h_FmPcd);
bool        FmPcdKgHwSchemeIsValid(uint32_t schemeModeReg);
uint32_t    FmPcdKgBuildCppReg(t_Handle h_FmPcd, uint8_t clsPlanGrpId);
uint32_t    FmPcdKgBuildWriteSchemeActionReg(uint8_t schemeId, bool updateCounter);
uint32_t    FmPcdKgBuildReadSchemeActionReg(uint8_t schemeId);
uint32_t    FmPcdKgBuildWriteClsPlanBlockActionReg(uint8_t grpId);
uint32_t    FmPcdKgBuildReadClsPlanBlockActionReg(uint8_t grpId);
uint32_t    FmPcdKgBuildWritePortSchemeBindActionReg(uint8_t hardwarePortId);
uint32_t    FmPcdKgBuildReadPortSchemeBindActionReg(uint8_t hardwarePortId);
uint32_t    FmPcdKgBuildWritePortClsPlanBindActionReg(uint8_t hardwarePortId);
uint8_t     FmPcdKgGetSchemeSwId(t_Handle h_FmPcd, uint8_t schemeHwId);
t_Error     FmPcdKgSchemeTryLock(t_Handle h_FmPcd, uint8_t schemeId);
void        FmPcdKgReleaseSchemeLock(t_Handle h_FmPcd, uint8_t schemeId);

t_Error     FmPcdKgBindPortToSchemes(t_Handle h_FmPcd , t_FmPcdKgInterModuleBindPortToSchemes  *p_SchemeBind);
t_Error     FmPcdKgUnbindPortToSchemes(t_Handle h_FmPcd , t_FmPcdKgInterModuleBindPortToSchemes *p_SchemeBind);
t_Error     FmPcdKgBindPortToClsPlanGrp(t_Handle p_FmPcd, uint8_t hardwarePortId, uint8_t clsPlanGrpId);
void        FmPcdKgUnbindPortToClsPlanGrp(t_Handle h_FmPcd, uint8_t hardwarePortId);

#ifndef CONFIG_MULTI_PARTITION_SUPPORT
void        FmPcdPrsIncludePortInStatistics(t_Handle p_FmPcd, uint8_t hardwarePortId,  bool include);
#endif /* !CONFIG_MULTI_PARTITION_SUPPORT */

t_Error     FmPcdPlcrAllocProfiles(t_Handle h_FmPcd, uint8_t hardwarePortId, uint16_t numOfProfiles);
t_Error     FmPcdPlcrFreeProfiles(t_Handle h_FmPcd, uint8_t hardwarePortId);
bool        FmPcdPlcrIsProfileValid(t_Handle h_FmPcd, uint16_t absoluteProfileId);
uint16_t    FmPcdPlcrGetPortProfilesBase(t_Handle h_FmPcd, uint8_t hardwarePortId);
uint16_t    FmPcdPlcrGetPortNumOfProfiles(t_Handle h_FmPcd, uint8_t hardwarePortId);
uint32_t    FmPcdPlcrBuildWritePlcrActionRegs(uint16_t absoluteProfileId);
uint32_t    FmPcdPlcrBuildCounterProfileReg(e_FmPcdPlcrProfileCounters counter);
uint32_t    FmPcdPlcrBuildWritePlcrActionReg(uint16_t absoluteProfileId);
uint32_t    FmPcdPlcrBuildReadPlcrActionReg(uint16_t absoluteProfileId);
t_Error     FmPcdPlcrBuildProfile(t_Handle h_FmPcd, t_FmPcdPlcrProfileParams *p_Profile, t_FmPcdPlcrInterModuleProfileRegs *p_PlcrRegs);
t_Error     FmPcdPlcrGetAbsoluteProfileId(t_Handle h_FmPcd,
                                e_FmPcdProfileTypeSelection profileType,
                                t_Handle  h_FmPort,
                                uint16_t relativeProfile,
                                uint16_t *p_AbsoluteId);
void        FmPcdPlcrInvalidateProfileSw(t_Handle h_FmPcd, uint16_t absoluteProfileId);
void        FmPcdPlcrValidateProfileSw(t_Handle h_FmPcd, uint16_t absoluteProfileId);
bool        FmPcdPlcrHwProfileIsValid(uint32_t profileModeReg);
t_Error     FmPcdPlcrProfileTryLock(t_Handle h_FmPcd, uint16_t profileId);
void        FmPcdPlcrReleaseProfileLock(t_Handle h_FmPcd, uint16_t profileId);

t_Error     FmPcdCcBindTree(t_Handle h_FmPcd, t_Handle h_CcTree,  uint32_t  *p_Offset);
t_Error     FmPcdCcUnbindTree(t_Handle h_FmPcd, t_Handle h_CcTree);

#if (defined(CONFIG_MULTI_PARTITION_SUPPORT) && !defined(CONFIG_GUEST_PARTITION))
t_Error     FmPcdHandleIpcMsg(t_Handle h_FmPcd, uint32_t msgId, uint8_t msgBody[MSG_BODY_SIZE]);
#endif /* (defined(CONFIG_MULTI_PARTITION_SUPPORT) && ... */


t_Error     FmPortSetPcd(t_Handle h_FmPort, t_FmPortPcdParams *p_PcdParams);
t_Error     FmPortDeletePcd(t_Handle h_FmPort);
uint8_t     FmPortGetNetEnvId(t_Handle h_FmPort);
uint8_t     FmPortGetHardwarePortId(t_Handle h_FmPort);
uint32_t    FmPortGetPcdEngines(t_Handle h_FmPort);
uint8_t     FmPortGetClsPlanId(t_Handle h_FmPort);
t_Error     FmPortPcdKgSwBindClsPlanGrp (t_Handle h_FmPort, bool useClsPlan, uint8_t newClsPlanGrpId);
void        FmPortPcdKgSwUnbindClsPlanGrp (t_Handle h_FmPort);
t_Error     FmPortAttachPCD(t_Handle h_FmPort);


#ifdef CONFIG_MULTI_PARTITION_SUPPORT
uint8_t     FmGetPartitionId(t_Handle h_Fm);
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
/**************************************************************************//**
 @Function      FmRegisterIntr

 @Description   Used to register an inter-module event handler to be processed by FM

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     mod             The module that causes the event
 @Param[in]     modId           Module id - if more than 1 instansiation of this
                                mode exists,0 otherwise.
 @Param[in]     intrType        Interrupt type (error/normal) selection.
 @Param[in]     f_Isr           The interrupt service routine.
 @Param[in]     h_Arg           Argument to be passed to f_Isr.

 @Return        None.
*//***************************************************************************/
void FmRegisterIntr(t_Handle                h_Fm,
                     e_FmEventModules       mod,
                     uint8_t                modId,
                     e_FmIntrType           intrType,
                     void                   (*f_Isr) (t_Handle h_Arg),
                     t_Handle               h_Arg);

/**************************************************************************//**
 @Function      FmUnregisterIntr

 @Description   Used to un-register an inter-module event handler that was processed by FM

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     mod             The module that causes the event
 @Param[in]     modId           Module id - if more than 1 instansiation of this
                                mode exists,0 otherwise.
 @Param[in]     intrType        Interrupt type (error/normal) selection.

 @Return        None.
*//***************************************************************************/
void FmUnregisterIntr(t_Handle                h_Fm,
                     e_FmEventModules       mod,
                     uint8_t                modId,
                     e_FmIntrType           intrType);

/**************************************************************************//**
 @Function      FmRegisterFmCtlIntr

 @Description   Used to register to one of the fmCtl events in the FM module

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     eventRegId      FmCtl event id (0-7).
 @Param[in]     f_Isr           The interrupt service routine.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
void  FmRegisterFmCtlIntr(t_Handle h_Fm, uint8_t eventRegId, void (*f_Isr) (t_Handle h_Fm, uint32_t event));

/**************************************************************************//**
 @Description   enum for defining MAC types
*//***************************************************************************/
typedef enum e_FmMacType {
    e_FM_MAC_10G,               /**< 10G MAC */
    e_FM_MAC_1G                 /**< 1G MAC */
} e_FmMacType;

/**************************************************************************//**
 @Description   Structure for port-FM communication during FM_PORT_Init.
                Fields commented 'IN' are passed by the port module to be used
                by the FM module.
                Fields commented 'OUT' will be filled by FM before returning to port.
                Some fields are optional (depending on configuration) and
                will be analized by the port and FM modules accordingly.
*//***************************************************************************/
typedef struct t_FmInterModulePortInitParams {
    uint8_t             hardwarePortId;     /**< IN. port Id */
    e_FmPortType        portType;           /**< IN. Port type */
    bool                useRateLimit;       /**< IN. AN indication whether rate limit
                                                 is used, to enable timestamp. */
    uint8_t             count1MicroBit;     /**< OUT. an indication of which TS bit
                                                 counts closest to 1 Microsecond. */
    bool                independentMode;    /**< IN. TRUE if FM Port operates in independent mode */
    uint16_t            portPartition;      /**< IN. Port's requested resource */
    uint8_t             numOfTasks;         /**< IN. Port's requested resource */
    uint8_t             numOfExtraTasks;    /**< IN. Port's requested resource */
    uint8_t             numOfOpenDmas;      /**< IN. Port's requested resource */
    uint8_t             numOfExtraOpenDmas; /**< IN. Port's requested resource */
    uint32_t            sizeOfFifo;         /**< IN. Port's requested resource */
    uint32_t            extraSizeOfFifo;    /**< IN. Port's requested resource */
    uint8_t             deqPipelineDepth;   /**< IN. Port's requested resource */
    t_FmPhysAddr        fmMuramPhysBaseAddr;/**< OUT. FM-MURAM physical address*/
} t_FmInterModulePortInitParams;

/**************************************************************************//**
 @Description   Structure for port-FM communication during FM_PORT_Free.
*//***************************************************************************/
typedef struct t_FmInterModulePortFreeParams {
    uint8_t             hardwarePortId;     /**< IN. port Id */
    e_FmPortType        portType;           /**< IN. Port type */
    uint8_t             deqPipelineDepth;   /**< IN. Port's requested resource */
} t_FmInterModulePortFreeParams;

/**************************************************************************//**
 @Function      FmGetPcdPrsBaseAddr

 @Description   Get the base address of the Parser from the FM module

 @Param[in]     h_Fm            A handle to an FM Module.

 @Return        Base address.
*//***************************************************************************/
uint64_t FmGetPcdPrsBaseAddr(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FmGetPcdKgBaseAddr

 @Description   Get the base address of the Keygen from the FM module

 @Param[in]     h_Fm            A handle to an FM Module.

 @Return        Base address.
*//***************************************************************************/
uint64_t FmGetPcdKgBaseAddr(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FmGetPcdPlcrBaseAddr

 @Description   Get the base address of the Policer from the FM module

 @Param[in]     h_Fm            A handle to an FM Module.

 @Return        Base address.
*//***************************************************************************/
uint64_t FmGetPcdPlcrBaseAddr(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FmGetMuramHandle

 @Description   Get the handle of the MURAM from the FM module

 @Param[in]     h_Fm            A handle to an FM Module.

 @Return        MURAM module handle.
*//***************************************************************************/
t_Handle FmGetMuramHandle(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FmGetPhysicalMuramBase

 @Description   Get the physical base address of the MURAM from the FM module

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     fmPhysAddr      Physical MURAM base

 @Return        Physical base address.
*//***************************************************************************/
t_Error FmGetPhysicalMuramBase(t_Handle h_Fm, t_FmPhysAddr *fmPhysAddr);

/**************************************************************************//**
 @Function      FmGetTimeStampScale

 @Description   Used internally by other modules in order to get the timeStamp
                period as requested by the application.

 @Param[in]     h_Fm                    A handle to an FM Module.

 @Return        TimeStamp period in nanoseconds.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
uint32_t    FmGetTimeStampScale(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FmResumeStalledPort

 @Description   Used internally by FM port to release a stalled port.

 @Param[in]     h_Fm                            A handle to an FM Module.
 @Param[in]     hardwarePortId                    HW port id.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
t_Error FmResumeStalledPort(t_Handle h_Fm, uint8_t hardwarePortId);

/**************************************************************************//**
 @Function      FmIsPortStalled

 @Description   Used internally by FM port to read the port's status.

 @Param[in]     h_Fm                            A handle to an FM Module.
 @Param[in]     hardwarePortId                    HW port id.

 @Return        TRUE if port is stalled, FALSE otherwize

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
bool FmIsPortStalled(t_Handle h_Fm, uint8_t hardwarePortId);

/**************************************************************************//**
 @Function      FmResetMac

 @Description   Used by MAC driver to reset the MAC registers

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in]     type            MAC type.
 @Param[in]     macId           MAC id - according to type.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
t_Error FmResetMac(t_Handle h_Fm, e_FmMacType type, uint8_t macId);

/**************************************************************************//**
 @Function      FmGetClockFreq

 @Description   Used by MAC driver to get the FM clock frequency

 @Param[in]     h_Fm            A handle to an FM Module.

 @Return        clock-freq on success; 0 otherwise.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
uint16_t FmGetClockFreq(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FmGetId

 @Description   Used by PCD driver to read rhe FM id

 @Param[in]     h_Fm            A handle to an FM Module.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
uint8_t FmGetId(t_Handle h_Fm);

/**************************************************************************//**
 @Function      FmGetSetPortParams

 @Description   Used by FM-PORT driver to pass and receive parameters between
                PORT and FM modules.

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in,out] p_PortParams    A structure of FM Port parameters.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
t_Error FmGetSetPortParams(t_Handle h_Fm,t_FmInterModulePortInitParams *p_PortParams);

/**************************************************************************//**
 @Function      FmFreePortParams

 @Description   Used by FM-PORT driver to free port's resources within the FM.

 @Param[in]     h_Fm            A handle to an FM Module.
 @Param[in,out] p_PortParams    A structure of FM Port parameters.

 @Return        None.

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
void FmFreePortParams(t_Handle h_Fm,t_FmInterModulePortFreeParams *p_PortParams);

#if (defined(CONFIG_MULTI_PARTITION_SUPPORT) && !defined(CONFIG_GUEST_PARTITION))
t_Error     FmHandleIpcMsg(t_Handle h_Fm, uint32_t msgId, uint8_t msgBody[MSG_BODY_SIZE]);
#endif /* (defined(CONFIG_MULTI_PARTITION_SUPPORT) && ... */
void        FmRegisterPcd(t_Handle h_Fm, t_Handle h_FmPcd);
void        FmUnregisterPcd(t_Handle h_Fm);
t_Handle    FmGetPcdHandle(t_Handle h_Fm);
bool        FmRamsEccIsExternalCtl(t_Handle h_Fm);
t_Error     FmEnableRamsEcc(t_Handle h_Fm);
t_Error     FmDisableRamsEcc(t_Handle h_Fm);
void        FmGetRevision(t_Handle h_Fm, t_FmRevisionInfo *p_FmRevisionInfo);


#endif /* __FM_COMMON_H */
