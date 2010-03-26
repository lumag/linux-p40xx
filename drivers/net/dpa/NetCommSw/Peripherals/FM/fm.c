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

/******************************************************************************
 @File          fm.c

 @Description   FM driver routines implementation.
*//***************************************************************************/
#include "std_ext.h"
#include "error_ext.h"
#include "xx_ext.h"
#include "string_ext.h"
#include "sprint_ext.h"
#include "debug_ext.h"
#include "fm_muram_ext.h"
#include "fm_common.h"
#ifdef FM_MASTER_PARTITION
#include "fm_ipc.h"
#endif /* FM_MASTER_PARTITION */
#include "fm.h"


#define TS_FRAC_PRECISION_FACTOR    1000


/****************************************/
/*       static functions               */
/****************************************/
static t_Error CheckFmParameters(t_Fm *p_Fm)
{
    uint8_t     i;

    if(p_Fm->p_FmDriverParam->enTimeStamp)
    {
        if(!p_Fm->timeStampPeriod)
            RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("timeStampPeriod may not be 0"));
    }
    if(!p_Fm->p_FmDriverParam->dmaAxiDbgNumOfBeats || (p_Fm->p_FmDriverParam->dmaAxiDbgNumOfBeats > DMA_MODE_MAX_AXI_DBG_NUM_OF_BEATS))
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("axiDbgNumOfBeats has to be in the range 1 - %d", DMA_MODE_MAX_AXI_DBG_NUM_OF_BEATS));
    if(p_Fm->p_FmDriverParam->dmaCamNumOfEntries % DMA_CAM_UNITS)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("dmaCamNumOfEntries has to be divisble by %d", DMA_CAM_UNITS));
    if(!p_Fm->p_FmDriverParam->dmaCamNumOfEntries || (p_Fm->p_FmDriverParam->dmaCamNumOfEntries > DMA_MODE_MAX_CAM_NUM_OF_ENTRIES))
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("dmaCamNumOfEntries has to be in the range 1 - %d", DMA_MODE_MAX_CAM_NUM_OF_ENTRIES));
    if(p_Fm->p_FmDriverParam->dmaCommQThresholds.assertEmergency > DMA_THRESH_MAX_COMMQ)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("dmaCommQThresholds.assertEmergency can not be larger than %d", DMA_THRESH_MAX_COMMQ));
    if(p_Fm->p_FmDriverParam->dmaCommQThresholds.clearEmergency > DMA_THRESH_MAX_COMMQ)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("dmaCommQThresholds.clearEmergency can not be larger than %d", DMA_THRESH_MAX_COMMQ));
    if(p_Fm->p_FmDriverParam->dmaCommQThresholds.clearEmergency >= p_Fm->p_FmDriverParam->dmaCommQThresholds.assertEmergency)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("dmaCommQThresholds.clearEmergency must be smaller than dmaCommQThresholds.assertEmergency"));
    if(p_Fm->p_FmDriverParam->dmaReadBufThresholds.assertEmergency > DMA_THRESH_MAX_BUF)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("dmaReadBufThresholds.assertEmergency can not be larger than %d", DMA_THRESH_MAX_BUF));
    if(p_Fm->p_FmDriverParam->dmaReadBufThresholds.clearEmergency > DMA_THRESH_MAX_BUF)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("dmaReadBufThresholds.clearEmergency can not be larger than %d", DMA_THRESH_MAX_BUF));
    if(p_Fm->p_FmDriverParam->dmaReadBufThresholds.clearEmergency >= p_Fm->p_FmDriverParam->dmaReadBufThresholds.assertEmergency)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("dmaReadBufThresholds.clearEmergency must be smaller than dmaReadBufThresholds.assertEmergency"));
    if(p_Fm->p_FmDriverParam->dmaWriteBufThresholds.assertEmergency > DMA_THRESH_MAX_BUF)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("dmaWriteBufThresholds.assertEmergency can not be larger than %d", DMA_THRESH_MAX_BUF));
    if(p_Fm->p_FmDriverParam->dmaWriteBufThresholds.clearEmergency > DMA_THRESH_MAX_BUF)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("dmaWriteBufThresholds.clearEmergency can not be larger than %d", DMA_THRESH_MAX_BUF));
    if(p_Fm->p_FmDriverParam->dmaWriteBufThresholds.clearEmergency >= p_Fm->p_FmDriverParam->dmaWriteBufThresholds.assertEmergency)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("dmaWriteBufThresholds.clearEmergency must be smaller than dmaWriteBufThresholds.assertEmergency"));

    if(!p_Fm->fmClkFreq)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("fmClkFreq must be set."));
    if(USEC_TO_CLK(p_Fm->p_FmDriverParam->dmaWatchdog, p_Fm->fmClkFreq) > DMA_MAX_WATCHDOG)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("dmaWatchdog depends on FM clock. dmaWatchdog(in microseconds) * clk (in Mhz), may not exceed 0xffffffff"));
    for (i=0 ; i<FM_MAX_NUM_OF_PARTITIONS ; i++)
        if(p_Fm->p_FmDriverParam->liodnPerPartition[i] & ~FM_LIODN_MASK)
            RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("liodn number is out of range"));

    if(p_Fm->totalFifoSize % BMI_FIFO_UNITS)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("totalFifoSize number has to be divisible by %d", BMI_FIFO_UNITS));
    if(!p_Fm->totalFifoSize || (p_Fm->totalFifoSize > BMI_MAX_FIFO_SIZE))
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("totalFifoSize number has to be in the range 256 - %d", BMI_MAX_FIFO_SIZE));
    if(!p_Fm->totalNumOfTasks || (p_Fm->totalNumOfTasks > BMI_MAX_NUM_OF_TASKS))
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("totalNumOfTasks number has to be in the range 1 - %d", BMI_MAX_NUM_OF_TASKS));
    if(!p_Fm->maxNumOfOpenDmas || (p_Fm->maxNumOfOpenDmas > BMI_MAX_NUM_OF_DMAS))
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("maxNumOfOpenDmas number has to be in the range 1 - %d", BMI_MAX_NUM_OF_DMAS));

    if(p_Fm->p_FmDriverParam->thresholds.dispLimit > FPM_MAX_DISP_LIMIT)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("thresholds.dispLimit can't be greater than %d", FPM_MAX_DISP_LIMIT));

    if(!p_Fm->f_Exceptions)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Exceptions callback not provided"));
    if(!p_Fm->f_BusError)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Exceptions callback not provided"));

    return E_OK;
}

static uint8_t  GetPartition(t_Fm *p_Fm, uint16_t liodn)
{
    int         i;
    uint32_t    tmpReg;

    for (i=0 ; i<FM_MAX_NUM_OF_PARTITIONS ; i+=2)
    {
        tmpReg = GET_UINT32(p_Fm->p_FmDmaRegs->fmdmplr[i/2]);
        if (liodn == (uint16_t)((tmpReg & DMA_HIGH_LIODN_MASK )>> DMA_LIODN_SHIFT))
            return (uint8_t)i;
        if (liodn == (uint16_t)(tmpReg & DMA_LOW_LIODN_MASK))
            return (uint8_t)(i+1);
    }
    REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("Partition not found in LIODN table"));
    return 0;
}

static void    BmiErrEvent(t_Fm *p_Fm)
{
    uint32_t    event, mask;

    event = GET_UINT32(p_Fm->p_FmBmiRegs->fmbm_ievr);
    mask = GET_UINT32(p_Fm->p_FmBmiRegs->fmbm_ier);
    event &= mask;
    /* clear the acknowledged events */
    WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_ievr, event);

    if(event & BMI_ERR_INTR_EN_PIPELINE_ECC)
        p_Fm->f_Exceptions(p_Fm->h_App,e_FM_EX_BMI_PIPELINE_ECC);
    if(event & BMI_ERR_INTR_EN_LIST_RAM_ECC)
        p_Fm->f_Exceptions(p_Fm->h_App,e_FM_EX_BMI_LIST_RAM_ECC);
    if(event & BMI_ERR_INTR_EN_STATISTICS_RAM_ECC)
        p_Fm->f_Exceptions(p_Fm->h_App,e_FM_EX_BMI_STATISTICS_RAM_ECC);
}

static void    QmiErrEvent(t_Fm *p_Fm)
{
    uint32_t    event, mask;

    event = GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_eie);
    mask = GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_eien);
    event &= mask;
    /* clear the acknowledged events */
    WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_eie, event);

    if(event & QMI_ERR_INTR_EN_DOUBLE_ECC)
        p_Fm->f_Exceptions(p_Fm->h_App,e_FM_EX_QMI_DOUBLE_ECC);
    if(event & QMI_ERR_INTR_EN_DEQ_FROM_DEF)
        p_Fm->f_Exceptions(p_Fm->h_App,e_FM_EX_QMI_DEQ_FROM_DEFQ);
}

static void    DmaErrEvent(t_Fm *p_Fm)
{
    uint64_t            addr=0;
    uint32_t            status, mask, tmpReg=0;
    uint8_t             tnum, partition;
    uint8_t             hardwarePortId;
    uint8_t             relativePortId;

    status = GET_UINT32(p_Fm->p_FmDmaRegs->fmdmsr);
    mask = GET_UINT32(p_Fm->p_FmDmaRegs->fmdmmr);

    /* get bus error regs befor clearing BER */
    if ((status & DMA_STATUS_BUS_ERR) && (mask & DMA_MODE_BER))
    {
        addr = (uint64_t)GET_UINT32(p_Fm->p_FmDmaRegs->fmdmtal);
        addr |= (uint64_t)GET_UINT32(p_Fm->p_FmDmaRegs->fmdmtah) << 32;

        /* get information about the owner of that bus error */
        tmpReg = GET_UINT32(p_Fm->p_FmDmaRegs->fmdmtcid);
    }

    /* clear set events */
    WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmsr, status);

    if ((status & DMA_STATUS_BUS_ERR) && (mask & DMA_MODE_BER))
    {
        ASSERT_COND(p_Fm->h_FmPorts[((tmpReg & DMA_TRANSFER_PORTID_MASK) >> DMA_TRANSFER_PORTID_SHIFT)]);
        hardwarePortId = (uint8_t)(((tmpReg & DMA_TRANSFER_PORTID_MASK) >> DMA_TRANSFER_PORTID_SHIFT));
        GET_RELATIVE_PORTID(relativePortId, hardwarePortId);
        tnum = (uint8_t)((tmpReg & DMA_TRANSFER_TNUM_MASK) >> DMA_TRANSFER_TNUM_SHIFT);
        partition = GetPartition(p_Fm, (uint16_t)(tmpReg & DMA_TRANSFER_LIODN_MASK));
        ASSERT_COND(p_Fm->portsTypes[hardwarePortId] != e_FM_PORT_TYPE_DUMMY);
        p_Fm->f_BusError(p_Fm->h_App, p_Fm->portsTypes[hardwarePortId] , relativePortId, addr, tnum, partition) ;
    }
    if(mask & DMA_MODE_ECC)
    {
        if (status & DMA_STATUS_READ_ECC)
            p_Fm->f_Exceptions(p_Fm->h_App, e_FM_EX_DMA_READ_ECC) ;
        if (status & DMA_STATUS_SYSTEM_WRITE_ECC)
            p_Fm->f_Exceptions(p_Fm->h_App, e_FM_EX_DMA_SYSTEM_WRITE_ECC) ;
        if (status & DMA_STATUS_FM_WRITE_ECC)
            p_Fm->f_Exceptions(p_Fm->h_App, e_FM_EX_DMA_FM_WRITE_ECC) ;
    }
}

static void    FpmErrEvent(t_Fm *p_Fm)
{
    uint32_t    event;

    event = GET_UINT32(p_Fm->p_FmFpmRegs->fpmem);
    /* clear the all occured events */
    WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmem, event);

    if((event  & FPM_EV_MASK_DOUBLE_ECC) && (event & FPM_EV_MASK_DOUBLE_ECC_EN))
        p_Fm->f_Exceptions(p_Fm->h_App,e_FM_EX_FPM_DOUBLE_ECC);
    if((event  & FPM_EV_MASK_STALL) && (event & FPM_EV_MASK_STALL_EN))
        p_Fm->f_Exceptions(p_Fm->h_App,e_FM_EX_FPM_STALL_ON_TASKS);
    if((event  & FPM_EV_MASK_SINGLE_ECC) && (event & FPM_EV_MASK_SINGLE_ECC_EN))
        p_Fm->f_Exceptions(p_Fm->h_App,e_FM_EX_FPM_SINGLE_ECC);
}

static void    MuramErrIntr(t_Fm *p_Fm)
{
    uint32_t    event;

    event = GET_UINT32(p_Fm->p_FmFpmRegs->fmrcr);

    /* clear MURAM event bit */
    WRITE_UINT32(p_Fm->p_FmFpmRegs->fmrcr, event & ~FPM_RAM_CTL_IRAM_ECC);

    ASSERT_COND(event  & FPM_RAM_CTL_MURAM_ECC);
    ASSERT_COND(event  & FPM_RAM_CTL_RAMS_ECC_EN);

    p_Fm->f_Exceptions(p_Fm->h_App, e_FM_EX_MURAM_ECC);
}

static void    IramErrIntr(t_Fm *p_Fm)
{
    uint32_t    event;

    event = GET_UINT32(p_Fm->p_FmFpmRegs->fmrcr) ;
    /* clear the acknowledged events (do not clear IRAM event) */
    WRITE_UINT32(p_Fm->p_FmFpmRegs->fmrcr, event & ~FPM_RAM_CTL_MURAM_ECC );

    ASSERT_COND(event  & FPM_RAM_CTL_IRAM_ECC);
    ASSERT_COND(event  & FPM_RAM_CTL_IRAM_ECC_EN);

    p_Fm->f_Exceptions(p_Fm->h_App, e_FM_EX_IRAM_ECC);
}

static void     QmiEvent(t_Fm *p_Fm)
{
    uint32_t    event, mask;

    event = GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_ie);
    mask = GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_ien);
    event &= mask;
    /* clear the acknowledged events */
    WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_ie, event);

    if(event & QMI_INTR_EN_SINGLE_ECC)
        p_Fm->f_Exceptions(p_Fm->h_App,e_FM_EX_QMI_SINGLE_ECC);
}

static void     FmCtlEvent(t_Fm *p_Fm, uint32_t pending)
{
    uint32_t    eventRegBitMask = FPM_EVENT_FM_CTL_0;
    uint8_t     i;
    uint32_t    event;

    for(i=0;i<NUM_OF_FM_CTL_EVENT_REGS;i++)
    {
       if (pending & eventRegBitMask)
       {
            event = GET_UINT32(p_Fm->p_FmFpmRegs->fpmcev[i]);
            /* clear event bits */
            WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmcev[i], event);
            p_Fm->f_FmCtlIsr[i](p_Fm, event);
       }
       eventRegBitMask >>= 1;
    }
}

static void UnimplementedIsr(t_Handle h_Arg)
{
    UNUSED(h_Arg);

    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Unimplemented Isr!"));
}

static void UnimplementedFmCtlIsr(t_Handle h_Arg, uint32_t event)
{
    UNUSED(h_Arg); UNUSED(event);

    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Unimplemented FmCtl Isr!"));
}

static void FmFreeInitResources(t_Fm *p_Fm)
{
    if (p_Fm->camBaseAddr)
       FM_MURAM_FreeMem(p_Fm->h_FmMuram, CAST_UINT64_TO_POINTER(p_Fm->camBaseAddr));
    if (p_Fm->fifoBaseAddr)
       FM_MURAM_FreeMem(p_Fm->h_FmMuram, CAST_UINT64_TO_POINTER(p_Fm->fifoBaseAddr));
}

static void LoadPatch(t_Fm *p_Fm)
{
    t_FMIramRegs    *p_Iram = CAST_UINT64_TO_POINTER_TYPE(t_FMIramRegs, (p_Fm->baseAddr + FM_MM_IMEM));
    int             i;

    SANITY_CHECK_RETURN(p_Fm, E_INVALID_HANDLE);

    DBG(TRACE, ("Loading firmware to IRAM ..."));

    /* Applying patch to IRAM */
    WRITE_UINT32(p_Iram->iadd, IRAM_IADD_AIE);
    while (GET_UINT32(p_Iram->iadd) != IRAM_IADD_AIE) ;

    for (i=0; i < (p_Fm->p_FmDriverParam->firmware.size / 4); i++)
        WRITE_UINT32(p_Iram->idata, p_Fm->p_FmDriverParam->firmware.p_Code[i]);

    WRITE_UINT32(p_Iram->iadd,0x0);
    /* verify that writing has completed */
    while (GET_UINT32(p_Iram->idata) != p_Fm->p_FmDriverParam->firmware.p_Code[0]) ;

    /* Enable patch from IRAM */
    WRITE_UINT32(p_Iram->iready, IRAM_READY);
}

/****************************************/
/*       Inter-Module functions        */
/****************************************/

#ifdef FM_MASTER_PARTITION
t_Error     FmHandleIpcMsg(t_Handle h_Fm, uint32_t msgId, uint8_t msgBody[MSG_BODY_SIZE])
{
    t_Fm    *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);

    switch(msgId)
    {
        case (FM_GET_SET_PORT_PARAMS):
            return FmGetSetPortParams(h_Fm, (t_FmInterModulePortInitParams*)msgBody);
         case (FM_RESUME_STALLED_PORT):
            return FmResumeStalledPort(h_Fm, msgBody[0]);
        case (FM_IS_PORT_STALLED):
            ((t_FmIpcPortIsStalled*)msgBody)->isStalled = FmIsPortStalled(h_Fm, (uint8_t)(((t_FmIpcPortIsStalled*)msgBody)->hardwarePortId));
            break;
        case (FM_RESET_MAC):
        {
            t_FmIpcMacReset ipcParams;
            memcpy(msgBody, (uint8_t *)&ipcParams, sizeof(t_FmIpcMacReset));
            return FmResetMac(p_Fm, (e_FmMacType)ipcParams.type, ipcParams.id);
        }
        case (FM_FREE_PORT):
            FmFreePortParams(h_Fm, (t_FmInterModulePortFreeParams*)msgBody);
            break;
#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
        case (FM_DUMP_PORT_REGS):
            return FmDumpPortRegs(h_Fm, msgBody[0]);
#endif /* (defined(DEBUG_ERRORS) && ... */
        default:
            RETURN_ERROR(MINOR, E_INVALID_SELECTION, ("command not found!!!"));
    }
    return E_OK;
}
#endif /* FM_MASTER_PARTITION */

uint64_t FmGetPcdPrsBaseAddr(t_Handle h_Fm)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_VALUE(p_Fm, E_INVALID_HANDLE, 0);

    return (p_Fm->baseAddr + FM_MM_PRS);
}

uint64_t FmGetPcdKgBaseAddr(t_Handle h_Fm)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_VALUE(p_Fm, E_INVALID_HANDLE, 0);

    return (p_Fm->baseAddr + FM_MM_KG);
}

uint64_t FmGetPcdPlcrBaseAddr(t_Handle h_Fm)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_VALUE(p_Fm, E_INVALID_HANDLE, 0);

    return (p_Fm->baseAddr + FM_MM_PLCR);
}

t_Handle FmGetMuramHandle(t_Handle h_Fm)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_VALUE(p_Fm, E_INVALID_HANDLE, NULL);

    return (p_Fm->h_FmMuram);
}

t_Error FmGetPhysicalMuramBase(t_Handle h_Fm, t_FmPhysAddr *fmPhysAddr)
{
    t_Fm            *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);

    /* General FM driver initialization */
    fmPhysAddr->low=(uint32_t)p_Fm->fmMuramPhysBaseAddr;
    fmPhysAddr->high=(uint16_t)((p_Fm->fmMuramPhysBaseAddr & 0x0000ffff00000000LL) >> 32);

    return E_OK;
}


void FmRegisterIntr(t_Handle h_Fm,
                        e_FmInterModuleEvent   event,
                        void (*f_Isr) (t_Handle h_Arg),
                        t_Handle    h_Arg)
{
    t_Fm       *p_Fm = (t_Fm*)h_Fm;

    p_Fm->intrMng[event].f_Isr = f_Isr;
    p_Fm->intrMng[event].h_SrcHandle = h_Arg;
}

void  FmRegisterFmCtlIntr(t_Handle h_Fm, uint8_t eventRegId, void (*f_Isr) (t_Handle h_Fm, uint32_t event))
{
    t_Fm       *p_Fm = (t_Fm*)h_Fm;

    p_Fm->f_FmCtlIsr[eventRegId] = f_Isr;
}

void  FmRegisterPcd(t_Handle h_Fm, t_Handle h_FmPcd)
{
    t_Fm       *p_Fm = (t_Fm*)h_Fm;

    if(p_Fm->h_Pcd)
        REPORT_ERROR(MAJOR, E_ALREADY_EXISTS, ("PCD already set"));

    p_Fm->h_Pcd = h_FmPcd;

}

t_Handle  FmGetPcdHandle(t_Handle h_Fm)
{
    t_Fm       *p_Fm = (t_Fm*)h_Fm;

    return p_Fm->h_Pcd;

}

uint8_t FmGetId(t_Handle h_Fm)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_VALUE(p_Fm, E_INVALID_HANDLE, 0xff);

    return p_Fm->fmId;
}

t_Error FmGetSetPortParams(t_Handle h_Fm,t_FmInterModulePortInitParams *p_PortParams)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;
    uint32_t    tmpReg;
    uint8_t     hardwarePortId = p_PortParams->hardwarePortId;
    uint8_t     enqTh;
    uint8_t     deqTh;
    bool        update = FALSE;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);

    /* return parameters */
    p_PortParams->timeStampPeriod = p_Fm->timeStampPeriod;

    if(p_PortParams->independentMode)
    {
        /* set port parameters */
        p_Fm->independentMode = p_PortParams->independentMode;
        /* disable dispatch limit */
        WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmflc, 0);
    }

    if(p_PortParams->portType == e_FM_PORT_TYPE_HOST_COMMAND)
    {
        if(p_Fm->hcPortInitialized)
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Only one host command port is allowed."));
        else
            p_Fm->hcPortInitialized = TRUE;
    }
    p_Fm->portsTypes[hardwarePortId] = p_PortParams->portType;

    /* check that there are enough uncommited tasks */
    if(p_PortParams->numOfExtraTasks > p_Fm->extraTasksPoolSize)
        p_Fm->extraTasksPoolSize = p_PortParams->numOfExtraTasks;

    if((p_Fm->accumulatedNumOfTasks + p_PortParams->numOfTasks) > p_Fm->totalNumOfTasks-p_Fm->extraTasksPoolSize)
        RETURN_ERROR(MAJOR, E_NOT_AVAILABLE, ("Requested numOfTasks and extra tasks pool exceed total numOfTasks."));
    else
    {
        p_Fm->accumulatedNumOfTasks += p_PortParams->numOfTasks;
        tmpReg = (uint32_t)(((p_PortParams->numOfTasks-1) << BMI_NUM_OF_TASKS_SHIFT) |
                    (p_PortParams->numOfExtraTasks << BMI_EXTRA_NUM_OF_TASKS_SHIFT));
        WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_pp[hardwarePortId-1],tmpReg);
    }

    if((p_PortParams->portType != e_FM_PORT_TYPE_RX) && (p_PortParams->portType != e_FM_PORT_TYPE_RX_10G))
    /* for transmit & O/H ports */
    {
        /* update qmi ENQ/DEQ threshold */
        p_Fm->accumulatedNumOfDeqTnums += p_PortParams->deqPipelineDepth;
        tmpReg = GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_gc);
        enqTh = (uint8_t)(tmpReg>>8);
        /* if enqTh is too big, we reduce it to the max value that is still OK */
        if(enqTh >= (QMI_MAX_NUM_OF_TNUMS - p_Fm->accumulatedNumOfDeqTnums))
        {
            enqTh = (uint8_t)(QMI_MAX_NUM_OF_TNUMS - p_Fm->accumulatedNumOfDeqTnums - 1);
            tmpReg &= ~QMI_CFG_ENQ_MASK;
            tmpReg |= ((uint32_t)enqTh << 8);
            update = TRUE;
        }

        deqTh = (uint8_t)tmpReg;
        /* if deqTh is too small, we enlarge it to the min value that is still OK.
         deqTh may not be larger than 63 (QMI_MAX_NUM_OF_TNUMS-1). */
        if((deqTh <= p_Fm->accumulatedNumOfDeqTnums)  && (deqTh < QMI_MAX_NUM_OF_TNUMS-1))
        {
            deqTh = (uint8_t)(p_Fm->accumulatedNumOfDeqTnums + 1);
            tmpReg &= ~QMI_CFG_DEQ_MASK;
            tmpReg |= (uint32_t)deqTh;
            update = TRUE;
        }
        if(update)
            WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_gc, tmpReg);
    }

    /* check that there are enough uncommited open DMA's */
    if(p_PortParams->numOfExtraOpenDmas > p_Fm->extraOpenDmasPoolSize)
        p_Fm->extraOpenDmasPoolSize = p_PortParams->numOfExtraOpenDmas;

    if((p_Fm->accumulatedNumOfOpenDmas + p_PortParams->numOfOpenDmas) > p_Fm->maxNumOfOpenDmas)
        RETURN_ERROR(MAJOR, E_NOT_AVAILABLE, ("Requested numOfOpenDmas exceeds total numOfTasks."));
    else
    {
        p_Fm->accumulatedNumOfOpenDmas += p_PortParams->numOfOpenDmas;
        tmpReg = (uint32_t)(((p_PortParams->numOfOpenDmas-1) << BMI_NUM_OF_DMAS_SHIFT) |
                    (p_PortParams->numOfExtraOpenDmas << BMI_EXTRA_NUM_OF_DMAS_SHIFT));
        WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_pp[hardwarePortId-1],
                GET_UINT32(p_Fm->p_FmBmiRegs->fmbm_pp[hardwarePortId-1]) | tmpReg);
        /* update total num of DMA's with committed number of open DMAS, and max uncommitted pool. */
        tmpReg = GET_UINT32(p_Fm->p_FmBmiRegs->fmbm_cfg2) & ~BMI_CFG2_DMAS_MASK;
        tmpReg |= (uint32_t)(p_Fm->accumulatedNumOfOpenDmas + p_Fm->extraOpenDmasPoolSize - 1) << BMI_CFG2_DMAS_SHIFT;
        WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_cfg2,  tmpReg);
    }

    /* we leave FM_MAX_NUM_OF_RX_PORTS spare bufferes for excessive buffers */
        /* check that there are enough uncommited tasks */
    if(p_PortParams->extraSizeOfFifo > p_Fm->extraFifoPoolSize)
        p_Fm->extraTasksPoolSize = p_PortParams->numOfExtraTasks;

    if((p_Fm->accumulatedFifoSize + p_PortParams->sizeOfFifo) > (p_Fm->totalFifoSize - p_Fm->extraTasksPoolSize))
        RETURN_ERROR(MAJOR, E_NOT_AVAILABLE, ("Requested fifo size and extra size exceed total fifo size."));
    else
    {
        p_Fm->accumulatedFifoSize += p_PortParams->sizeOfFifo;
        tmpReg = (uint32_t)((p_PortParams->sizeOfFifo/BMI_FIFO_UNITS - 1) | ((p_PortParams->extraSizeOfFifo/BMI_FIFO_UNITS) << BMI_FIFO_SIZE_SHIFT));
        WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_pfs[hardwarePortId-1], tmpReg);
    }
    WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_ppid[hardwarePortId-1], (uint32_t)p_PortParams->portPartition);

    tmpReg = (uint32_t)(hardwarePortId << FPM_PORT_FM_CTL_PORTID_SHIFT);
    if(p_PortParams->independentMode)
    {
        if((p_PortParams->portType==e_FM_PORT_TYPE_RX) || (e_FM_PORT_TYPE_RX_10G))
            tmpReg |= (FPM_PORT_FM_CTL1 << FPM_PRC_ORA_FM_CTL_SEL_SHIFT) |FPM_PORT_FM_CTL1;
        else
            tmpReg |= (FPM_PORT_FM_CTL2 << FPM_PRC_ORA_FM_CTL_SEL_SHIFT) |FPM_PORT_FM_CTL2;
    }
    else
    {
        tmpReg |= (FPM_PORT_FM_CTL2|FPM_PORT_FM_CTL1);

        /* order restoration */
        if(hardwarePortId%2)
            tmpReg |= (FPM_PORT_FM_CTL1 << FPM_PRC_ORA_FM_CTL_SEL_SHIFT);
        else
            tmpReg |= (FPM_PORT_FM_CTL2 << FPM_PRC_ORA_FM_CTL_SEL_SHIFT);
    }
    WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmpr, tmpReg);

    FmGetPhysicalMuramBase(p_Fm, &p_PortParams->fmMuramPhysBaseAddr);

    return E_OK;
}

void FmFreePortParams(t_Handle h_Fm,t_FmInterModulePortFreeParams *p_PortParams)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;
    uint32_t    tmpReg;
    uint8_t     hardwarePortId = p_PortParams->hardwarePortId;
    uint8_t     enqTh;
    uint8_t     deqTh;
    uint8_t     numOfTasks;

    SANITY_CHECK_RETURN(p_Fm, E_INVALID_HANDLE);

    if(p_PortParams->portType == e_FM_PORT_TYPE_HOST_COMMAND)
    {
        ASSERT_COND(p_Fm->hcPortInitialized);
        p_Fm->hcPortInitialized = FALSE;
    }

    p_Fm->portsTypes[hardwarePortId] = e_FM_PORT_TYPE_DUMMY;

    tmpReg = GET_UINT32(p_Fm->p_FmBmiRegs->fmbm_pp[hardwarePortId-1]);
    /* free numOfTasks */
    numOfTasks = (uint8_t)(((tmpReg & BMI_NUM_OF_TASKS_MASK) >> BMI_NUM_OF_TASKS_SHIFT) + 1);
    ASSERT_COND(p_Fm->accumulatedNumOfTasks >= numOfTasks);
    p_Fm->accumulatedNumOfTasks -= numOfTasks;

    /* free numOfOpenDmas */
    ASSERT_COND(p_Fm->accumulatedNumOfOpenDmas >= ((tmpReg & BMI_NUM_OF_DMAS_MASK) >> BMI_NUM_OF_DMAS_SHIFT));
    p_Fm->accumulatedNumOfOpenDmas -= ((tmpReg & BMI_NUM_OF_DMAS_MASK) >> BMI_NUM_OF_DMAS_SHIFT);
    tmpReg = GET_UINT32(p_Fm->p_FmBmiRegs->fmbm_pfs[hardwarePortId-1]);
    /* free sizeOfFifo */
    ASSERT_COND(p_Fm->accumulatedFifoSize >= ((tmpReg & BMI_FIFO_SIZE_MASK) + 1)*BMI_FIFO_UNITS);
    p_Fm->accumulatedFifoSize -= ((tmpReg & BMI_FIFO_SIZE_MASK) + 1)*BMI_FIFO_UNITS;

    /* clear registers */
    WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_pp[hardwarePortId-1], 0);
    WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_pfs[hardwarePortId-1], 0);
    //WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_ppid[hardwarePortId-1], 0);

    if((p_PortParams->portType != e_FM_PORT_TYPE_RX) && (p_PortParams->portType != e_FM_PORT_TYPE_RX_10G))
    /* for transmit & O/H ports */
    {
        tmpReg = 0;
        /* update qmi ENQ/DEQ threshold */
        p_Fm->accumulatedNumOfDeqTnums -= p_PortParams->deqPipelineDepth;

        /* p_Fm->accumulatedNumOfDeqTnums is now smaller,
           so we can enlarge enqTh */
        enqTh = (uint8_t)(QMI_MAX_NUM_OF_TNUMS - p_Fm->accumulatedNumOfDeqTnums - 1);
        tmpReg &= ~QMI_CFG_ENQ_MASK;
        tmpReg |= ((uint32_t)enqTh << QMI_CFG_ENQ_SHIFT);

         /* p_Fm->accumulatedNumOfDeqTnums is now smaller,
           so we can reduce enqTh */
        deqTh = (uint8_t)(p_Fm->accumulatedNumOfDeqTnums + 1);
        tmpReg &= ~QMI_CFG_DEQ_MASK;
        tmpReg |= (uint32_t)deqTh;

        WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_gc, tmpReg);
    }

    return;
}


bool FmIsPortStalled(t_Handle h_Fm, uint8_t hardwarePortId)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;
    uint32_t    tmpReg;

    SANITY_CHECK_RETURN_VALUE(p_Fm, E_INVALID_HANDLE, FALSE);

    tmpReg = GET_UINT32(p_Fm->p_FmFpmRegs->fmfp_ps[hardwarePortId]);

    return (bool)!!(tmpReg & FPM_PS_STALLED);
}

t_Error FmResumeStalledPort(t_Handle h_Fm, uint8_t hardwarePortId)
{
    t_Fm                    *p_Fm = (t_Fm*)h_Fm;
    uint32_t                tmpReg;

    /* Get port status */
    if (!FmIsPortStalled(h_Fm, hardwarePortId))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Port is not stalled"));

    tmpReg = (uint32_t)((hardwarePortId << FPM_PORT_FM_CTL_PORTID_SHIFT) | FPM_PRC_REALSE_STALLED);
    WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmpr, tmpReg);

    return E_OK;
}

t_Error FmResetMac(t_Handle h_Fm, e_FmMacType type, uint8_t macId)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;
    uint32_t    bitMask;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);

    /* Get the relevant bit mask */
    if (type == e_FM_MAC_10G)
    {
        switch(macId)
        {
            case(0):
                bitMask = FPM_RSTC_10G0_RESET;
                break;
            default:
                RETURN_ERROR(MINOR, E_INVALID_VALUE, ("Illegal MAC Id"));
               break;
        }
    }
    else
    {
        switch(macId)
        {
            case(0):
                bitMask = FPM_RSTC_1G0_RESET;
                break;
            case(1):
                bitMask = FPM_RSTC_1G1_RESET;
                break;
            case(2):
                bitMask = FPM_RSTC_1G2_RESET;
                break;
            case(3):
                bitMask = FPM_RSTC_1G3_RESET;
                break;
            default:
                RETURN_ERROR(MINOR, E_INVALID_VALUE, ("Illegal MAC Id"));
                break;
        }
    }

    /* reset */
    WRITE_UINT32(p_Fm->p_FmFpmRegs->fmrstc, bitMask);

    return E_OK;
}

uint32_t    FmGetTimeStampPeriod(t_Handle h_Fm)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_VALUE(p_Fm, E_INVALID_HANDLE, 0);
    SANITY_CHECK_RETURN_VALUE(p_Fm->timeStampPeriod, E_INVALID_HANDLE, 0);

    return p_Fm->timeStampPeriod;
}

bool FmRamsEccIsExternalCtl(t_Handle h_Fm)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;
    uint32_t    tmpReg;

    tmpReg = GET_UINT32(p_Fm->p_FmFpmRegs->fmrcr);
    if(tmpReg & FPM_RAM_CTL_RAMS_ECC_EN_SRC_SEL)
        return TRUE;
    else
        return FALSE;
}

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
t_Error FmDumpPortRegs (t_Handle h_Fm,uint8_t hardwarePortId)
{
    t_Fm *p_Fm = (t_Fm *)h_Fm;

    DECLARE_DUMP;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);

    DUMP_TITLE(&p_Fm->p_FmBmiRegs->fmbm_pp[hardwarePortId-1], ("fmbm_pp for port %d", (hardwarePortId)));
    DUMP_MEMORY(&p_Fm->p_FmBmiRegs->fmbm_pp[hardwarePortId-1], sizeof(uint32_t));

    DUMP_TITLE(&p_Fm->p_FmBmiRegs->fmbm_pfs[hardwarePortId-1], ("fmbm_pfs for port %d", (hardwarePortId )));
    DUMP_MEMORY(&p_Fm->p_FmBmiRegs->fmbm_pfs[hardwarePortId-1], sizeof(uint32_t));

    DUMP_TITLE(&p_Fm->p_FmBmiRegs->fmbm_ppid[hardwarePortId-1], ("bm_ppid for port %d", (hardwarePortId)));
    DUMP_MEMORY(&p_Fm->p_FmBmiRegs->fmbm_ppid[hardwarePortId-1], sizeof(uint32_t));

    return E_OK;
}
#endif /* (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0)) */

/****************************************/
/*       API Init unit functions        */
/****************************************/
t_Handle FM_Config(t_FmParams *p_FmParam)
{
    t_Fm        *p_Fm;
    uint8_t     i;
    uint64_t    baseAddr = p_FmParam->baseAddr;

    SANITY_CHECK_RETURN_VALUE(((p_FmParam->firmware.p_Code && p_FmParam->firmware.size) ||
                               (!p_FmParam->firmware.p_Code && !p_FmParam->firmware.size)),
                              E_INVALID_VALUE, NULL);

    /* Allocate FM structure */
    p_Fm = (t_Fm *) XX_Malloc(sizeof(t_Fm));
    if (!p_Fm)
    {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FM driver structure"));
        return NULL;
    }
    memset(p_Fm, 0, sizeof(t_Fm));

    /* Allocate the FM driver's parameters structure */
    p_Fm->p_FmDriverParam = (t_FmDriverParam *)XX_Malloc(sizeof(t_FmDriverParam));
    if (!p_Fm->p_FmDriverParam)
    {
        XX_Free(p_Fm);
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FM driver parameters"));
        return NULL;
    }
    memset(p_Fm->p_FmDriverParam, 0, sizeof(t_FmDriverParam));

    /* Initialize FM parameters which will be kept by the driver */
    p_Fm->fmId              = p_FmParam->fmId;
    p_Fm->h_FmMuram         = p_FmParam->h_FmMuram;
    p_Fm->h_App             = p_FmParam->h_App;
    p_Fm->fmClkFreq         = p_FmParam->fmClkFreq;
    p_Fm->f_Exceptions      = p_FmParam->f_Exceptions;
    p_Fm->f_BusError        = p_FmParam->f_BusError;
    p_Fm->p_FmFpmRegs       = CAST_UINT64_TO_POINTER_TYPE(t_FmFpmRegs, (baseAddr + FM_MM_FPM));
    p_Fm->p_FmBmiRegs       = CAST_UINT64_TO_POINTER_TYPE(t_FmBmiRegs, (baseAddr + FM_MM_BMI));
    p_Fm->p_FmQmiRegs       = CAST_UINT64_TO_POINTER_TYPE(t_FmQmiRegs, (baseAddr + FM_MM_QMI));
    p_Fm->p_FmDmaRegs       = CAST_UINT64_TO_POINTER_TYPE(t_FmDmaRegs, (baseAddr + FM_MM_DMA));
    p_Fm->baseAddr          = baseAddr;
    p_Fm->irq               = p_FmParam->irq;
    p_Fm->errIrq            = p_FmParam->errIrq;
    p_Fm->hcPortInitialized = FALSE;
    p_Fm->independentMode   = FALSE;
    p_Fm->ramsEccEnable     = FALSE;
    p_Fm->totalNumOfTasks   = DEFAULT_totalNumOfTasks;
    p_Fm->totalFifoSize     = DEFAULT_totalFifoSize;
    p_Fm->maxNumOfOpenDmas  = DEFAULT_maxNumOfOpenDmas;
    p_Fm->extraFifoPoolSize     = FM_MAX_NUM_OF_RX_PORTS*BMI_FIFO_UNITS;

    p_Fm->exceptions        = DEFAULT_exceptions;
    for(i = 0;i<FM_MAX_NUM_OF_PORTS;i++)
        p_Fm->portsTypes[i] = e_FM_PORT_TYPE_DUMMY;
    /* Initialize FM driver parameters parameters (for initialization phase only) */
    memcpy(p_Fm->p_FmDriverParam->liodnPerPartition, p_FmParam->liodnPerPartition, FM_MAX_NUM_OF_PARTITIONS);

    /*p_Fm->p_FmDriverParam->numOfPartitions                      = p_FmParam->numOfPartitions;    */
    p_Fm->p_FmDriverParam->enCounters                           = FALSE;

    p_Fm->p_FmDriverParam->resetOnInit                          = DEFAULT_resetOnInit;

    p_Fm->p_FmDriverParam->thresholds.dispLimit                 = DEFAULT_dispLimit;
    p_Fm->p_FmDriverParam->thresholds.prsDispTh                 = DEFAULT_prsDispTh;
    p_Fm->p_FmDriverParam->thresholds.plcrDispTh                = DEFAULT_plcrDispTh;
    p_Fm->p_FmDriverParam->thresholds.kgDispTh                  = DEFAULT_kgDispTh;
    p_Fm->p_FmDriverParam->thresholds.bmiDispTh                 = DEFAULT_bmiDispTh;
    p_Fm->p_FmDriverParam->thresholds.qmiEnqDispTh              = DEFAULT_qmiEnqDispTh;
    p_Fm->p_FmDriverParam->thresholds.qmiDeqDispTh              = DEFAULT_qmiDeqDispTh;
    p_Fm->p_FmDriverParam->thresholds.fmCtl1DispTh               = DEFAULT_fmCtl1DispTh;
    p_Fm->p_FmDriverParam->thresholds.fmCtl2DispTh               = DEFAULT_fmCtl2DispTh;

    p_Fm->p_FmDriverParam->enTimeStamp                          = FALSE;

    p_Fm->p_FmDriverParam->dmaStopOnBusError                    = DEFAULT_dmaStopOnBusError;
    p_Fm->p_FmDriverParam->dmaBusProtect.privilegeBusProtect    = DEFAULT_privilegeBusProtect;
    p_Fm->p_FmDriverParam->dmaBusProtect.busProtectType         = DEFAULT_busProtectionType;

    p_Fm->p_FmDriverParam->dmaCacheOverride                     = DEFAULT_cacheOverride;
    p_Fm->p_FmDriverParam->dmaAidMode                           = DEFAULT_aidMode;
    p_Fm->p_FmDriverParam->dmaAidOverride                       = DEFAULT_aidOverride;
    p_Fm->p_FmDriverParam->dmaAxiDbgNumOfBeats                  = DEFAULT_axiDbgNumOfBeats;
    p_Fm->p_FmDriverParam->dmaCamNumOfEntries                   = DEFAULT_dmaCamNumOfEntries;
    if(p_Fm->fmClkFreq)
        p_Fm->p_FmDriverParam->dmaWatchdog                      = 0xffffffff/p_Fm->fmClkFreq; /* max possible */
    else
    {
        XX_Free(p_Fm->p_FmDriverParam);
        XX_Free(p_Fm);
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("fmClkFreq can't be '0'"));
        return NULL;
    }
    p_Fm->p_FmDriverParam->dmaCommQThresholds.clearEmergency               = DEFAULT_dmaCommQLow;
    p_Fm->p_FmDriverParam->dmaCommQThresholds.assertEmergency              = DEFAULT_dmaCommQHigh;
    p_Fm->p_FmDriverParam->dmaReadBufThresholds.clearEmergency             = DEFAULT_dmaReadIntBufLow;
    p_Fm->p_FmDriverParam->dmaReadBufThresholds.assertEmergency            = DEFAULT_dmaReadIntBufHigh;
    p_Fm->p_FmDriverParam->dmaWriteBufThresholds.clearEmergency            = DEFAULT_dmaWriteIntBufLow;
    p_Fm->p_FmDriverParam->dmaWriteBufThresholds.assertEmergency           = DEFAULT_dmaWriteIntBufHigh;
    p_Fm->p_FmDriverParam->dmaSosEmergency                      = DEFAULT_dmaSosEmergency;

    p_Fm->p_FmDriverParam->dmaDbgCntMode                        = DEFAULT_dmaDbgCntMode;

    p_Fm->p_FmDriverParam->dmaEnEmergency                       = FALSE;
    p_Fm->p_FmDriverParam->dmaEnEmergencySmoother               = FALSE;
    p_Fm->p_FmDriverParam->catastrophicErr                      = DEFAULT_catastrophicErr;
    p_Fm->p_FmDriverParam->dmaErr                               = DEFAULT_dmaErr;
    p_Fm->p_FmDriverParam->haltOnExternalActivation             = DEFAULT_haltOnExternalActivation;
    p_Fm->p_FmDriverParam->haltOnUnrecoverableEccError          = DEFAULT_haltOnUnrecoverableEccError;

    p_Fm->p_FmDriverParam->enIramTestMode                       = FALSE;
    p_Fm->p_FmDriverParam->enMuramTestMode                      = FALSE;
    p_Fm->p_FmDriverParam->externalEccRamsEnable                = DEFAULT_externalEccRamsEnable;

     p_Fm->p_FmDriverParam->firmware.size = p_FmParam->firmware.size;
     if (p_Fm->p_FmDriverParam->firmware.size)
     {
         p_Fm->p_FmDriverParam->firmware.p_Code = (uint32_t *)XX_Malloc(p_Fm->p_FmDriverParam->firmware.size);
        if (!p_Fm->p_FmDriverParam->firmware.p_Code)
        {
            XX_Free(p_Fm->p_FmDriverParam);
            XX_Free(p_Fm);
            REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FM firmware code"));
            return NULL;
        }
         memcpy(p_Fm->p_FmDriverParam->firmware.p_Code ,p_FmParam->firmware.p_Code ,p_Fm->p_FmDriverParam->firmware.size);
     }

#ifdef CONFIG_GUEST_PARTITION
    /* register to inter-core messaging mechanism */
    memset(p_Fm->fmModuleName, 0, MODULE_NAME_SIZE);
    if(Sprint (p_Fm->fmModuleName, "FM-%d",p_Fm->fmId) != 4)
    {
        XX_Free(p_Fm->p_FmDriverParam);
        XX_Free(p_Fm);
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("Sprint failed"));
        return NULL;
    }
#endif /* CONFIG_GUEST_PARTITION */

    return p_Fm;
}

/**************************************************************************//**
 @Function      FM_Init

 @Description   Initializes the FM module

 @Param[in]     h_Fm - FM module descriptor

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
t_Error FM_Init(t_Handle h_Fm)
{
    t_Fm                    *p_Fm = (t_Fm*)h_Fm;
    t_FmDriverParam         *p_FmDriverParam = NULL;
#ifdef FM_MASTER_PARTITION
    t_Error                 err;
#endif /* FM_MASTER_PARTITION */
    uint32_t                tmpReg, cfgReg = 0;
    int                     i;
    uint64_t                fraction;
    uint32_t                prescalar, integer, period;

    SANITY_CHECK_RETURN_ERROR(h_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    CHECK_INIT_PARAMETERS(p_Fm, CheckFmParameters);

    p_FmDriverParam = p_Fm->p_FmDriverParam;

    if(p_FmDriverParam->resetOnInit)
        WRITE_UINT32(p_Fm->p_FmFpmRegs->fmrstc, FPM_RSTC_FM_RESET);

    /**********************/
    /* Load patch to Iram */
    /**********************/
    if (p_Fm->p_FmDriverParam->firmware.p_Code)
        LoadPatch(p_Fm);

    /* General FM driver initialization */
    p_Fm->fmMuramPhysBaseAddr = CAST_POINTER_TO_UINT64(XX_VirtToPhys(CAST_UINT64_TO_POINTER(p_Fm->baseAddr + FM_MM_MURAM)));
    for(i=0;i<e_FM_EV_DUMMY_LAST;i++)
        p_Fm->intrMng[i].f_Isr = UnimplementedIsr;
    for(i=0;i<NUM_OF_FM_CTL_EVENT_REGS;i++)
        p_Fm->f_FmCtlIsr[i] = UnimplementedFmCtlIsr;

    /**********************/
    /* Init DMA Registers */
    /**********************/
    /* clear status reg events */
    tmpReg = (DMA_STATUS_BUS_ERR | DMA_STATUS_READ_ECC | DMA_STATUS_SYSTEM_WRITE_ECC | DMA_STATUS_FM_WRITE_ECC);
  //  tmpReg |= (DMA_STATUS_SYSTEM_DPEXT_ECC | DMA_STATUS_FM_DPEXT_ECC | DMA_STATUS_SYSTEM_DPDAT_ECC | DMA_STATUS_FM_DPDAT_ECC | DMA_STATUS_FM_SPDAT_ECC);
    WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmsr, GET_UINT32(p_Fm->p_FmDmaRegs->fmdmsr) | tmpReg);

    /* configure mode register */
    tmpReg = 0;
    tmpReg |= p_FmDriverParam->dmaCacheOverride << DMA_MODE_CACHE_OR_SHIFT;
    if(p_FmDriverParam->dmaAidOverride)
    {
        tmpReg |= DMA_MODE_AID_OR;
    }
    if (p_Fm->exceptions & FM_EX_DMA_BUS_ERROR)
    {
        tmpReg |= DMA_MODE_BER;
    }
    if ((p_Fm->exceptions & FM_EX_DMA_SYSTEM_WRITE_ECC) | (p_Fm->exceptions & FM_EX_DMA_READ_ECC) | (p_Fm->exceptions & FM_EX_DMA_FM_WRITE_ECC))
    {
        tmpReg |= DMA_MODE_ECC;
    }
    if(p_FmDriverParam->dmaStopOnBusError)
        tmpReg |= DMA_MODE_SBER;
    tmpReg |= (uint32_t)(p_FmDriverParam->dmaAxiDbgNumOfBeats - 1) << DMA_MODE_AXI_DBG_SHIFT;
    if (p_FmDriverParam->dmaEnEmergency)
    {
        tmpReg |= p_FmDriverParam->dmaEmergency.emergencyBusSelect;
        tmpReg |= p_FmDriverParam->dmaEmergency.emergencyLevel << DMA_MODE_EMERGENCY_LEVEL_SHIFT;
        if(p_FmDriverParam->dmaEnEmergencySmoother)
            WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmemsr, p_FmDriverParam->dmaEmergencySwitchCounter);
     }
    tmpReg |= ((p_FmDriverParam->dmaCamNumOfEntries/DMA_CAM_UNITS) - 1) << DMA_MODE_CEN_SHIFT;

    if(p_FmDriverParam->dmaBusProtect.privilegeBusProtect)
        tmpReg |= DMA_MODE_PRIVILEGE_PROT;
    tmpReg |= DMA_MODE_SECURE_PROT;
    tmpReg |= p_FmDriverParam->dmaBusProtect.busProtectType << DMA_MODE_BUS_PROT_SHIFT;
    tmpReg |= p_FmDriverParam->dmaDbgCntMode << DMA_MODE_DBG_SHIFT;
    tmpReg |= p_FmDriverParam->dmaAidMode << DMA_MODE_AID_MODE_SHIFT;

    WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmmr, tmpReg);

    /* configure thresholds register */
    tmpReg = ((uint32_t)p_FmDriverParam->dmaCommQThresholds.assertEmergency << DMA_THRESH_COMMQ_SHIFT) |
                ((uint32_t)p_FmDriverParam->dmaReadBufThresholds.assertEmergency << DMA_THRESH_READ_INT_BUF_SHIFT) |
                ((uint32_t)p_FmDriverParam->dmaWriteBufThresholds.assertEmergency);
    WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmtr, tmpReg);

    /* configure hysteresis register */
    tmpReg = ((uint32_t)p_FmDriverParam->dmaCommQThresholds.clearEmergency << DMA_THRESH_COMMQ_SHIFT) |
                ((uint32_t)p_FmDriverParam->dmaReadBufThresholds.clearEmergency << DMA_THRESH_READ_INT_BUF_SHIFT) |
                ((uint32_t)p_FmDriverParam->dmaWriteBufThresholds.clearEmergency);
    WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmhy, tmpReg);

    /* configure emergency threshold */
    WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmsetr, p_FmDriverParam->dmaSosEmergency);

    /* configure Watchdog */
    WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmwcr, USEC_TO_CLK(p_FmDriverParam->dmaWatchdog, p_Fm->fmClkFreq));

    /* Allocate MURAM for CAM */
    p_Fm->camBaseAddr = CAST_POINTER_TO_UINT64(FM_MURAM_AllocMem(p_Fm->h_FmMuram,
                                                                 (uint32_t)(p_FmDriverParam->dmaCamNumOfEntries*DMA_CAM_SIZEOF_ENTRY),
                                                                 DMA_CAM_ALIGN));
    if (!p_Fm->camBaseAddr )
        RETURN_ERROR(MAJOR, E_NO_MEMORY, ("MURAM alloc for DMA CAM failed"));

#ifndef VERIFICATION_SUPPORT
    WRITE_BLOCK(CAST_UINT64_TO_POINTER_TYPE(uint8_t, p_Fm->camBaseAddr), 0, (uint32_t)(p_FmDriverParam->dmaCamNumOfEntries*DMA_CAM_SIZEOF_ENTRY));
#endif /* VERIFICATION_SUPPORT */
    /* VirtToPhys */
    WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmebcr,
                 (uint32_t)(CAST_POINTER_TO_UINT64(XX_VirtToPhys(CAST_UINT64_TO_POINTER(p_Fm->camBaseAddr))) -
                            p_Fm->fmMuramPhysBaseAddr));

    /* liodn-partitions */
    for (i=0 ; i<FM_MAX_NUM_OF_PARTITIONS ; i+=2)
    {
        tmpReg = (((uint32_t)p_FmDriverParam->liodnPerPartition[i] << DMA_LIODN_SHIFT) |
                    (uint32_t)p_FmDriverParam->liodnPerPartition[i+1]);
        WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmplr[i/2], tmpReg);
    }

    /**********************/
    /* Init FPM Registers */
    /**********************/
    tmpReg = (uint32_t)(p_FmDriverParam->thresholds.dispLimit << FPM_DISP_LIMIT_SHIFT);
    WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmflc, tmpReg);

    tmpReg =   (((uint32_t)p_FmDriverParam->thresholds.prsDispTh  << FPM_THR1_PRS_SHIFT) |
                ((uint32_t)p_FmDriverParam->thresholds.kgDispTh  << FPM_THR1_KG_SHIFT) |
                ((uint32_t)p_FmDriverParam->thresholds.plcrDispTh  << FPM_THR1_PLCR_SHIFT) |
                ((uint32_t)p_FmDriverParam->thresholds.bmiDispTh  << FPM_THR1_BMI_SHIFT));
    WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmdis1, tmpReg);

    tmpReg =   (((uint32_t)p_FmDriverParam->thresholds.qmiEnqDispTh  << FPM_THR2_QMI_ENQ_SHIFT) |
                ((uint32_t)p_FmDriverParam->thresholds.qmiDeqDispTh  << FPM_THR2_QMI_DEQ_SHIFT) |
                ((uint32_t)p_FmDriverParam->thresholds.fmCtl1DispTh  << FPM_THR2_FM_CTL1_SHIFT) |
                ((uint32_t)p_FmDriverParam->thresholds.fmCtl2DispTh  << FPM_THR2_FM_CTL2_SHIFT));
    WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmdis2, tmpReg);

    /* define exceptions and error behavior */
    tmpReg = 0;
    /* Clear events */
    tmpReg |= (FPM_EV_MASK_STALL | FPM_EV_MASK_DOUBLE_ECC | FPM_EV_MASK_SINGLE_ECC);
    /* enable interrupts */
    if(p_Fm->exceptions & FM_EX_FPM_STALL_ON_TASKS)
    {
        tmpReg |= FPM_EV_MASK_STALL_EN;
    }
    if(p_Fm->exceptions & FM_EX_FPM_SINGLE_ECC)
    {
        tmpReg |= FPM_EV_MASK_SINGLE_ECC_EN;
    }
    if(p_Fm->exceptions & FM_EX_FPM_DOUBLE_ECC)
    {
        tmpReg |= FPM_EV_MASK_DOUBLE_ECC_EN ;
    }
    tmpReg |= (p_Fm->p_FmDriverParam->catastrophicErr  << FPM_EV_MASK_CAT_ERR_SHIFT);
    tmpReg |= (p_Fm->p_FmDriverParam->dmaErr << FPM_EV_MASK_DMA_ERR_SHIFT);
    if(p_Fm->p_FmDriverParam->haltOnExternalActivation)
        tmpReg |= FPM_EV_MASK_EXTERNAL_HALT;
    if(p_Fm->p_FmDriverParam->haltOnUnrecoverableEccError)
        tmpReg |= FPM_EV_MASK_ECC_ERR_HALT;
    WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmem, tmpReg);

    /* clear all fmCtls event registers */
    for(i=0;i<NUM_OF_FM_CTL_EVENT_REGS;i++)
        WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmcev[i], 0xFFFFFFFF);

    /* timeStamp */
    if(p_FmDriverParam->enTimeStamp)
    {
        period = p_Fm->timeStampPeriod;

        /* calculate the prescalar, considering fmClkFreq is in Mhz, and
        timeStampPeriod is in nanoseconds */
        prescalar = (period * p_Fm->fmClkFreq)/1000;
        if(!prescalar)
            RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("timeStampPeriod is too small"));
        if(prescalar > 256)
            RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("timeStampPeriod is too large"));
        WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmtsc1, (uint32_t)((prescalar - 1) | FPM_TS_CTL_EN));

        /* the FM HW allows to increase precision by enlarging timeStamp value by value
        different than 1, possibly by fraction. */
        integer  = (prescalar * 1000)/ (period * p_Fm->fmClkFreq); /* always 0 */
        if(integer > 255)
            RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("timeStampPeriod  is too large"));

        /* Since the prescalar may have been be rounded down, we increase the timeStamp by
        value smaller than the resolution required (and smaller than 1 when resolution is normally 1).
        Here we calculate the fraction that will give us the nearest result.
        Since we prefer not to use floating variables, we need to multiply by a large factor in
        order to get a precise enough number. Since the HW implementation uses 24-bit-fixed-point
        representation, a 16 bit fraction is used. In order to calculate the fraction value, we should
        multiply the number by 2^16. We therefor use the 2^16 also as the factor for
        the multiplication, and do not use another one for enlarging the fraction. */
        fraction = ((uint64_t)((uint64_t)(prescalar * 1000) << 16)/ (period * p_Fm->fmClkFreq)) - (integer << 16);
        ASSERT_COND((fraction & ~FPM_TS_FRACTION_MASK) == 0);
        tmpReg = (integer << FPM_TS_INT_SHIFT) | (uint16_t)fraction;
        WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmtsc2, tmpReg);
    }

#ifndef VERIFICATION_SUPPORT
    /* RAM ECC -  enable and clear events*/
    /* first we need to clear all parser memory, as it is uninitialized and
    may cause ECC errors */
    for(i=0;i<FM_SW_PRS_SIZE;i+=4)
        WRITE_UINT32(*CAST_UINT64_TO_POINTER_TYPE(uint32_t, (p_Fm->baseAddr + FM_MM_PRS + i)), 0);

    tmpReg = 0;
    if(p_Fm->exceptions & FM_EX_IRAM_ECC)
    {
        p_Fm->ramsEccEnable = TRUE;
        tmpReg |= FPM_IRAM_ECC_ERR_EX_EN;
    }
    if(p_Fm->exceptions & FM_EX_NURAM_ECC)
    {
        p_Fm->ramsEccEnable = TRUE;
        tmpReg |= FPM_MURAM_ECC_ERR_EX_EN;
    }
    WRITE_UINT32(p_Fm->p_FmFpmRegs->fmeie, tmpReg);

    /* event bits */
    tmpReg = (FPM_RAM_CTL_MURAM_ECC | FPM_RAM_CTL_IRAM_ECC);
    /* enable ECC */
    if(p_Fm->ramsEccEnable)
        tmpReg |= (FPM_RAM_CTL_RAMS_ECC_EN | FPM_RAM_CTL_IRAM_ECC_EN);
    /* Rams enable is not effected by the RCR bit, but by a COP configuration */
    if(p_Fm->p_FmDriverParam->externalEccRamsEnable)
        tmpReg |= FPM_RAM_CTL_RAMS_ECC_EN_SRC_SEL;

    /* enable test mode */
    if(p_FmDriverParam->enMuramTestMode)
        tmpReg |= FPM_RAM_CTL_MURAM_TEST_ECC;
    if(p_FmDriverParam->enIramTestMode)
        tmpReg |= FPM_RAM_CTL_IRAM_TEST_ECC;
    WRITE_UINT32(p_Fm->p_FmFpmRegs->fmrcr, tmpReg);
#endif  /*  VERIFICATION_SUPPORT */

    /**********************/
    /* Init BMI Registers */
    /**********************/

    /* define common resources */
    /* allocate MURAM for FIFO according to total size */
    p_Fm->fifoBaseAddr = CAST_POINTER_TO_UINT64(FM_MURAM_AllocMem(p_Fm->h_FmMuram,
                                                                  p_Fm->totalFifoSize,
                                                                  BMI_FIFO_ALIGN));
    if (!p_Fm->fifoBaseAddr)
    {
        FmFreeInitResources(p_Fm);
        RETURN_ERROR(MAJOR, E_NO_MEMORY, ("MURAM alloc for FIFO failed"));
    }

    tmpReg = (uint32_t)(CAST_POINTER_TO_UINT64(XX_VirtToPhys(CAST_UINT32_TO_POINTER(p_Fm->fifoBaseAddr))) - p_Fm->fmMuramPhysBaseAddr);
    tmpReg = tmpReg / BMI_FIFO_ALIGN;

    tmpReg |= ((p_Fm->totalFifoSize/BMI_FIFO_UNITS - 1) << BMI_CFG1_FIFO_SIZE_SHIFT);
    WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_cfg1, tmpReg);

    tmpReg =  ((uint32_t)(p_Fm->totalNumOfTasks - 1) << BMI_CFG2_TASKS_SHIFT );
    /* num of DMA's will be dynamically updated when each port is set */
    WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_cfg2, tmpReg);

    /* define unmaskable exceptions, enable and clear events */
    tmpReg = 0;
    WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_ievr, (BMI_ERR_INTR_EN_LIST_RAM_ECC|BMI_ERR_INTR_EN_PIPELINE_ECC|BMI_ERR_INTR_EN_STATISTICS_RAM_ECC));
    if(p_Fm->exceptions & FM_EX_BMI_LIST_RAM_ECC)
    {
        tmpReg |= BMI_ERR_INTR_EN_LIST_RAM_ECC;
    }
    if(p_Fm->exceptions & FM_EX_BMI_PIPELINE_ECC)
    {
        tmpReg |= BMI_ERR_INTR_EN_PIPELINE_ECC;
    }
    if(p_Fm->exceptions & FM_EX_BMI_STATISTICS_RAM_ECC)
    {
        tmpReg |= BMI_ERR_INTR_EN_STATISTICS_RAM_ECC;
    }
    WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_ier, tmpReg);

    /**********************/
    /* Init QMI Registers */
    /**********************/
     /* Clear error interrupt events */
    WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_eie, (QMI_ERR_INTR_EN_DOUBLE_ECC | QMI_ERR_INTR_EN_DEQ_FROM_DEF));
    tmpReg = 0;
    if(p_Fm->exceptions & FM_EX_QMI_DEQ_FROM_DEFQ)
    {
        tmpReg |= QMI_ERR_INTR_EN_DEQ_FROM_DEF;
    }
    if(p_Fm->exceptions & FM_EX_QMI_DOUBLE_ECC)
    {
        tmpReg |= QMI_ERR_INTR_EN_DOUBLE_ECC;
    }
    /* enable events */
    WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_eien, tmpReg);

    tmpReg = 0;
    /* Clear interrupt events */
    WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_ie, QMI_INTR_EN_SINGLE_ECC);
    if(p_Fm->exceptions & FM_EX_QMI_SINGLE_ECC)
    {
        tmpReg |= QMI_INTR_EN_SINGLE_ECC;
    }
    /* enable events */
    WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_ien, tmpReg);

    /* clear & enable global counters  - calculate reg and save for later,
       because it's the same reg for QMI enable */
    if(p_Fm->p_FmDriverParam->enCounters)
        cfgReg = QMI_CFG_EN_COUNTERS;

    cfgReg |= (uint32_t)(((QMI_DEF_TNUMS_THRESH) << 8) |  (uint32_t)QMI_DEF_TNUMS_THRESH);

    if (p_Fm->irq != NO_IRQ)
    {
        XX_SetIntr(p_Fm->irq, FM_Isr, p_Fm);
        XX_EnableIntr(p_Fm->irq);
    }

    if (p_Fm->errIrq != NO_IRQ)
    {
        XX_SetIntr(p_Fm->errIrq, FM_Isr, p_Fm);
        XX_EnableIntr(p_Fm->errIrq);
    }

#ifdef FM_MASTER_PARTITION
    err = XX_RegisterMessageHandler(p_Fm->fmModuleName, FmHandleIpcMsg, p_Fm);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);
#endif /* FM_MASTER_PARTITION */

    /**********************/
    /* Enable all modules */
    /**********************/
    WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_init, BMI_INIT_START);
    WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_gc, cfgReg | QMI_CFG_ENQ_EN | QMI_CFG_DEQ_EN);

    if (p_Fm->p_FmDriverParam->firmware.p_Code)
    {
        XX_Free(p_Fm->p_FmDriverParam->firmware.p_Code);
        p_Fm->p_FmDriverParam->firmware.p_Code = NULL;
    }

    XX_Free(p_Fm->p_FmDriverParam);
    p_Fm->p_FmDriverParam = NULL;

    return E_OK;
}

/**************************************************************************//**
 @Function      FM_Free

 @Description   Frees all resources that were assigned to FM module.

                Calling this routine invalidates the descriptor.

 @Param[in]     h_Fm - FM module descriptor

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
t_Error FM_Free(t_Handle h_Fm)
{
   t_Fm        *p_Fm = (t_Fm*)h_Fm;

    if (!p_Fm)
        return ERROR_CODE(E_INVALID_HANDLE);

#ifdef FM_MASTER_PARTITION
    XX_UnregisterMessageHandler(p_Fm->fmModuleName);
#endif /* FM_MASTER_PARTITION */

    if(p_Fm->p_FmDriverParam)
    {
        XX_Free(p_Fm->p_FmDriverParam);
        p_Fm->p_FmDriverParam = NULL;
    }
    FmFreeInitResources(p_Fm);

    XX_Free(p_Fm);

    return E_OK;
}

/*************************************************/
/*       API Advanced Init unit functions        */
/*************************************************/

t_Error FM_ConfigResetOnInit(t_Handle h_Fm, bool enable)
{

    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->resetOnInit = enable;

    return E_OK;
}

#if 0
t_Error FM_ConfigIndependentMode(t_Handle h_Fm)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    RETURN_ERROR(MINOR, E_NOT_SUPPORTED, NO_MSG);
    return E_OK;
}
#endif /* 0 */

t_Error FM_ConfigTotalNumOfTasks(t_Handle h_Fm, uint8_t totalNumOfTasks)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->totalNumOfTasks = totalNumOfTasks;

    return E_OK;
}

t_Error FM_ConfigTotalFifoSize(t_Handle h_Fm, uint32_t totalFifoSize)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->totalFifoSize = totalFifoSize;

    return E_OK;
}

t_Error FM_ConfigMaxNumOfOpenDmas(t_Handle h_Fm, uint8_t maxNumOfOpenDmas)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->maxNumOfOpenDmas = maxNumOfOpenDmas;

    return E_OK;
}

t_Error FM_ConfigThresholds(t_Handle h_Fm, t_FmThresholds *p_FmThresholds)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    memcpy(&p_Fm->p_FmDriverParam->thresholds, p_FmThresholds, sizeof(t_FmThresholds));

    return E_OK;
}

t_Error FM_ConfigTimeStamp(t_Handle h_Fm, uint32_t timeStampPeriod)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->enTimeStamp = TRUE;
    p_Fm->timeStampPeriod = timeStampPeriod;

    return E_OK;
}

t_Error FM_ConfigDmaBusProtect(t_Handle h_Fm, t_FmDmaBusProtect *p_FmDmaBusProtect)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    memcpy(&p_Fm->p_FmDriverParam->dmaBusProtect, p_FmDmaBusProtect, sizeof(t_FmDmaBusProtect));

    return E_OK;
}

t_Error FM_ConfigDmaCacheOverride(t_Handle h_Fm, e_FmDmaCacheOverride cacheOverride)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->dmaCacheOverride = cacheOverride;

    return E_OK;
}

t_Error FM_ConfigDmaAidOverride(t_Handle h_Fm, bool aidOverride)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->dmaAidOverride = aidOverride;

    return E_OK;
}

t_Error FM_ConfigDmaAidMode(t_Handle h_Fm, e_FmDmaAidMode aidMode)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->dmaAidMode = aidMode;

    return E_OK;
}

t_Error FM_ConfigDmaAxiDbgNumOfBeats(t_Handle h_Fm, uint8_t axiDbgNumOfBeats)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->dmaAxiDbgNumOfBeats = axiDbgNumOfBeats;

    return E_OK;
}

t_Error FM_ConfigDmaCamNumOfEntries(t_Handle h_Fm, uint8_t numOfEntries)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->dmaCamNumOfEntries = numOfEntries;

    return E_OK;
}

t_Error FM_ConfigDmaWatchdog(t_Handle h_Fm, uint32_t watchdogValue)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->dmaWatchdog = watchdogValue;

    return E_OK;
}

t_Error FM_ConfigDmaWriteBufThresholds(t_Handle h_Fm, t_FmDmaThresholds *p_FmDmaThresholds)

{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    memcpy(&p_Fm->p_FmDriverParam->dmaWriteBufThresholds, p_FmDmaThresholds, sizeof(t_FmDmaThresholds));

    return E_OK;
}

t_Error FM_ConfigDmaCommQThresholds(t_Handle h_Fm, t_FmDmaThresholds *p_FmDmaThresholds)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    memcpy(&p_Fm->p_FmDriverParam->dmaCommQThresholds, p_FmDmaThresholds, sizeof(t_FmDmaThresholds));

    return E_OK;
}

t_Error FM_ConfigDmaReadBufThresholds(t_Handle h_Fm, t_FmDmaThresholds *p_FmDmaThresholds)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    memcpy(&p_Fm->p_FmDriverParam->dmaReadBufThresholds, p_FmDmaThresholds, sizeof(t_FmDmaThresholds));

    return E_OK;
}

t_Error FM_ConfigDmaEmergency(t_Handle h_Fm, t_FmDmaEmergency *p_Emergency)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->dmaEnEmergency = TRUE;
    memcpy(&p_Fm->p_FmDriverParam->dmaEmergency, p_Emergency, sizeof(t_FmDmaEmergency));

    return E_OK;
}

t_Error FM_ConfigDmaEmergencySmoother(t_Handle h_Fm, uint32_t emergencyCnt)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    if(!p_Fm->p_FmDriverParam->dmaEnEmergency)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("FM_ConfigEnDmaEmergencySmoother may be called only after FM_ConfigEnDmaEmergency"));

    p_Fm->p_FmDriverParam->dmaEnEmergencySmoother = TRUE;
    p_Fm->p_FmDriverParam->dmaEmergencySwitchCounter = emergencyCnt;

    return E_OK;
}

t_Error FM_ConfigDmaDbgCounter(t_Handle h_Fm, e_FmDmaDbgCntMode fmDmaDbgCntMode)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->dmaDbgCntMode = fmDmaDbgCntMode;

    return E_OK;
}

t_Error FM_ConfigDmaStopOnBusErr(t_Handle h_Fm, bool stop)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->dmaStopOnBusError = stop;

    return E_OK;
}

t_Error FM_ConfigDmaSosEmergencyThreshold(t_Handle h_Fm, uint32_t dmaSosEmergency)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->dmaSosEmergency = dmaSosEmergency;

    return E_OK;
}

t_Error FM_ConfigEnableCounters(t_Handle h_Fm)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->enCounters = TRUE;

    return E_OK;
}

t_Error FM_ConfigDmaErr(t_Handle h_Fm, e_FmDmaErr dmaErr)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->dmaErr = dmaErr;

    return E_OK;
}

t_Error FM_ConfigCatastrophicErr(t_Handle h_Fm, e_FmCatastrophicErr catastrophicErr)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->catastrophicErr = catastrophicErr;

    return E_OK;
}

t_Error FM_ConfigEnableMuramTestMode(t_Handle h_Fm)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->enMuramTestMode = TRUE;

    return E_OK;
}

t_Error FM_ConfigEnableIramTestMode(t_Handle h_Fm)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->enIramTestMode = TRUE;

    return E_OK;
}

t_Error FM_ConfigHaltOnExternalActivation(t_Handle h_Fm, bool enable)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->haltOnExternalActivation = enable;

    return E_OK;
}

t_Error FM_ConfigHaltOnUnrecoverableEccError(t_Handle h_Fm, bool enable)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->haltOnUnrecoverableEccError = enable;

    return E_OK;
}

t_Error FM_ConfigException(t_Handle h_Fm, e_FmExceptions exception, bool enable)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;
    uint32_t    bitMask = 0;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);

    GET_EXCEPTION_FLAG(bitMask, exception);
    if(bitMask)
    {
        if (enable)
            p_Fm->exceptions |= bitMask;
        else
            p_Fm->exceptions &= ~bitMask;
   }
    else
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Undefined exception"));

    return E_OK;
}

t_Error FM_ConfigExternalEccRamsEnable(t_Handle h_Fm, bool enable)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);

    p_Fm->p_FmDriverParam->externalEccRamsEnable = enable;

    return E_OK;
}

/****************************************************/
/*       API Run-time Control uint functions        */
/****************************************************/
t_Handle FM_GetPcdHandle(t_Handle h_Fm)
{
    SANITY_CHECK_RETURN_VALUE(h_Fm, E_INVALID_HANDLE, NULL);
    SANITY_CHECK_RETURN_VALUE(!((t_Fm*)h_Fm)->p_FmDriverParam, E_INVALID_STATE, NULL);

    return ((t_Fm*)h_Fm)->h_Pcd;
}

void FM_Isr(t_Handle h_Fm)
{
    t_Fm                    *p_Fm = (t_Fm*)h_Fm;
    uint32_t                pending;

    SANITY_CHECK_RETURN(h_Fm, E_INVALID_HANDLE);

    /* error interrupts */
    pending = GET_UINT32(p_Fm->p_FmFpmRegs->fmepi);
    if(pending) /* remove if separate sources */
    {
        if(pending & ERR_INTR_EN_BMI)
            BmiErrEvent(p_Fm);
        if(pending & ERR_INTR_EN_QMI)
            QmiErrEvent(p_Fm);
        if(pending & ERR_INTR_EN_FPM)
            FpmErrEvent(p_Fm);
        if(pending & ERR_INTR_EN_DMA)
            DmaErrEvent(p_Fm);
        if(pending & ERR_INTR_EN_IRAM)
            IramErrIntr(p_Fm);
        if(pending & ERR_INTR_EN_MURAM)
            MuramErrIntr(p_Fm);
        if(pending & ERR_INTR_EN_PRS)
            p_Fm->intrMng[e_FM_EV_ERR_PRS].f_Isr(p_Fm->intrMng[e_FM_EV_ERR_PRS].h_SrcHandle);
        if(pending & ERR_INTR_EN_PLCR)
            p_Fm->intrMng[e_FM_EV_ERR_PLCR].f_Isr(p_Fm->intrMng[e_FM_EV_ERR_PLCR].h_SrcHandle);
        if(pending & ERR_INTR_EN_KG)
            p_Fm->intrMng[e_FM_EV_ERR_KG].f_Isr(p_Fm->intrMng[e_FM_EV_ERR_KG].h_SrcHandle);
        if(pending & ERR_INTR_EN_1G_MAC0)
            p_Fm->intrMng[e_FM_EV_ERR_1G_MAC0].f_Isr(p_Fm->intrMng[e_FM_EV_ERR_1G_MAC0].h_SrcHandle);
        if(pending & ERR_INTR_EN_1G_MAC1)
            p_Fm->intrMng[e_FM_EV_ERR_1G_MAC1].f_Isr(p_Fm->intrMng[e_FM_EV_ERR_1G_MAC1].h_SrcHandle);
        if(pending & ERR_INTR_EN_1G_MAC2)
            p_Fm->intrMng[e_FM_EV_ERR_1G_MAC2].f_Isr(p_Fm->intrMng[e_FM_EV_ERR_1G_MAC2].h_SrcHandle);
        if(pending & ERR_INTR_EN_1G_MAC3)
            p_Fm->intrMng[e_FM_EV_ERR_1G_MAC3].f_Isr(p_Fm->intrMng[e_FM_EV_ERR_1G_MAC3].h_SrcHandle);
        if(pending & ERR_INTR_EN_10G_MAC0)
            p_Fm->intrMng[e_FM_EV_ERR_10G_MAC0].f_Isr(p_Fm->intrMng[e_FM_EV_ERR_10G_MAC0].h_SrcHandle);
    }

    /* normal interrupts */
    pending = GET_UINT32(p_Fm->p_FmFpmRegs->fmnpi);
    if(pending) /* remove if separate sources */
    {
        if(pending & INTR_EN_BMI)
            REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("BMI Event - undefined!"));
        if(pending & INTR_EN_QMI)
            QmiEvent(p_Fm);
        if(pending & INTR_EN_PRS)
            p_Fm->intrMng[e_FM_EV_PRS].f_Isr(p_Fm->intrMng[e_FM_EV_PRS].h_SrcHandle);
        if(pending & INTR_EN_PLCR)
            p_Fm->intrMng[e_FM_EV_PLCR].f_Isr(p_Fm->intrMng[e_FM_EV_PLCR].h_SrcHandle);
        if(pending & INTR_EN_KG)
            p_Fm->intrMng[e_FM_EV_KG].f_Isr(p_Fm->intrMng[e_FM_EV_KG].h_SrcHandle);
        if(pending & FPM_EVENT_FM_CTL)
            FmCtlEvent(p_Fm, pending  & FPM_EVENT_FM_CTL);
        if(pending & INTR_EN_1G_MAC1)
            p_Fm->intrMng[e_FM_EV_1G_MAC1].f_Isr(p_Fm->intrMng[e_FM_EV_1G_MAC1].h_SrcHandle);
        if(pending & INTR_EN_1G_MAC2)
            p_Fm->intrMng[e_FM_EV_1G_MAC2].f_Isr(p_Fm->intrMng[e_FM_EV_1G_MAC2].h_SrcHandle);
        if(pending & INTR_EN_1G_MAC3)
            p_Fm->intrMng[e_FM_EV_1G_MAC3].f_Isr(p_Fm->intrMng[e_FM_EV_1G_MAC3].h_SrcHandle);
        if(pending & INTR_EN_1G_MAC0_TMR)
            p_Fm->intrMng[e_FM_EV_1G_MAC0_TMR].f_Isr(p_Fm->intrMng[e_FM_EV_1G_MAC0_TMR].h_SrcHandle);
        if(pending & INTR_EN_1G_MAC1_TMR)
            p_Fm->intrMng[e_FM_EV_1G_MAC1_TMR].f_Isr(p_Fm->intrMng[e_FM_EV_1G_MAC1_TMR].h_SrcHandle);
        if(pending & INTR_EN_1G_MAC2_TMR)
            p_Fm->intrMng[e_FM_EV_1G_MAC2_TMR].f_Isr(p_Fm->intrMng[e_FM_EV_1G_MAC2_TMR].h_SrcHandle);
        if(pending & INTR_EN_1G_MAC3_TMR)
            p_Fm->intrMng[e_FM_EV_1G_MAC3_TMR].f_Isr(p_Fm->intrMng[e_FM_EV_1G_MAC3_TMR].h_SrcHandle);
        if(pending & INTR_EN_TMR)
            p_Fm->intrMng[e_FM_EV_TMR].f_Isr(p_Fm->intrMng[e_FM_EV_TMR].h_SrcHandle);
    }
}

t_Error FM_SetPortsBandwidth(t_Handle h_Fm, t_PortsParam *p_PortsBandwidth)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;
    int         j, i;
    uint8_t     sum;
    uint8_t     hardwarePortId=0;
    uint8_t     portPrecent[FM_MAX_NUM_OF_PORTS];
    uint32_t    tmpReg;
    uint8_t     relativePortId, remain, shift, weight, maxPercent = 0;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_Fm->p_FmDriverParam, E_INVALID_STATE);

    memset(portPrecent, 0, FM_MAX_NUM_OF_PORTS);
    for(i=0;i<NUM_OF_PORT_TYPES;i++)
        for(j=0;j<MAX_NUM_OF_PORTS_PER_TYPE;j++)
        {
            if((*p_PortsBandwidth)[i][j])
            {
                GET_GLOBAL_PORTID(hardwarePortId, i,j);
                portPrecent[hardwarePortId] = (*p_PortsBandwidth)[i][j];
            }
        }

    /* check that all ports add up to 100% */
    sum = 0;
    for (i=0;i<FM_MAX_NUM_OF_PORTS;i++)
        sum +=portPrecent[i];
    if (sum != 100)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Sum of ports bandwidth differ from 100%"));

    tmpReg = 0;
    /* find highest precent */
    for (i=0;i<FM_MAX_NUM_OF_PORTS;i++)
    {
        if (portPrecent[i] > maxPercent)
            maxPercent = portPrecent[i];
    }

    /* calculate weight for each port */
    for (i=0;i<FM_MAX_NUM_OF_PORTS;i++)
    {
        weight = (uint8_t)((portPrecent[i] * PORT_MAX_WEIGHT )/maxPercent);
        remain = (uint8_t)((portPrecent[i] * PORT_MAX_WEIGHT ) - maxPercent*weight);

        /* round the remain to add 1 if it is bigger than 0.5 */
        if (remain*2 > maxPercent)
            weight++;

        /* find the location of this port within the register */
        relativePortId = (uint8_t)(i % 8);
        shift = (uint8_t)(32-4*(relativePortId+1));


        if(weight)
            /* Add this port to tmpReg */
            tmpReg |= ((weight-1) << shift);

        /* each 8 ports result in one register, write this register */
        if (relativePortId == 7 && tmpReg)
        {
            WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_arb[i/8], tmpReg);
            tmpReg = 0;
        }
    }


    return E_OK;
}

t_Error FM_EnableRamsEcc(t_Handle h_Fm)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;
    uint32_t    tmpReg;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    tmpReg = GET_UINT32(p_Fm->p_FmFpmRegs->fmrcr);
    if(tmpReg & FPM_RAM_CTL_RAMS_ECC_EN_SRC_SEL)
        RETURN_ERROR(MINOR, E_INVALID_STATE,("Rams ECC is configured to be controlled through JTAG"));

    if(p_Fm->ramsEccEnable)
        return E_OK;
    else
    {
        WRITE_UINT32(p_Fm->p_FmFpmRegs->fmrcr, tmpReg | (FPM_RAM_CTL_RAMS_ECC_EN | FPM_RAM_CTL_IRAM_ECC_EN));
        p_Fm->ramsEccEnable = TRUE;
    }

    return E_OK;
}

t_Error FM_DisableRamsEcc(t_Handle h_Fm)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;
    uint32_t    tmpReg;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_Fm->p_FmDriverParam, E_INVALID_HANDLE);

    tmpReg = GET_UINT32(p_Fm->p_FmFpmRegs->fmrcr);
    if(tmpReg & FPM_RAM_CTL_RAMS_ECC_EN_SRC_SEL)
        RETURN_ERROR(MINOR, E_INVALID_STATE,("Rams ECC is configured to be controlled through JTAG"));

    if(!p_Fm->ramsEccEnable)
        return E_OK;
    else
    {
        WRITE_UINT32(p_Fm->p_FmFpmRegs->fmrcr, tmpReg & ~(FPM_RAM_CTL_RAMS_ECC_EN | FPM_RAM_CTL_IRAM_ECC_EN));
        p_Fm->ramsEccEnable = FALSE;
    }

    return E_OK;
}

t_Error FM_SetException(t_Handle h_Fm, e_FmExceptions exception, bool enable)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;
    uint32_t    bitMask = 0;
    uint32_t    tmpReg;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_Fm->p_FmDriverParam, E_INVALID_STATE);

    GET_EXCEPTION_FLAG(bitMask, exception);
    if(bitMask)
    {
        if (enable)
            p_Fm->exceptions |= bitMask;
        else
            p_Fm->exceptions &= ~bitMask;

        switch(exception)
        {
             case(e_FM_EX_DMA_BUS_ERROR):
                tmpReg = GET_UINT32(p_Fm->p_FmDmaRegs->fmdmmr);
                if(enable)
                    tmpReg |= DMA_MODE_BER;
                else
                    tmpReg &= ~DMA_MODE_BER;
                /* disable bus error */
                WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmmr, tmpReg);
                break;
             case(e_FM_EX_DMA_READ_ECC):
             case(e_FM_EX_DMA_SYSTEM_WRITE_ECC):
             case(e_FM_EX_DMA_FM_WRITE_ECC):
                tmpReg = GET_UINT32(p_Fm->p_FmDmaRegs->fmdmmr);
                if(enable)
                    tmpReg |= DMA_MODE_ECC;
                else
                    tmpReg &= ~DMA_MODE_ECC;
                WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmmr, tmpReg);
                break;
             case(e_FM_EX_FPM_STALL_ON_TASKS):
                tmpReg = GET_UINT32(p_Fm->p_FmFpmRegs->fpmem);
                if(enable)
                    tmpReg |= FPM_EV_MASK_STALL_EN;
                else
                    tmpReg &= ~FPM_EV_MASK_STALL_EN;
                WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmem, tmpReg);
                break;
             case(e_FM_EX_FPM_SINGLE_ECC):
                tmpReg = GET_UINT32(p_Fm->p_FmFpmRegs->fpmem);
                if(enable)
                    tmpReg |= FPM_EV_MASK_SINGLE_ECC_EN;
                else
                    tmpReg &= ~FPM_EV_MASK_SINGLE_ECC_EN;
                WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmem, tmpReg);
                break;
            case( e_FM_EX_FPM_DOUBLE_ECC):
                tmpReg = GET_UINT32(p_Fm->p_FmFpmRegs->fpmem);
                if(enable)
                    tmpReg |= FPM_EV_MASK_DOUBLE_ECC_EN;
                else
                    tmpReg &= ~FPM_EV_MASK_DOUBLE_ECC_EN;
                WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmem, tmpReg);
                break;
            case( e_FM_EX_QMI_SINGLE_ECC):
                tmpReg = GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_ien);
                if(enable)
                    tmpReg |= QMI_INTR_EN_SINGLE_ECC;
                else
                    tmpReg &= ~QMI_INTR_EN_SINGLE_ECC;
                WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_ien, tmpReg);
                break;
             case(e_FM_EX_QMI_DOUBLE_ECC):
                tmpReg = GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_eien);
                if(enable)
                    tmpReg |= QMI_ERR_INTR_EN_DOUBLE_ECC;
                else
                    tmpReg &= ~QMI_ERR_INTR_EN_DOUBLE_ECC;
                WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_eien, tmpReg);
                break;
             case(e_FM_EX_QMI_DEQ_FROM_DEFQ):
                tmpReg = GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_eien);
                if(enable)
                    tmpReg |= QMI_ERR_INTR_EN_DEQ_FROM_DEF;
                else
                    tmpReg &= ~QMI_ERR_INTR_EN_DEQ_FROM_DEF;
                WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_eien, tmpReg);
                break;
             case(e_FM_EX_BMI_LIST_RAM_ECC):
                tmpReg = GET_UINT32(p_Fm->p_FmBmiRegs->fmbm_ier);
                if(enable)
                    tmpReg |= BMI_ERR_INTR_EN_LIST_RAM_ECC;
                else
                    tmpReg &= ~BMI_ERR_INTR_EN_LIST_RAM_ECC;
                WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_ier, tmpReg);
                break;
             case(e_FM_EX_BMI_PIPELINE_ECC):
                tmpReg = GET_UINT32(p_Fm->p_FmBmiRegs->fmbm_ier);
                if(enable)
                    tmpReg |= BMI_ERR_INTR_EN_PIPELINE_ECC;
                else
                    tmpReg &= ~BMI_ERR_INTR_EN_PIPELINE_ECC;
                WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_ier, tmpReg);
                break;
              case(e_FM_EX_BMI_STATISTICS_RAM_ECC):
                tmpReg = GET_UINT32(p_Fm->p_FmBmiRegs->fmbm_ier);
                if(enable)
                    tmpReg |= BMI_ERR_INTR_EN_STATISTICS_RAM_ECC;
                else
                    tmpReg &= ~BMI_ERR_INTR_EN_STATISTICS_RAM_ECC;
                WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_ier, tmpReg);
                break;
            case(e_FM_EX_IRAM_ECC):
                tmpReg = GET_UINT32(p_Fm->p_FmFpmRegs->fmeie);
                if(enable)
                {
                    /* enable ECC if not enabled */
                    if(!p_Fm->ramsEccEnable)
                    {

                        WRITE_UINT32(p_Fm->p_FmFpmRegs->fmrcr, GET_UINT32(p_Fm->p_FmFpmRegs->fmrcr) |
                                                                (FPM_RAM_CTL_IRAM_ECC_EN | FPM_RAM_CTL_RAMS_ECC_EN));
                        p_Fm->ramsEccEnable = TRUE;
                    }
                    /* enable ECC interrupts */
                    tmpReg |= FPM_IRAM_ECC_ERR_EX_EN;
                }
                else
                    tmpReg &= FPM_IRAM_ECC_ERR_EX_EN;
                WRITE_UINT32(p_Fm->p_FmFpmRegs->fmeie, tmpReg);
                break;

             case(e_FM_EX_MURAM_ECC):
                tmpReg = GET_UINT32(p_Fm->p_FmFpmRegs->fmeie);
                if(enable)
                {
                    /* enable ECC if not enabled */
                    if(!FmRamsEccIsExternalCtl(p_Fm))
                        FM_EnableRamsEcc(p_Fm);
                    /* enable ECC interrupts */
                    tmpReg |= FPM_MURAM_ECC_ERR_EX_EN;
                }
                else
                    tmpReg &= FPM_MURAM_ECC_ERR_EX_EN;
                WRITE_UINT32(p_Fm->p_FmFpmRegs->fmeie, tmpReg);

                break;
            default:
                RETURN_ERROR(MINOR, E_INVALID_SELECTION, NO_MSG);

        }
    }
    else
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Undefined exception"));

    return E_OK;
}

void FM_GetRevision(t_Handle h_Fm, t_FmRevisionInfo *p_FmRevisionInfo)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;
    uint32_t    tmpReg;

    SANITY_CHECK_RETURN(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN(!p_Fm->p_FmDriverParam, E_INVALID_STATE);

    /* read revision register 1 */
    tmpReg = GET_UINT32(p_Fm->p_FmFpmRegs->fm_ip_rev_1);
    p_FmRevisionInfo->majorRev = (uint8_t)((tmpReg & FPM_REV1_MAJOR_MASK) >> FPM_REV1_MAJOR_SHIFT);
    p_FmRevisionInfo->minorRev = (uint8_t)((tmpReg & FPM_REV1_MINOR_MASK) >> FPM_REV1_MINOR_SHIFT);
}

uint32_t FM_GetTimeStamp(t_Handle h_Fm)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_VALUE(p_Fm, E_INVALID_HANDLE, 0);
    SANITY_CHECK_RETURN_VALUE(!p_Fm->p_FmDriverParam, E_INVALID_STATE, 0);

    /* check that timeStamp is enabled */
    if (!(GET_UINT32(p_Fm->p_FmFpmRegs->fpmtsc1) & FPM_TS_CTL_EN))
    {
        REPORT_ERROR(MINOR, E_INVALID_STATE, ("Time Stamp was not enabled"));
        return 0;
    }

    return GET_UINT32(p_Fm->p_FmFpmRegs->fpmtsp);;
}

uint32_t  FM_GetCounter(t_Handle h_Fm, e_FmCounters counter)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_VALUE(p_Fm, E_INVALID_HANDLE, 0);
    SANITY_CHECK_RETURN_VALUE(!p_Fm->p_FmDriverParam, E_INVALID_STATE, 0);

        switch(counter)
    {
        case(e_FM_COUNTERS_ENQ_TOTAL_FRAME):
            return GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_etfc);
        break;
        case(e_FM_COUNTERS_DEQ_TOTAL_FRAME):
            return GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_dtfc);
        case(e_FM_COUNTERS_DEQ_0):
            return GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_dc0);
        case(e_FM_COUNTERS_DEQ_1):
            return GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_dc1);
        case(e_FM_COUNTERS_DEQ_2):
            return GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_dc2);
        case(e_FM_COUNTERS_DEQ_3):
            return GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_dc3);
        case(e_FM_COUNTERS_DEQ_FROM_DEFAULT):
            return GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_dfdc);
        case(e_FM_COUNTERS_DEQ_FROM_CONTEXT):
            return GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_dfcc);
        case(e_FM_COUNTERS_DEQ_FROM_FD):
            return GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_dffc);
        case(e_FM_COUNTERS_DEQ_CONFIRM):
            return GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_dcc);
        case(e_FM_COUNTERS_SEMAPHOR_ENTRY_FULL_REJECT):
            return GET_UINT32(p_Fm->p_FmDmaRegs->fmdmsefrc);
        case(e_FM_COUNTERS_SEMAPHOR_QUEUE_FULL_REJECT):
            return GET_UINT32(p_Fm->p_FmDmaRegs->fmdmsqfrc);
        case(e_FM_COUNTERS_SEMAPHOR_SYNC_REJECT):
            return GET_UINT32(p_Fm->p_FmDmaRegs->fmdmssrc);
        default:
            break;
    }
    /* should never get here */
    ASSERT_COND(FALSE);

    return 0;
}

t_Error  FM_SetCounter(t_Handle h_Fm, e_FmCounters counter, uint32_t val)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

   SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
   SANITY_CHECK_RETURN_ERROR(!p_Fm->p_FmDriverParam, E_INVALID_STATE);

    /* When applicable (when there is an 'enable counters' bit,
    check that counters are enabled */
    switch(counter)
    {
        case(e_FM_COUNTERS_ENQ_TOTAL_FRAME):
        case(e_FM_COUNTERS_DEQ_TOTAL_FRAME):
        case(e_FM_COUNTERS_DEQ_0):
        case(e_FM_COUNTERS_DEQ_1):
        case(e_FM_COUNTERS_DEQ_2):
        case(e_FM_COUNTERS_DEQ_3):
        case(e_FM_COUNTERS_DEQ_FROM_DEFAULT):
        case(e_FM_COUNTERS_DEQ_FROM_CONTEXT):
        case(e_FM_COUNTERS_DEQ_FROM_FD):
        case(e_FM_COUNTERS_DEQ_CONFIRM):
            if(!(GET_UINT32(p_Fm->p_FmQmiRegs->fmqm_gc) & QMI_CFG_EN_COUNTERS))
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("Requested counter was not enabled"));
            break;
        default:
            break;
    }

    /* Set counter */
    switch(counter)
    {
        case(e_FM_COUNTERS_ENQ_TOTAL_FRAME):
            WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_etfc, val);
            break;
        case(e_FM_COUNTERS_DEQ_TOTAL_FRAME):
            WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_dtfc, val);
            break;
        case(e_FM_COUNTERS_DEQ_0):
            WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_dc0, val);
            break;
        case(e_FM_COUNTERS_DEQ_1):
            WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_dc1, val);
            break;
        case(e_FM_COUNTERS_DEQ_2):
            WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_dc2, val);
            break;
        case(e_FM_COUNTERS_DEQ_3):
            WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_dc3, val);
            break;
        case(e_FM_COUNTERS_DEQ_FROM_DEFAULT):
            WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_dfdc, val);
            break;
        case(e_FM_COUNTERS_DEQ_FROM_CONTEXT):
            WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_dfcc, val);
            break;
        case(e_FM_COUNTERS_DEQ_FROM_FD):
            WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_dffc, val);
            break;
        case(e_FM_COUNTERS_DEQ_CONFIRM):
            WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_dcc, val);
            break;
        case(e_FM_COUNTERS_SEMAPHOR_ENTRY_FULL_REJECT):
            WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmsefrc, val);
            break;
        case(e_FM_COUNTERS_SEMAPHOR_QUEUE_FULL_REJECT):
            WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmsqfrc, val);
            break;
        case(e_FM_COUNTERS_SEMAPHOR_SYNC_REJECT):
            WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmssrc, val);
            break;
        default:
            break;
    }

    return E_OK;
}

void FM_DmaEmergencyCtrl(t_Handle h_Fm, e_FmDmaMuramPort muramPort, bool enable)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;
    uint32_t    bitMask;

    SANITY_CHECK_RETURN(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN(!p_Fm->p_FmDriverParam, E_INVALID_STATE);

    bitMask = (uint32_t)((muramPort==e_FM_DMA_MURAM_PORT_WRITE) ? DMA_MODE_EMERGENCY_WRITE : DMA_MODE_EMERGENCY_READ);

    if(enable)
        WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmmr, GET_UINT32(p_Fm->p_FmDmaRegs->fmdmmr) | bitMask);
    else /* disable */
        WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmmr, GET_UINT32(p_Fm->p_FmDmaRegs->fmdmmr) & ~bitMask);

    return;
}

void FM_SetDmaExtBusPri(t_Handle h_Fm, e_FmDmaExtBusPri pri)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN(!p_Fm->p_FmDriverParam, E_INVALID_STATE);

    WRITE_UINT32(p_Fm->p_FmDmaRegs->fmdmmr, GET_UINT32(p_Fm->p_FmDmaRegs->fmdmmr) | ((uint32_t)pri << DMA_MODE_BUS_PRI_SHIFT) );

    return;
}

void FM_GetDmaStatus(t_Handle h_Fm, t_FmDmaStatus *p_FmDmaStatus)
{
    t_Fm        *p_Fm = (t_Fm*)h_Fm;
    uint32_t    tmpReg;

    SANITY_CHECK_RETURN(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN(!p_Fm->p_FmDriverParam, E_INVALID_STATE);

    tmpReg = GET_UINT32(p_Fm->p_FmDmaRegs->fmdmsr);

    p_FmDmaStatus->cmqNotEmpty = (bool)(tmpReg & DMA_STATUS_CMD_QUEUE_NOT_EMPTY);
    p_FmDmaStatus->busError = (bool)(tmpReg & DMA_STATUS_BUS_ERR);
    p_FmDmaStatus->readBufEccError = (bool)(tmpReg & DMA_STATUS_READ_ECC);
    p_FmDmaStatus->writeBufEccSysError = (bool)(tmpReg & DMA_STATUS_SYSTEM_WRITE_ECC);
    p_FmDmaStatus->writeBufEccFmError = (bool)(tmpReg & DMA_STATUS_FM_WRITE_ECC);
    return;
}

t_Error FM_ForceIntr (t_Handle h_Fm, e_FmExceptions exception)
{
    t_Fm *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_Fm->p_FmDriverParam, E_INVALID_STATE);

    switch(exception)
    {
        case e_FM_EX_QMI_DEQ_FROM_DEFQ:
            if (!(p_Fm->exceptions & FM_EX_QMI_DEQ_FROM_DEFQ))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_eif, QMI_ERR_INTR_EN_DEQ_FROM_DEF);
            break;
        case e_FM_EX_QMI_SINGLE_ECC:
            if (!(p_Fm->exceptions & FM_EX_QMI_SINGLE_ECC))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_if, QMI_INTR_EN_SINGLE_ECC);
            break;
        case e_FM_EX_QMI_DOUBLE_ECC:
            if (!(p_Fm->exceptions & FM_EX_QMI_DOUBLE_ECC))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_Fm->p_FmQmiRegs->fmqm_eif, QMI_ERR_INTR_EN_DOUBLE_ECC);
            break;
        case e_FM_EX_BMI_LIST_RAM_ECC:
            if (!(p_Fm->exceptions & FM_EX_BMI_LIST_RAM_ECC))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_ifr, BMI_ERR_INTR_EN_LIST_RAM_ECC);
            break;
        case e_FM_EX_BMI_PIPELINE_ECC:
            if (!(p_Fm->exceptions & FM_EX_BMI_PIPELINE_ECC))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_ifr, BMI_ERR_INTR_EN_PIPELINE_ECC);
            break;
        case e_FM_EX_BMI_STATISTICS_RAM_ECC:
            if (!(p_Fm->exceptions & FM_EX_BMI_STATISTICS_RAM_ECC))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_Fm->p_FmBmiRegs->fmbm_ifr, BMI_ERR_INTR_EN_STATISTICS_RAM_ECC);
            break;
        default:
            RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception may not be forced"));
    }

    return E_OK;
}

void FM_Resume(t_Handle h_Fm)
{
    t_Fm            *p_Fm = (t_Fm*)h_Fm;
    uint32_t        tmpReg;

    SANITY_CHECK_RETURN(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN(!p_Fm->p_FmDriverParam, E_INVALID_STATE);

    tmpReg  = GET_UINT32(p_Fm->p_FmFpmRegs->fpmem);
    /* clear tmpReg event bits in order not to clear standing events */
    tmpReg &= ~(FPM_EV_MASK_DOUBLE_ECC | FPM_EV_MASK_STALL | FPM_EV_MASK_SINGLE_ECC);
    WRITE_UINT32(p_Fm->p_FmFpmRegs->fpmem, tmpReg | FPM_EV_MASK_RELEASE_FM);
}

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
t_Error FM_DumpRegs(t_Handle h_Fm)
{
    t_Fm    *p_Fm = (t_Fm *)h_Fm;
    uint8_t i = 0;

    DECLARE_DUMP;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_Fm->p_FmDriverParam, E_INVALID_STATE);

    DUMP_SUBTITLE(("\n"));
    DUMP_TITLE(p_Fm->p_FmFpmRegs, ("FmFpmRegs Regs"));

    DUMP_VAR(p_Fm->p_FmFpmRegs,fpmtnc);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fpmpr);
    DUMP_VAR(p_Fm->p_FmFpmRegs,brkc);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fpmflc);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fpmdis1);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fpmdis2);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fmepi);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fmeie);

    DUMP_TITLE(&p_Fm->p_FmFpmRegs->fpmrev, ("fpmrev"));
    DUMP_SUBSTRUCT_ARRAY(i, 8)
    {
        DUMP_MEMORY(&p_Fm->p_FmFpmRegs->fpmrev[i], sizeof(uint32_t));
    }

    DUMP_TITLE(&p_Fm->p_FmFpmRegs->fpmmsk, ("fpmmsk"));
    DUMP_SUBSTRUCT_ARRAY(i, 8)
    {
        DUMP_MEMORY(&p_Fm->p_FmFpmRegs->fpmmsk[i], sizeof(uint32_t));
    }

    DUMP_SUBTITLE(("\n"));
    DUMP_VAR(p_Fm->p_FmFpmRegs,fpmtsc1);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fpmtsc2);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fpmtsp);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fpmtsf);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fmrcr);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fpmextc);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fpmext1);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fpmext2);

    DUMP_TITLE(&p_Fm->p_FmFpmRegs->fpmdrd, ("fpmdrd"));
    DUMP_SUBSTRUCT_ARRAY(i, 16)
    {
        DUMP_MEMORY(&p_Fm->p_FmFpmRegs->fpmdrd[i], sizeof(uint32_t));
    }

    DUMP_SUBTITLE(("\n"));
    DUMP_VAR(p_Fm->p_FmFpmRegs,fpmdra);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fm_ip_rev_1);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fm_ip_rev_2);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fmrstc);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fmcld);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fmnpi);
    DUMP_VAR(p_Fm->p_FmFpmRegs,fpmem);

    DUMP_TITLE(&p_Fm->p_FmFpmRegs->fpmcev, ("fpmcev"));
    DUMP_SUBSTRUCT_ARRAY(i, 8)
    {
        DUMP_MEMORY(&p_Fm->p_FmFpmRegs->fpmcev[i], sizeof(uint32_t));
    }

    DUMP_TITLE(&p_Fm->p_FmFpmRegs->fmfp_ps, ("fmfp_ps"));
    DUMP_SUBSTRUCT_ARRAY(i, 64)
    {
        DUMP_MEMORY(&p_Fm->p_FmFpmRegs->fmfp_ps[i], sizeof(uint32_t));
    }


    DUMP_TITLE(p_Fm->p_FmDmaRegs, ("p_FmDmaRegs Regs"));
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmsr);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmmr);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmtr);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmhy);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmsetr);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmtah);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmtal);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmtcid);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmra);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmrd);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmwcr);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmebcr);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmccqdr);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmccqvr1);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmccqvr2);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmcqvr3);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmcqvr4);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmcqvr5);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmsefrc);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmsqfrc);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmssrc);
    DUMP_VAR(p_Fm->p_FmDmaRegs,fmdmdcr);

    DUMP_TITLE(&p_Fm->p_FmDmaRegs->fmdmplr, ("fmdmplr"));

    DUMP_SUBSTRUCT_ARRAY(i, FM_MAX_NUM_OF_PARTITIONS/2)
    {
        DUMP_MEMORY(&p_Fm->p_FmDmaRegs->fmdmplr[i], sizeof(uint32_t));
    }

    DUMP_TITLE(p_Fm->p_FmBmiRegs, ("p_FmBmiRegs COMMON Regs"));
    DUMP_VAR(p_Fm->p_FmBmiRegs,fmbm_init);
    DUMP_VAR(p_Fm->p_FmBmiRegs,fmbm_cfg1);
    DUMP_VAR(p_Fm->p_FmBmiRegs,fmbm_cfg2);
    DUMP_VAR(p_Fm->p_FmBmiRegs,fmbm_ievr);
    DUMP_VAR(p_Fm->p_FmBmiRegs,fmbm_ier);

    DUMP_TITLE(&p_Fm->p_FmBmiRegs->fmbm_arb, ("fmbm_arb"));
    DUMP_SUBSTRUCT_ARRAY(i, 8)
    {
        DUMP_MEMORY(&p_Fm->p_FmBmiRegs->fmbm_arb[i], sizeof(uint32_t));
    }


    DUMP_TITLE(p_Fm->p_FmQmiRegs, ("p_FmQmiRegs COMMON Regs"));
    DUMP_VAR(p_Fm->p_FmQmiRegs,fmqm_gc);
    DUMP_VAR(p_Fm->p_FmQmiRegs,fmqm_eie);
    DUMP_VAR(p_Fm->p_FmQmiRegs,fmqm_eien);
    DUMP_VAR(p_Fm->p_FmQmiRegs,fmqm_eif);
    DUMP_VAR(p_Fm->p_FmQmiRegs,fmqm_ie);
    DUMP_VAR(p_Fm->p_FmQmiRegs,fmqm_ien);
    DUMP_VAR(p_Fm->p_FmQmiRegs,fmqm_if);
    DUMP_VAR(p_Fm->p_FmQmiRegs,fmqm_gs);
    DUMP_VAR(p_Fm->p_FmQmiRegs,fmqm_ts);
    DUMP_VAR(p_Fm->p_FmQmiRegs,fmqm_etfc);

    return E_OK;
}
#endif /* (defined(DEBUG_ERRORS) && ... */
