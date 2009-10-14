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

#include "std_ext.h"
#include "error_ext.h"
#include "sprint_ext.h"
#include "string_ext.h"

#include "fm_common.h"
#include "fm_hc.h"


#define __ERR_MODULE__  MODULE_FM

#define SIZE_OF_HC_FRAME_PORT_REGS          (sizeof(t_HcFrame)-sizeof(t_FmPcdKgInterModuleSchemeRegs)+sizeof(t_FmPcdKgPortRegs))
#define SIZE_OF_HC_FRAME_SCHEME_REGS        sizeof(t_HcFrame)
#define SIZE_OF_HC_FRAME_PROFILES_REGS      (sizeof(t_HcFrame)-sizeof(t_FmPcdKgInterModuleSchemeRegs)+sizeof(t_FmPcdPlcrInterModuleProfileRegs))
#define SIZE_OF_HC_FRAME_PROFILE_CNT        (sizeof(t_HcFrame)-sizeof(t_FmPcdPlcrInterModuleProfileRegs)+sizeof(uint32_t))
#define SIZE_OF_HC_FRAME_READ_OR_CC_DYNAMIC 16

#define BUILD_FD(len)                                                           \
do {                                                                            \
        memset(&fmFd, 0, sizeof(t_FmFD));                                       \
        FM_FD_SET_ADDR(&fmFd, &hcFrame);                                        \
        FM_FD_SET_OFFSET(&fmFd, 0);                                             \
        FM_FD_SET_LENGTH(&fmFd, len);                                           \
} while (0)

#define ENQUEUE_FRM(frm)                                                        \
do {                                                                            \
    uint32_t savedSeqNum = p_FmHc->seqNum;                                      \
    p_FmHc->seqNum = (uint32_t)((p_FmHc->seqNum+1)%32);                         \
    ASSERT_COND(!p_FmHc->wait[savedSeqNum]);                                    \
    p_FmHc->wait[savedSeqNum] = TRUE;                                           \
    DBG(TRACE, ("Send Hc 0x%x , SeqNum %d, fd addr 0x%x, fd offset 0x%x",       \
            p_FmHc,savedSeqNum,FM_FD_GET_ADDR(frm),FM_FD_GET_OFFSET(frm)));     \
    err = p_FmHc->f_QmEnqueueCB(p_FmHc->h_QmArg, p_FmHc->enqFqid, (void *)frm); \
    if(err)                                                                     \
        RETURN_ERROR(MINOR, err, ("HC enqueue failed"));                        \
    while (p_FmHc->wait[savedSeqNum]) ;                                         \
} while (0)

#define ENQUEUE_FRM_RET_NULL(frm)                                               \
do {                                                                            \
    uint32_t savedSeqNum = p_FmHc->seqNum;                                      \
    p_FmHc->seqNum = (uint32_t)((p_FmHc->seqNum+1)%32);                         \
    ASSERT_COND(!p_FmHc->wait[savedSeqNum]);                                    \
    p_FmHc->wait[savedSeqNum] = TRUE;                                           \
    DBG(TRACE, ("Send Hc Null 0x%x , SeqNum %d, fd addr 0x%x, fd offset 0x%x",  \
            p_FmHc,savedSeqNum,FM_FD_GET_ADDR(frm),FM_FD_GET_OFFSET(frm)));     \
    err = p_FmHc->f_QmEnqueueCB(p_FmHc->h_QmArg, p_FmHc->enqFqid, (void *)frm); \
    if(err)  {                                                                  \
        REPORT_ERROR(MINOR, err, ("HC enqueue failed")); return NULL;           \
    }                                                                           \
    while (p_FmHc->wait[savedSeqNum]) ;                                         \
} while (0)

#define TRY_LOCK                                                                \
do {                                                                            \
    uint32_t intFlags;                                                          \
    intFlags = XX_DisableAllIntr();                                             \
    if (p_FmHc->lock)                                                           \
    {                                                                           \
        XX_RestoreAllIntr(intFlags);                                            \
        return ERROR_CODE(E_BUSY);                                              \
    }                                                                           \
    p_FmHc->lock = TRUE;                                                        \
    XX_RestoreAllIntr(intFlags);                                                \
} while (0)

#define TRY_LOCK_RETURN_NULL                                                    \
do {                                                                            \
    uint32_t intFlags;                                                          \
    intFlags = XX_DisableAllIntr();                                             \
    if (p_FmHc->lock)                                                           \
    {                                                                           \
        XX_RestoreAllIntr(intFlags);                                            \
        REPORT_ERROR(MINOR, E_BUSY, ("nested host-commands!"));                 \
        return NULL;                                                            \
    }                                                                           \
    p_FmHc->lock = TRUE;                                                        \
    XX_RestoreAllIntr(intFlags);                                                \
} while (0)



#ifdef __MWERKS__
#pragma pack(push,1)
#endif /*__MWERKS__ */
#define MEM_MAP_START

/**************************************************************************//**
 @Description   PCD KG scheme registers
*//***************************************************************************/
typedef _Packed struct t_FmPcdKgSchemeRegsWithoutCounter {
    uint32_t kgse_mode;    /**< MODE */
    uint32_t kgse_ekfc;    /**< Extract Known Fields Command */
    uint32_t kgse_ekdv;    /**< Extract Known Default Value */
    uint32_t kgse_bmch;    /**< Bit Mask Command High */
    uint32_t kgse_bmcl;    /**< Bit Mask Command Low */
    uint32_t kgse_fqb;     /**< Frame Queue Base */
    uint32_t kgse_hc;      /**< Hash Command */
    uint32_t kgse_ppc;     /**< Policer Profile Command */
    uint32_t kgse_gec[FM_PCD_KG_NUM_OF_GENERIC_REGS];
                           /**< Generic Extract Command */
    uint32_t kgse_dv0;     /**< KeyGen Scheme Entry Default Value 0 */
    uint32_t kgse_dv1;     /**< KeyGen Scheme Entry Default Value 1 */
    uint32_t kgse_ccbs;    /**< KeyGen Scheme Entry Coarse Classification Bit*/
    uint32_t kgse_mv;      /**< KeyGen Scheme Entry Match vector */
} _PackedType t_FmPcdKgSchemeRegsWithoutCounter;

#define MEM_MAP_END
#ifdef __MWERKS__
#pragma pack(pop)
#endif /* __MWERKS__ */

typedef struct t_FmPcdKgPortRegs {
        uint32_t                spReg;
        uint32_t                cppReg;
} t_FmPcdKgPortRegs;

typedef struct t_HcFrame {
    uint32_t                    opcode;
    uint32_t                    actionReg;
    uint32_t                    extraReg;
    uint32_t                    commandSequence;
    union
    {
        t_FmPcdKgInterModuleSchemeRegs      schemeRegs;
        t_FmPcdKgInterModuleSchemeRegs      schemeRegsWithoutCounter;
        t_FmPcdPlcrInterModuleProfileRegs   profileRegs;
        uint32_t                            singleRegForWrite;    /* for writing SP, CPP, profile counter */
        t_FmPcdKgPortRegs                   portRegsForRead;
        uint32_t                            clsPlanEntries[CLS_PLAN_NUM_PER_GRP];
    } hcSpecificData;
} t_HcFrame;

typedef struct t_FmHc {
    t_Handle                h_FmPcd;
    t_Handle                h_HcPortDev;
    uint32_t                enqFqid;            /**< Host-Command enqueue Queue Id. */
    t_FmPcdQmEnqueueCB      *f_QmEnqueueCB;     /**< A callback for enquing frames to the QM */
    t_Handle                h_QmArg;            /**< A handle to the QM module */

    //volatile bool           lock;
    uint32_t                seqNum;
    volatile bool           wait[32];
} t_FmHc;


static t_Error KgHcSetClsPlan(t_FmHc *p_FmHc, t_FmPcdKgInterModuleClsPlanSet *p_Set)
{
    t_HcFrame               hcFrame;
    t_FmFD                  fmFd;
    int                     i;
    t_Error                 err = E_OK;

    ASSERT_COND(p_FmHc);

    for(i=p_Set->baseEntry;i<p_Set->baseEntry+p_Set->numOfClsPlanEntries;i+=8)
    {
        memset(&hcFrame, 0, sizeof(hcFrame));
        hcFrame.opcode = 0x00000001;
        hcFrame.actionReg  = FmPcdKgBuildWriteClsPlanBlockActionReg((uint8_t)(i / CLS_PLAN_NUM_PER_GRP));
        hcFrame.extraReg = 0xFFFFF800;
        hcFrame.commandSequence = p_FmHc->seqNum;
        memcpy(&hcFrame.hcSpecificData.clsPlanEntries, &p_Set->vectors[i-p_Set->baseEntry], CLS_PLAN_NUM_PER_GRP*sizeof(uint32_t));

        BUILD_FD(sizeof(hcFrame));

        ENQUEUE_FRM(&fmFd);
    }

    return E_OK;
}

static t_Error CcHcDoDynamicChange(t_FmHc *p_FmHc, bool keyModify, t_Handle p_OldPointer, t_Handle p_NewPointer)
{
    t_HcFrame               hcFrame;
    t_FmFD                  fmFd;
    t_Error                 err = E_OK;

    ASSERT_COND(p_FmHc);

    memset(&hcFrame, 0, sizeof(hcFrame));
    hcFrame.opcode = 0x00000003;
    hcFrame.actionReg  = FmPcdCcGetNodeAddrOffset(p_FmHc->h_FmPcd, p_NewPointer);
    if(hcFrame.actionReg == ILLEGAL_BASE)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Something wrong with base address"));
    hcFrame.actionReg  |=  0xc0000000;
    if(keyModify)
        hcFrame.extraReg   = FmPcdCcGetNodeAddrOffsetFromNodeInfo(p_FmHc->h_FmPcd, p_OldPointer);
    else
        hcFrame.extraReg   = FmPcdCcGetNodeAddrOffset(p_FmHc->h_FmPcd, p_OldPointer);
    if(hcFrame.extraReg == ILLEGAL_BASE)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Something wrong with base address"));
    hcFrame.commandSequence = p_FmHc->seqNum;

    BUILD_FD(SIZE_OF_HC_FRAME_READ_OR_CC_DYNAMIC);

    ENQUEUE_FRM(&fmFd);

    return E_OK;
}

static t_Error CcHcDynamicChangeForNextEngine(t_FmHc *p_FmHc, t_Handle h_OldPointer, t_Handle h_NewPointer)
{
    t_Error err = E_OK;

    ASSERT_COND(p_FmHc);

    err = CcHcDoDynamicChange(p_FmHc, FALSE, h_OldPointer, h_NewPointer);
    if(err)
    {
        FmPcdCcReleaseModifiedOnlyNextEngine(p_FmHc->h_FmPcd, h_OldPointer, h_NewPointer, FALSE);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    return FmPcdCcReleaseModifiedOnlyNextEngine(p_FmHc->h_FmPcd, h_OldPointer, h_NewPointer, TRUE);
}
static t_Error HcDynamicChangeForKey(t_FmHc *p_FmHc,t_Handle  *h_OldPointersLst, t_Handle h_NewPointer)
{

    t_List      *p_Pos;
    uint16_t    i = 0;
    t_Error     err = E_OK;
    t_List      *p_OldPointersLst = (t_List *)h_OldPointersLst;

    LIST_FOR_EACH(p_Pos, p_OldPointersLst)
    {
        err = CcHcDoDynamicChange(p_FmHc, TRUE, (t_Handle)p_Pos, h_NewPointer);
        if(err)
        {
            FmPcdCcReleaseModifiedKey(p_FmHc->h_FmPcd, p_OldPointersLst, h_NewPointer, i);
            RETURN_ERROR(MAJOR, err, ("For part of nodes changes are done - situation is danger"));
        }
        i++;
    }

    err = FmPcdCcReleaseModifiedKey(p_FmHc->h_FmPcd, p_OldPointersLst, h_NewPointer, i);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    LIST_Del(p_OldPointersLst);

    return E_OK;
}

t_Handle    FmHcConfigAndInit(t_FmHcParams *p_FmHcParams)
{
    t_FmHc          *p_FmHc;
#ifndef CONFIG_GUEST_PARTITION
    t_FmPortParams  fmPortParam;
    t_Error         err = E_OK;
#endif /* !CONFIG_GUEST_PARTITION */

    p_FmHc = XX_Malloc(sizeof(t_FmHc));
    if (!p_FmHc)
    {
        REPORT_ERROR(MINOR, E_NO_MEMORY, ("HC obj"));
        return NULL;
    }
    memset(p_FmHc,0,sizeof(t_FmHc));

    p_FmHc->h_FmPcd             = p_FmHcParams->h_FmPcd;
    p_FmHc->enqFqid             = p_FmHcParams->params.enqFqid;
    p_FmHc->f_QmEnqueueCB       = p_FmHcParams->params.f_QmEnqueueCB;
    p_FmHc->h_QmArg             = p_FmHcParams->params.h_QmArg;

#ifndef CONFIG_GUEST_PARTITION
    memset(&fmPortParam, 0, sizeof(fmPortParam));
    fmPortParam.baseAddr    = p_FmHcParams->params.portBaseAddr;
    fmPortParam.portType    = e_FM_PORT_TYPE_HOST_COMMAND;
    fmPortParam.portId      = p_FmHcParams->params.portId;
    fmPortParam.h_Fm        = p_FmHcParams->h_Fm;

    fmPortParam.specificParams.nonRxParams.errFqid      = p_FmHcParams->params.errFqid;
    fmPortParam.specificParams.nonRxParams.dfltFqid     = p_FmHcParams->params.confFqid;
    fmPortParam.specificParams.nonRxParams.deqSubPortal = p_FmHcParams->params.deqSubPortal;

    p_FmHc->h_HcPortDev = FM_PORT_Config(&fmPortParam);
    if(!p_FmHc->h_HcPortDev)
    {
        REPORT_ERROR(MAJOR, E_INVALID_HANDLE, ("FM HC port!"));
        XX_Free(p_FmHc);
        return NULL;
    }

    /* final init */
    if ((err = FM_PORT_Init(p_FmHc->h_HcPortDev)) != E_OK)
    {
        REPORT_ERROR(MAJOR, E_INVALID_HANDLE, ("FM HC port!"));
        FmHcFree(p_FmHc);
        return NULL;
    }
#endif /* !CONFIG_GUEST_PARTITION */

    return (t_Handle)p_FmHc;
}

void FmHcFree(t_Handle h_FmHc)
{
    t_FmHc  *p_FmHc = (t_FmHc*)h_FmHc;

    if (!p_FmHc)
        return;

    if (p_FmHc->h_HcPortDev)
        FM_PORT_Free(p_FmHc->h_HcPortDev);

    XX_Free(p_FmHc);
}

void FmHcTxConf(t_Handle h_FmHc, t_FmFD *p_Fd)
{
    t_FmHc      *p_FmHc = (t_FmHc*)h_FmHc;
    t_HcFrame   *p_HcFrame;

    ASSERT_COND(p_FmHc);

    p_HcFrame  = CAST_UINT64_TO_POINTER_TYPE(t_HcFrame,
                                             (CAST_POINTER_TO_UINT64(FM_FD_GET_ADDR(p_Fd)) + FM_FD_GET_OFFSET(p_Fd)));

    DBG(TRACE, ("Hc Conf 0x%x , SeqNum %d, fd addr 0x%x, fd offset 0x%x",
            p_FmHc,p_HcFrame->commandSequence,FM_FD_GET_ADDR(p_Fd),FM_FD_GET_OFFSET(p_Fd)));

    ASSERT_COND(p_FmHc->wait[p_HcFrame->commandSequence]);

    p_FmHc->wait[p_HcFrame->commandSequence] = FALSE;
}

t_Handle FmHcPcdKgSetScheme(t_Handle h_FmHc, t_FmPcdKgSchemeParams *p_Scheme)
{
    t_FmHc                              *p_FmHc = (t_FmHc*)h_FmHc;
    t_Error                             err = E_OK;
    t_FmPcdKgInterModuleSchemeRegs      schemeRegs;
    t_HcFrame                           hcFrame;
    t_FmFD                              fmFd;
    uint8_t                             physicalSchemeId, relativeSchemeId;

    if (FmPcdTryLock(p_FmHc->h_FmPcd))
        return NULL;

    if(!p_Scheme->modify)
    {
        /* check that schameId is in range */
        if(p_Scheme->id.relativeSchemeId >= FmPcdKgGetNumOfPartitionSchemes(p_FmHc->h_FmPcd))
        {
            REPORT_ERROR(MAJOR, E_NOT_IN_RANGE, ("Scheme is out of range"));
            return NULL;
        }

        relativeSchemeId = p_Scheme->id.relativeSchemeId;

        if (FmPcdKgSchemeTryLock(p_FmHc->h_FmPcd, relativeSchemeId))
        {
            FmPcdReleaseLock(p_FmHc->h_FmPcd);
            return NULL;
        }
        FmPcdReleaseLock(p_FmHc->h_FmPcd);

        physicalSchemeId = FmPcdKgGetPhysicalSchemeId(p_FmHc->h_FmPcd, relativeSchemeId);

        memset(&hcFrame, 0, sizeof(hcFrame));
        hcFrame.opcode = 0x00000001;
        hcFrame.actionReg  = FmPcdKgBuildReadSchemeActionReg(physicalSchemeId);
        hcFrame.extraReg = 0xFFFFF800;
        hcFrame.commandSequence = p_FmHc->seqNum;

        BUILD_FD(SIZE_OF_HC_FRAME_READ_OR_CC_DYNAMIC);

        ENQUEUE_FRM_RET_NULL(&fmFd);

        /* check if this scheme is already used */
        if (FmPcdKgHwSchemeIsValid(hcFrame.hcSpecificData.schemeRegs.kgse_mode))
        {
            FmPcdKgReleaseSchemeLock(p_FmHc->h_FmPcd, relativeSchemeId);
            REPORT_ERROR(MAJOR, E_ALREADY_EXISTS, ("Scheme is already used"));
            return NULL;
        }
    }
    else
    {
        physicalSchemeId = (uint8_t)(CAST_POINTER_TO_UINT32(p_Scheme->id.h_Scheme)-1);
        relativeSchemeId = FmPcdKgGetRelativeSchemeId(p_FmHc->h_FmPcd, physicalSchemeId);
        if (FmPcdKgSchemeTryLock(p_FmHc->h_FmPcd, relativeSchemeId))
        {
            FmPcdReleaseLock(p_FmHc->h_FmPcd);
            return NULL;
        }
        FmPcdReleaseLock(p_FmHc->h_FmPcd);

        if( relativeSchemeId == FM_PCD_KG_NUM_OF_SCHEMES)
        {
            FmPcdKgReleaseSchemeLock(p_FmHc->h_FmPcd, relativeSchemeId);
            REPORT_ERROR(MAJOR, E_NOT_IN_RANGE, NO_MSG);
            return NULL;
        }
    }

    err = FmPcdKgBuildScheme(p_FmHc->h_FmPcd, p_Scheme, &schemeRegs,  &p_Scheme->orderedArray);
    if(err)
    {
        FmPcdKgReleaseSchemeLock(p_FmHc->h_FmPcd, relativeSchemeId);
        REPORT_ERROR(MAJOR, err, NO_MSG);
        return NULL;
    }

    memset(&hcFrame, 0, sizeof(hcFrame));
    hcFrame.opcode = 0x00000001;
    hcFrame.actionReg  = FmPcdKgBuildWriteSchemeActionReg(physicalSchemeId, p_Scheme->schemeCounter.update);
    hcFrame.extraReg = 0xFFFFF800;
    hcFrame.commandSequence = p_FmHc->seqNum;
    memcpy(&hcFrame.hcSpecificData.schemeRegs, &schemeRegs, sizeof(t_FmPcdKgInterModuleSchemeRegs));
    //p_NewStruct= (t_FmPcdKgSchemeRegsWithoutCounter*)&hcFrame.hcSpecificData;
    if(!p_Scheme->schemeCounter.update)
        memcpy((t_FmPcdKgSchemeRegsWithoutCounter*)&hcFrame.hcSpecificData.schemeRegs.kgse_dv0, &schemeRegs.kgse_dv0, 4*sizeof(uint32_t));

    BUILD_FD(sizeof(hcFrame));

    ENQUEUE_FRM_RET_NULL(&fmFd);

    FmPcdKgValidateSchemeSw(p_FmHc->h_FmPcd, relativeSchemeId);

    FmPcdKgReleaseSchemeLock(p_FmHc->h_FmPcd, relativeSchemeId);

    return CAST_UINT32_TO_POINTER((uint32_t)physicalSchemeId+1);
}

t_Error FmHcPcdKgDeleteScheme(t_Handle h_FmHc, t_Handle h_Scheme)
{
    t_FmHc                              *p_FmHc = (t_FmHc*)h_FmHc;
    t_Error                             err = E_OK;
    t_HcFrame                           hcFrame;
    t_FmFD                              fmFd;
    uint8_t                             relativeSchemeId;
    uint8_t                             physicalSchemeId = (uint8_t)(CAST_POINTER_TO_UINT32(h_Scheme)-1);

    if ((err = FmPcdTryLock(p_FmHc->h_FmPcd)) != E_OK)
        return err;

    relativeSchemeId = FmPcdKgGetRelativeSchemeId(p_FmHc->h_FmPcd, physicalSchemeId);

    err = FmPcdKgSchemeTryLock(p_FmHc->h_FmPcd, relativeSchemeId);
    FmPcdReleaseLock(p_FmHc->h_FmPcd);
    if (err)
        return err;

    if(relativeSchemeId == FM_PCD_KG_NUM_OF_SCHEMES)
    {
        FmPcdKgReleaseSchemeLock(p_FmHc->h_FmPcd, relativeSchemeId);
        REPORT_ERROR(MAJOR, E_NOT_IN_RANGE, NO_MSG);
    }

    FmPcdKgCheckInvalidateSchemeSw(p_FmHc->h_FmPcd, relativeSchemeId);

    memset(&hcFrame, 0, sizeof(hcFrame));
    hcFrame.opcode = 0x00000001;
    hcFrame.actionReg  = FmPcdKgBuildWriteSchemeActionReg(physicalSchemeId, TRUE);
    hcFrame.extraReg = 0xFFFFF800;
    memset(&hcFrame.hcSpecificData.schemeRegs, 0, sizeof(t_FmPcdKgInterModuleSchemeRegs));
    hcFrame.commandSequence = p_FmHc->seqNum;

    BUILD_FD(sizeof(hcFrame));

    ENQUEUE_FRM(&fmFd);

    FmPcdKgInvalidateSchemeSw(p_FmHc->h_FmPcd, relativeSchemeId);

    FmPcdKgReleaseSchemeLock(p_FmHc->h_FmPcd, relativeSchemeId);

    return E_OK;
}

uint32_t  FmHcPcdKgGetSchemeCounter(t_Handle h_FmHc, t_Handle h_Scheme)
{
    t_FmHc                              *p_FmHc = (t_FmHc*)h_FmHc;
    t_Error                             err = E_OK;
    t_HcFrame                           hcFrame;
    t_FmFD                              fmFd;
    uint32_t                            retVal;
    uint8_t                             relativeSchemeId;
    uint8_t                             physicalSchemeId = (uint8_t)(CAST_POINTER_TO_UINT32(h_Scheme)-1);

    if (FmPcdTryLock(p_FmHc->h_FmPcd))
        return 0;

    relativeSchemeId = FmPcdKgGetRelativeSchemeId(p_FmHc->h_FmPcd, physicalSchemeId);
    if( relativeSchemeId == FM_PCD_KG_NUM_OF_SCHEMES)
    {
        FmPcdReleaseLock(p_FmHc->h_FmPcd);
        REPORT_ERROR(MAJOR, E_NOT_IN_RANGE, NO_MSG);
        return 0;
    }

    err = FmPcdKgSchemeTryLock(p_FmHc->h_FmPcd, relativeSchemeId);
    FmPcdReleaseLock(p_FmHc->h_FmPcd);
    if (err)
        return 0;

    /* first read scheme and check that it is valid */
    memset(&hcFrame, 0, sizeof(hcFrame));
    hcFrame.opcode = 0x00000001;
    hcFrame.actionReg  = FmPcdKgBuildReadSchemeActionReg(physicalSchemeId);
    hcFrame.extraReg = 0xFFFFF800;
    hcFrame.commandSequence = p_FmHc->seqNum;

    BUILD_FD(SIZE_OF_HC_FRAME_READ_OR_CC_DYNAMIC);

    ENQUEUE_FRM(&fmFd);

    if (!FmPcdKgHwSchemeIsValid(hcFrame.hcSpecificData.schemeRegs.kgse_mode))
    {
        REPORT_ERROR(MAJOR, E_ALREADY_EXISTS, ("Scheme is invalid"));
        return 0;
    }

    retVal = hcFrame.hcSpecificData.schemeRegs.kgse_spc;

    FmPcdKgReleaseSchemeLock(p_FmHc->h_FmPcd, relativeSchemeId);

    return retVal;
}

t_Error  FmHcPcdKgSetSchemeCounter(t_Handle h_FmHc, t_Handle h_Scheme, uint32_t value)
{

    t_FmHc                              *p_FmHc = (t_FmHc*)h_FmHc;
    t_Error                             err = E_OK;
    t_HcFrame                           hcFrame;
    t_FmFD                              fmFd;
    uint8_t                             relativeSchemeId, physicalSchemeId = (uint8_t)(CAST_POINTER_TO_UINT32(h_Scheme)-1);

    if (FmPcdTryLock(p_FmHc->h_FmPcd))
        return ERROR_CODE(E_BUSY);

    relativeSchemeId = FmPcdKgGetRelativeSchemeId(p_FmHc->h_FmPcd, physicalSchemeId);
    if( relativeSchemeId == FM_PCD_KG_NUM_OF_SCHEMES)
    {
        FmPcdReleaseLock(p_FmHc->h_FmPcd);
        RETURN_ERROR(MAJOR, E_NOT_IN_RANGE, NO_MSG);
    }

    err = FmPcdKgSchemeTryLock(p_FmHc->h_FmPcd, relativeSchemeId);
    FmPcdReleaseLock(p_FmHc->h_FmPcd);
    if (err)
        return err;

    /* first read scheme and check that it is valid */
    memset(&hcFrame, 0, sizeof(hcFrame));
    hcFrame.opcode = 0x00000001;
    hcFrame.actionReg  = FmPcdKgBuildReadSchemeActionReg(physicalSchemeId);
    hcFrame.extraReg = 0xFFFFF800;
    hcFrame.commandSequence = p_FmHc->seqNum;

    BUILD_FD(SIZE_OF_HC_FRAME_READ_OR_CC_DYNAMIC);

    ENQUEUE_FRM(&fmFd);

    /* check that scheme is valid */
    if (!FmPcdKgHwSchemeIsValid(hcFrame.hcSpecificData.schemeRegs.kgse_mode))
    {
        FmPcdKgReleaseSchemeLock(p_FmHc->h_FmPcd, relativeSchemeId);
        RETURN_ERROR(MAJOR, E_ALREADY_EXISTS, ("Scheme is invalid"));
    }

    /* Write scheme back, with modified counter */
    hcFrame.opcode = 0x00000001;
    hcFrame.actionReg  = FmPcdKgBuildWriteSchemeActionReg(physicalSchemeId, TRUE);
    hcFrame.extraReg = 0xFFFFF800;
    hcFrame.commandSequence = p_FmHc->seqNum;
    /* write counter */
    hcFrame.hcSpecificData.schemeRegs.kgse_spc = value;

    BUILD_FD(sizeof(hcFrame));

    ENQUEUE_FRM(&fmFd);

    FmPcdKgReleaseSchemeLock(p_FmHc->h_FmPcd, relativeSchemeId);

    return E_OK;
}

t_Handle FmHcPcdKgSetClsPlanGrp(t_Handle h_FmHc, t_FmPcdKgClsPlanGrpParams *p_Grp)
{
    t_FmHc                          *p_FmHc = (t_FmHc*)h_FmHc;
    t_FmPcdKgInterModuleClsPlanSet  clsPlanSet;
    t_Handle                        h_ClsPlanGrp;
    t_Error                         err = E_OK;

    h_ClsPlanGrp = FmPcdKgBuildClsPlanGrp(p_FmHc->h_FmPcd, p_Grp, &clsPlanSet);
    if(!h_ClsPlanGrp)
    {
        REPORT_ERROR(MAJOR, E_INVALID_HANDLE, NO_MSG);
        return NULL;
    }

    /* write clsPlan entries to memory */
    err = KgHcSetClsPlan(p_FmHc, &clsPlanSet);
    if (err)
    {
        REPORT_ERROR(MAJOR, err, NO_MSG);
        return NULL;
    }

    return h_ClsPlanGrp;
}

t_Error FmHcPcdKgDeleteClsPlanGrp(t_Handle h_FmHc, t_Handle h_ClsPlanGrp)
{
    t_FmHc                              *p_FmHc = (t_FmHc*)h_FmHc;
    uint8_t                             grpId = (uint8_t)(CAST_POINTER_TO_UINT32(h_ClsPlanGrp)-1);
    t_FmPcdKgInterModuleClsPlanSet      clsPlanSet;

    /* clear clsPlan entries in memory */
    clsPlanSet.baseEntry = FmPcdKgGetClsPlanGrpBase(p_FmHc->h_FmPcd, grpId);
    clsPlanSet.numOfClsPlanEntries = FmPcdKgGetClsPlanGrpSize(p_FmHc->h_FmPcd, grpId);
    memset(clsPlanSet.vectors, 0, clsPlanSet.numOfClsPlanEntries*sizeof(uint32_t));

    KgHcSetClsPlan(p_FmHc, &clsPlanSet);

    FmPcdKgDestroyClsPlanGrp(p_FmHc->h_FmPcd, grpId);

    return E_OK;
}

t_Handle FmHcPcdPlcrSetProfile(t_Handle h_FmHc,t_FmPcdPlcrProfileParams *p_Profile)
{
    t_FmHc                              *p_FmHc = (t_FmHc*)h_FmHc;
    t_FmPcdPlcrInterModuleProfileRegs              profileRegs;
    t_Error                             err = E_OK;
    uint16_t                            profileIndx;
    t_HcFrame                           hcFrame;
    t_FmFD                              fmFd;

    if (p_Profile->modify)
    {
        profileIndx = (uint16_t)(CAST_POINTER_TO_UINT32(p_Profile->id.h_Profile)-1);
        if (FmPcdPlcrProfileTryLock(p_FmHc->h_FmPcd, profileIndx))
            return NULL;
    }
    else
    {
        if (FmPcdTryLock(p_FmHc->h_FmPcd))
            return NULL;
        err = FmPcdPlcrGetAbsoluteProfileId(p_FmHc->h_FmPcd,
                                            p_Profile->id.newParams.profileType,
                                            p_Profile->id.newParams.h_FmPort,
                                            p_Profile->id.newParams.relativeProfileId,
                                            &profileIndx);
        if (err)
        {
            REPORT_ERROR(MAJOR, err, NO_MSG);
            return NULL;
        }
        if (FmPcdPlcrProfileTryLock(p_FmHc->h_FmPcd, profileIndx))
        {
            FmPcdReleaseLock(p_FmHc->h_FmPcd);
            return NULL;
        }
        FmPcdReleaseLock(p_FmHc->h_FmPcd);
    }

    if(!p_Profile->modify)
    {
        memset(&hcFrame, 0, sizeof(hcFrame));
        hcFrame.opcode = 0x00000000;
        hcFrame.actionReg  = FmPcdPlcrBuildReadPlcrActionReg(profileIndx);
        hcFrame.extraReg = 0x00008000;
        hcFrame.commandSequence = p_FmHc->seqNum;

        BUILD_FD(SIZE_OF_HC_FRAME_READ_OR_CC_DYNAMIC);

        ENQUEUE_FRM_RET_NULL(&fmFd);

        /* check if this scheme is already used */
        if (FmPcdPlcrHwProfileIsValid(hcFrame.hcSpecificData.profileRegs.fmpl_pemode))
        {
            FmPcdPlcrReleaseProfileLock(p_FmHc->h_FmPcd, profileIndx);
            REPORT_ERROR(MAJOR, E_ALREADY_EXISTS, ("Policer is already used"));
            return NULL;
        }
    }

    memset(&profileRegs, 0, sizeof(t_FmPcdPlcrInterModuleProfileRegs));
    err = FmPcdPlcrBuildProfile(p_FmHc->h_FmPcd, p_Profile, &profileRegs);
    if(err)
    {
        FmPcdPlcrReleaseProfileLock(p_FmHc->h_FmPcd, profileIndx);
        REPORT_ERROR(MAJOR, err, NO_MSG);
        return NULL;
    }

    memset(&hcFrame, 0, sizeof(hcFrame));
    hcFrame.opcode = 0x00000000;
    hcFrame.actionReg  = FmPcdPlcrBuildWritePlcrActionRegs(profileIndx);
    hcFrame.extraReg = 0x00008000;
    hcFrame.commandSequence = p_FmHc->seqNum;
    memcpy(&hcFrame.hcSpecificData.profileRegs, &profileRegs, sizeof(t_FmPcdPlcrInterModuleProfileRegs));

    BUILD_FD(sizeof(hcFrame));

    ENQUEUE_FRM_RET_NULL(&fmFd);

    FmPcdPlcrValidateProfileSw(p_FmHc->h_FmPcd, profileIndx);

    FmPcdPlcrReleaseProfileLock(p_FmHc->h_FmPcd, profileIndx);

    return CAST_UINT32_TO_POINTER((uint32_t)profileIndx+1);
}

t_Error FmHcPcdPlcrDeleteProfile(t_Handle h_FmHc, t_Handle h_Profile)
{
    t_FmHc                              *p_FmHc = (t_FmHc*)h_FmHc;
    uint16_t                            absoluteProfileId = (uint16_t)(CAST_POINTER_TO_UINT32(h_Profile)-1);
    t_Error                             err = E_OK;
    t_HcFrame                           hcFrame;
    t_FmFD                              fmFd;

    if (FmPcdPlcrProfileTryLock(p_FmHc->h_FmPcd, absoluteProfileId))
        return ERROR_CODE(E_BUSY);

    FmPcdPlcrInvalidateProfileSw(p_FmHc->h_FmPcd, absoluteProfileId);
    memset(&hcFrame, 0, sizeof(hcFrame));
    hcFrame.opcode = 0x00000000;
    hcFrame.actionReg  = FmPcdPlcrBuildWritePlcrActionReg(absoluteProfileId);
    hcFrame.actionReg  |= 0x00008000;
    hcFrame.extraReg = 0x00008000;
    hcFrame.commandSequence = p_FmHc->seqNum;
    memset(&hcFrame.hcSpecificData.profileRegs, 0, sizeof(t_FmPcdPlcrInterModuleProfileRegs));

    BUILD_FD(sizeof(hcFrame));

    ENQUEUE_FRM(&fmFd);

    FmPcdPlcrReleaseProfileLock(p_FmHc->h_FmPcd, absoluteProfileId);

    return E_OK;
}

t_Error  FmHcPcdPlcrSetProfileCounter(t_Handle h_FmHc, t_Handle h_Profile, e_FmPcdPlcrProfileCounters counter, uint32_t value)
{

    t_FmHc                              *p_FmHc = (t_FmHc*)h_FmHc;
    uint16_t                            absoluteProfileId = (uint16_t)(CAST_POINTER_TO_UINT32(h_Profile)-1);
    t_Error                             err = E_OK;
    t_HcFrame                           hcFrame;
    t_FmFD                              fmFd;

    if (FmPcdPlcrProfileTryLock(p_FmHc->h_FmPcd, absoluteProfileId))
        return ERROR_CODE(E_BUSY);

    /* first read scheme and check that it is valid */
    memset(&hcFrame, 0, sizeof(hcFrame));
    hcFrame.opcode = 0x00000000;
    hcFrame.actionReg  = FmPcdPlcrBuildReadPlcrActionReg(absoluteProfileId);
    hcFrame.extraReg = 0x00008000;
    hcFrame.commandSequence = p_FmHc->seqNum;

    BUILD_FD(SIZE_OF_HC_FRAME_READ_OR_CC_DYNAMIC);

    ENQUEUE_FRM(&fmFd);

    /* check that profile is valid */
    if (!FmPcdPlcrHwProfileIsValid(hcFrame.hcSpecificData.profileRegs.fmpl_pemode))
    {
        FmPcdPlcrReleaseProfileLock(p_FmHc->h_FmPcd, absoluteProfileId);
        RETURN_ERROR(MAJOR, E_ALREADY_EXISTS, ("Policer is already used"));
    }

    hcFrame.opcode = 0x00000000;
    hcFrame.actionReg  = FmPcdPlcrBuildWritePlcrActionReg(absoluteProfileId);
    hcFrame.actionReg |= FmPcdPlcrBuildCounterProfileReg(counter);
    hcFrame.extraReg = 0x00008000;
    hcFrame.commandSequence = p_FmHc->seqNum;
    hcFrame.hcSpecificData.singleRegForWrite = value;

    BUILD_FD(SIZE_OF_HC_FRAME_PROFILE_CNT);

    ENQUEUE_FRM(&fmFd);

    FmPcdPlcrReleaseProfileLock(p_FmHc->h_FmPcd, absoluteProfileId);

    return E_OK;
}

uint32_t FmHcPcdPlcrGetProfileCounter(t_Handle h_FmHc, t_Handle h_Profile, e_FmPcdPlcrProfileCounters counter)
{
    t_FmHc                              *p_FmHc = (t_FmHc*)h_FmHc;
    uint16_t                            absoluteProfileId = (uint16_t)(CAST_POINTER_TO_UINT32(h_Profile)-1);
    t_Error                             err = E_OK;
    t_HcFrame                           hcFrame;
    t_FmFD                              fmFd;
    uint32_t                            retVal;

    SANITY_CHECK_RETURN_VALUE(h_FmHc, E_INVALID_HANDLE,0);

    if (FmPcdPlcrProfileTryLock(p_FmHc->h_FmPcd, absoluteProfileId))
        return 0;

    /* first read scheme and check that it is valid */
    memset(&hcFrame, 0, sizeof(hcFrame));
    hcFrame.opcode = 0x00000000;
    hcFrame.actionReg  = FmPcdPlcrBuildReadPlcrActionReg(absoluteProfileId);
    hcFrame.extraReg = 0x00008000;
    hcFrame.commandSequence = p_FmHc->seqNum;

    BUILD_FD(SIZE_OF_HC_FRAME_READ_OR_CC_DYNAMIC);

    ENQUEUE_FRM(&fmFd);

    /* check that profile is valid */
    if (!FmPcdPlcrHwProfileIsValid(hcFrame.hcSpecificData.profileRegs.fmpl_pemode))
    {
        FmPcdPlcrReleaseProfileLock(p_FmHc->h_FmPcd, absoluteProfileId);
        RETURN_ERROR(MAJOR, E_ALREADY_EXISTS, ("Policer is already used"));
    }

    switch (counter)
    {
        case e_FM_PCD_PLCR_PROFILE_GREEN_PACKET_TOTAL_COUNTER:
            retVal = hcFrame.hcSpecificData.profileRegs.fmpl_pegpc;
            break;
        case e_FM_PCD_PLCR_PROFILE_YELLOW_PACKET_TOTAL_COUNTER:
            retVal = hcFrame.hcSpecificData.profileRegs.fmpl_peypc;
            break;
        case e_FM_PCD_PLCR_PROFILE_RED_PACKET_TOTAL_COUNTER:
            retVal = hcFrame.hcSpecificData.profileRegs.fmpl_perpc;
            break;
        case e_FM_PCD_PLCR_PROFILE_RECOLOURED_YELLOW_PACKET_TOTAL_COUNTER:
            retVal = hcFrame.hcSpecificData.profileRegs.fmpl_perypc;
            break;
        case e_FM_PCD_PLCR_PROFILE_RECOLOURED_RED_PACKET_TOTAL_COUNTER:
            retVal = hcFrame.hcSpecificData.profileRegs.fmpl_perrpc;
            break;
        default:
            REPORT_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
            retVal = 0;
    }

    FmPcdPlcrReleaseProfileLock(p_FmHc->h_FmPcd, absoluteProfileId);

    return retVal;
}

t_Error FmHcPcdCcModifyTreeNextEngine(t_Handle h_FmHc, t_Handle h_CcTree, uint8_t grpId, uint8_t index, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams)
{
    t_FmHc      *p_FmHc = (t_FmHc*)h_FmHc;
    t_Error     err = E_OK;
    t_Handle    h_OldPointer, h_NewPointer;

    if ((err = FmPcdCcTreeTryLock(h_CcTree)) != E_OK)
        return err;

    err = FmPcdCcModifyNextEngineParamTree(p_FmHc->h_FmPcd, h_CcTree, grpId, index, p_FmPcdCcNextEngineParams,
            &h_OldPointer, &h_NewPointer);
    if(err)
    {
        FmPcdCcTreeReleaseLock(h_CcTree);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    err = CcHcDynamicChangeForNextEngine(p_FmHc, h_OldPointer, h_NewPointer);
    FmPcdCcTreeReleaseLock(h_CcTree);
    return err;
}

t_Error FmHcPcdCcModifyNodeNextEngine(t_Handle h_FmHc, t_Handle h_CcNode, uint8_t keyIndex, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams)
{
    t_FmHc      *p_FmHc = (t_FmHc*)h_FmHc;
    t_Error     err = E_OK;
    t_Handle    h_OldPointer, h_NewPointer;
    t_List      h_List;

    INIT_LIST(&h_List);

    if (FmPcdTryLock(p_FmHc->h_FmPcd) != E_OK)
        return err;

    if ((err = FmPcdCcNodeTreeTryLock(p_FmHc->h_FmPcd, h_CcNode, &h_List)) != E_OK)
    {
        FmPcdReleaseLock(p_FmHc->h_FmPcd);
        return err;
    }

    FmPcdReleaseLock(p_FmHc->h_FmPcd);
    err = FmPcdCcModiyNextEngineParamNode(p_FmHc->h_FmPcd, h_CcNode, keyIndex, p_FmPcdCcNextEngineParams, &h_OldPointer, &h_NewPointer);
    if(err)
    {
        FmPcdCcNodeTreeReleaseLock(p_FmHc->h_FmPcd, &h_List);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    err =   CcHcDynamicChangeForNextEngine(p_FmHc, h_OldPointer, h_NewPointer);
    FmPcdCcNodeTreeReleaseLock(p_FmHc->h_FmPcd, &h_List);
    return err;
}

t_Error FmHcPcdCcModifyNodeMissNextEngine(t_Handle h_FmHc, t_Handle h_CcNode, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams)
{
    t_FmHc      *p_FmHc = (t_FmHc*)h_FmHc;
    t_Error     err = E_OK;
    t_Handle    h_OldPointer, h_NewPointer;
    t_List      h_List      ;

    INIT_LIST(&h_List);

    if (FmPcdTryLock(p_FmHc->h_FmPcd) != E_OK)
        return err;

    if ((err = FmPcdCcNodeTreeTryLock(p_FmHc->h_FmPcd, h_CcNode, &h_List)) != E_OK)
    {
        FmPcdReleaseLock(p_FmHc->h_FmPcd);
        return err;
    }

    FmPcdReleaseLock(p_FmHc->h_FmPcd);

    err = FmPcdCcModifyMissNextEngineParamNode(p_FmHc->h_FmPcd, h_CcNode, p_FmPcdCcNextEngineParams, &h_OldPointer, &h_NewPointer);
    if(err)
    {
        FmPcdCcNodeTreeReleaseLock(p_FmHc->h_FmPcd, &h_List);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    err =   CcHcDynamicChangeForNextEngine(p_FmHc, h_OldPointer, h_NewPointer);

    FmPcdCcNodeTreeReleaseLock(p_FmHc->h_FmPcd, &h_List);

    return E_OK;
}

t_Error FmHcPcdCcRemoveKey(t_Handle h_FmHc, t_Handle h_CcNode, uint8_t keyIndex)
{
    t_FmHc      *p_FmHc = (t_FmHc*)h_FmHc;
    t_Handle    h_NewPointer;
    t_List      h_OldPointersLst;
    t_Error     err = E_OK;
    t_List      h_List;

    INIT_LIST(&h_List);

    if ((err = FmPcdTryLock(p_FmHc->h_FmPcd)) != E_OK)
        return err;

    if ((err = FmPcdCcNodeTreeTryLock(p_FmHc->h_FmPcd, h_CcNode, &h_List)) != E_OK)
    {
        FmPcdReleaseLock(p_FmHc->h_FmPcd);
        return err;
    }

    FmPcdReleaseLock(p_FmHc->h_FmPcd);

    INIT_LIST(&h_OldPointersLst);

    err = FmPcdCcRemoveKey(p_FmHc->h_FmPcd,h_CcNode,keyIndex, &h_OldPointersLst,&h_NewPointer);
    if(err)
    {
        FmPcdCcNodeTreeReleaseLock(p_FmHc->h_FmPcd, &h_List);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    err =  HcDynamicChangeForKey(p_FmHc, (t_Handle)&h_OldPointersLst, h_NewPointer);

    FmPcdCcNodeTreeReleaseLock(p_FmHc->h_FmPcd, &h_List);

    return E_OK;
}

t_Error FmHcPcdCcAddKey(t_Handle h_FmHc, t_Handle h_CcNode, uint8_t keyIndex, uint8_t keySize, t_FmPcdCcKeyParams  *p_KeyParams)
{
    t_FmHc      *p_FmHc = (t_FmHc*)h_FmHc;
    t_Handle    h_NewPointer;
    t_List      h_OldPointersLst;
    t_Error     err = E_OK;
    t_List      h_List;

    UNUSED(keySize);

    INIT_LIST(&h_List);

    if ((err = FmPcdTryLock(p_FmHc->h_FmPcd)) != E_OK)
        return err;

    if ((err = FmPcdCcNodeTreeTryLock(p_FmHc->h_FmPcd, h_CcNode, &h_List)) != E_OK)
    {
        FmPcdReleaseLock(p_FmHc->h_FmPcd);
        return err;
    }

    FmPcdReleaseLock(p_FmHc->h_FmPcd);

    INIT_LIST(&h_OldPointersLst);


    err = FmPcdCcAddKey(p_FmHc->h_FmPcd,h_CcNode,keyIndex,p_KeyParams, &h_OldPointersLst,&h_NewPointer);
    if(err)
    {
        FmPcdCcNodeTreeReleaseLock(p_FmHc->h_FmPcd, &h_List);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    err =  HcDynamicChangeForKey(p_FmHc, (t_Handle)&h_OldPointersLst, h_NewPointer);

    FmPcdCcNodeTreeReleaseLock(p_FmHc->h_FmPcd, &h_List);

    return err;
}

t_Error FmHcPcdCcModifyKeyAndNextEngine(t_Handle h_FmHc, t_Handle h_CcNode, uint8_t keyIndex, uint8_t keySize, t_FmPcdCcKeyParams  *p_KeyParams)
{
    t_FmHc      *p_FmHc = (t_FmHc*)h_FmHc;
    t_List      h_OldPointersLst;
    t_Handle    h_NewPointer;
    t_Error     err = E_OK;
    t_List      h_List;

    UNUSED(keySize);

    INIT_LIST(&h_List);

    if ((err = FmPcdTryLock(p_FmHc->h_FmPcd)) != E_OK)
        return err;

    if ((err = FmPcdCcNodeTreeTryLock(p_FmHc->h_FmPcd, h_CcNode, &h_List)) != E_OK)
    {
        FmPcdReleaseLock(p_FmHc->h_FmPcd);
        return err;
    }

    FmPcdReleaseLock(p_FmHc->h_FmPcd);

    INIT_LIST(&h_OldPointersLst);

    err = FmPcdCcModifyKeyAndNextEngine(p_FmHc->h_FmPcd,h_CcNode,keyIndex,p_KeyParams, &h_OldPointersLst,&h_NewPointer);
    if(err)
    {
        FmPcdCcNodeTreeReleaseLock(p_FmHc->h_FmPcd, &h_List);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    err =  HcDynamicChangeForKey(p_FmHc, (t_Handle)&h_OldPointersLst, h_NewPointer);

    FmPcdCcNodeTreeReleaseLock(p_FmHc->h_FmPcd, &h_List);

    return err;
}

t_Error FmHcPcdCcModifyKey(t_Handle h_FmHc, t_Handle h_CcNode, uint8_t keyIndex, uint8_t keySize, uint8_t  *p_Key, uint8_t *p_Mask)
{
    t_FmHc      *p_FmHc = (t_FmHc*)h_FmHc;
    t_List      h_OldPointersLst;
    t_Handle    h_NewPointer;
    t_Error     err = E_OK;
    t_List      h_List;
    UNUSED(keySize);

    INIT_LIST(&h_List);

    if ((err = FmPcdTryLock(p_FmHc->h_FmPcd)) != E_OK)
        return err;

    if ((err = FmPcdCcNodeTreeTryLock(p_FmHc->h_FmPcd, h_CcNode, &h_List)) != E_OK)
    {
        FmPcdReleaseLock(p_FmHc->h_FmPcd);
        return err;
    }

    FmPcdReleaseLock(p_FmHc->h_FmPcd);

    INIT_LIST(&h_OldPointersLst);

    err = FmPcdCcModifyKey(p_FmHc->h_FmPcd, h_CcNode, keyIndex, p_Key, p_Mask, &h_OldPointersLst,&h_NewPointer);
    if(err)
    {
        FmPcdCcNodeTreeReleaseLock(p_FmHc->h_FmPcd, &h_List);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    err =  HcDynamicChangeForKey(p_FmHc, (t_Handle)&h_OldPointersLst, h_NewPointer);
    FmPcdCcNodeTreeReleaseLock(p_FmHc->h_FmPcd, &h_List);

    return err;
}

t_Error FmHcKgWriteSp(t_Handle h_FmHc, uint8_t hardwarePortId, uint32_t spReg, bool add)
{
    t_FmHc                  *p_FmHc = (t_FmHc*)h_FmHc;
    t_HcFrame               hcFrame;
    t_FmFD                  fmFd;
    t_Error                 err = E_OK;

    ASSERT_COND(p_FmHc);

    memset(&hcFrame, 0, sizeof(hcFrame));
    /* first read SP register */
    hcFrame.opcode = 0x00000001;
    hcFrame.actionReg  = FmPcdKgBuildReadPortSchemeBindActionReg(hardwarePortId);
    hcFrame.extraReg = 0xFFFFF800;
    hcFrame.commandSequence = p_FmHc->seqNum;

    BUILD_FD(SIZE_OF_HC_FRAME_PORT_REGS);

    ENQUEUE_FRM(&fmFd);

    /* spReg is the first reg, so we can use it bothe for read and for write */
    if(add)
        hcFrame.hcSpecificData.portRegsForRead.spReg |= spReg;
    else
        hcFrame.hcSpecificData.portRegsForRead.spReg &= ~spReg;

    hcFrame.actionReg  = FmPcdKgBuildWritePortSchemeBindActionReg(hardwarePortId);
    hcFrame.commandSequence = p_FmHc->seqNum;

    BUILD_FD(sizeof(hcFrame));

    ENQUEUE_FRM(&fmFd);

    return E_OK;
}

t_Error FmHcKgWriteCpp(t_Handle h_FmHc, uint8_t hardwarePortId, uint32_t cppReg)
{
    t_FmHc                  *p_FmHc = (t_FmHc*)h_FmHc;
    t_HcFrame               hcFrame;
    t_FmFD                  fmFd;
    t_Error                 err = E_OK;

    ASSERT_COND(p_FmHc);

    memset(&hcFrame, 0, sizeof(hcFrame));
    /* first read SP register */
    hcFrame.opcode = 0x00000001;
    hcFrame.actionReg  = FmPcdKgBuildWritePortClsPlanBindActionReg(hardwarePortId);
    hcFrame.extraReg = 0xFFFFF800;
    hcFrame.commandSequence = p_FmHc->seqNum;
    hcFrame.hcSpecificData.singleRegForWrite = cppReg;

    BUILD_FD(sizeof(hcFrame));

    ENQUEUE_FRM(&fmFd);

    return E_OK;
}
