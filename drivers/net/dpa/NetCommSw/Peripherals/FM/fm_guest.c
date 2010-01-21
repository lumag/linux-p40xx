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
#include "fm_ipc.h"
#include "fm.h"

/****************************************/
/*       Inter-Module functions        */
/****************************************/
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
    SANITY_CHECK_RETURN_ERROR(h_Fm, E_INVALID_HANDLE);

    return XX_SendMessage(((t_Fm*)h_Fm)->fmMasterModuleName, FM_GET_SET_PORT_PARAMS, (uint8_t*)p_PortParams, NULL, NULL);
}

void FmFreePortParams(t_Handle h_Fm,t_FmInterModulePortFreeParams *p_PortParams)
{
    t_Error err;

    SANITY_CHECK_RETURN(h_Fm, E_INVALID_HANDLE);

    err = XX_SendMessage(((t_Fm*)h_Fm)->fmMasterModuleName, FM_FREE_PORT, (uint8_t*)p_PortParams, NULL, NULL);
    if(err)
        REPORT_ERROR(MINOR, err, NO_MSG);
}


bool FmIsPortStalled(t_Handle h_Fm, uint8_t hardwarePortId)
{
    t_FmIpcPortIsStalled    isStalled;
    t_Error                 err;

    SANITY_CHECK_RETURN_VALUE(h_Fm, E_INVALID_HANDLE, FALSE);

    isStalled.hardwarePortId = hardwarePortId;
    err = XX_SendMessage(((t_Fm*)h_Fm)->fmMasterModuleName, FM_IS_PORT_STALLED, (uint8_t*)&isStalled, NULL, NULL);
    if(err)
        REPORT_ERROR(MINOR, err, NO_MSG);

    return isStalled.isStalled;
}

t_Error FmResumeStalledPort(t_Handle h_Fm, uint8_t hardwarePortId)
{
    SANITY_CHECK_RETURN_ERROR(h_Fm, E_INVALID_HANDLE);

    return XX_SendMessage(((t_Fm*)h_Fm)->fmMasterModuleName, FM_RESUME_STALLED_PORT, (uint8_t*)&hardwarePortId, NULL, NULL);
}

t_Error FmResetMac(t_Handle h_Fm, e_FmMacType type, uint8_t macId)
{
    t_FmIpcMacReset macReset;

    SANITY_CHECK_RETURN_ERROR(h_Fm, E_INVALID_HANDLE);

    macReset.id = macId;
    macReset.type = (e_FmIpcMacType)type;

    return XX_SendMessage(((t_Fm*)h_Fm)->fmMasterModuleName, FM_RESET_MAC, (uint8_t*)&macReset, NULL, NULL);
}

uint16_t FmGetClockFreq(t_Handle h_Fm)
{
    SANITY_CHECK_RETURN_ERROR(h_Fm, E_INVALID_HANDLE);

    return ((t_Fm*)h_Fm)->fmClkFreq;
}

uint32_t FmGetTimeStampPeriod(t_Handle h_Fm)
{
    uint32_t                timeStampPeriod;
    t_Error                 err;

    SANITY_CHECK_RETURN_VALUE(h_Fm, E_INVALID_HANDLE, 0);

    err = XX_SendMessage(((t_Fm*)h_Fm)->fmMasterModuleName, FM_GET_TIMESTAMP_PERIOD, (uint8_t*)&timeStampPeriod, NULL, NULL);
    if(err )
        REPORT_ERROR(MINOR, err, NO_MSG);

    return timeStampPeriod;
}

t_Error FmHandleIpcMsg(t_Handle h_Fm, uint32_t msgId, uint8_t msgBody[MSG_BODY_SIZE])
{
    t_Fm    *p_Fm = (t_Fm*)h_Fm;

    SANITY_CHECK_RETURN_ERROR(p_Fm, E_INVALID_HANDLE);

    switch(msgId)
    {
        case (FM_GUEST_ISR):
            if(((t_FmIpcIsr*)msgBody)->err)
                FM_ErrorIsr(h_Fm, uint32_t ((t_FmIpcIsr*)msgBody)->pending);
            else
                FM_EventIsr(h_Fm, uint32_t ((t_FmIpcIsr*)msgBody)->pending);
        break;
        default:
            RETURN_ERROR(MINOR, E_INVALID_SELECTION, ("command not found!!!"));
    }
    return E_OK;
}

void FmRegisterIntr(t_Handle h_Fm,
                        e_FmEventModules        module,
                        uint8_t                 modId,
                        bool                    err,
                        void (*f_Isr) (t_Handle h_Arg),
                        t_Handle    h_Arg)
{
    t_Fm                *p_Fm = (t_Fm*)h_Fm;
    uint8_t             event= 0;
    t_FmIpcRegisterIntr *fmIpcRegisterIntr;

    /* register in local FM structure */
    GET_FM_MODULE_EVENT(module, modId,err, event);
    ASSERT_COND(event != e_FM_EV_DUMMY_LAST);
    p_Fm->intrMng[event].f_Isr = f_Isr;
    p_Fm->intrMng[event].h_SrcHandle = h_Arg;

    /* register in Master FM structure */
    fmIpcRegisterIntr.event = event;
    fmIpcRegisterIntr.partitionId = p_Fm->partitionId;
    return XX_SendMessage(((t_Fm*)h_Fm)->fmMasterModuleName, FM_REGISTER_INTR, (uint8_t*)&fmIpcRegisterIntr, NULL, NULL);
}

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
t_Error FmDumpPortRegs (t_Handle h_Fm,uint8_t hardwarePortId)
{
    SANITY_CHECK_RETURN_ERROR(h_Fm, E_INVALID_HANDLE);

    return XX_SendMessage(((t_Fm*)h_Fm)->fmMasterModuleName, FM_DUMP_PORT_REGS, (uint8_t*)&hardwarePortId, NULL, NULL);
}
#endif /* (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0)) */

/****************************************/
/*       API Init unit functions        */
/****************************************/
t_Handle FM_Config(t_FmParams *p_FmParam)
{
    t_Fm        *p_Fm;
    uint8_t     i;

    /* Allocate FM structure */
    p_Fm = (t_Fm *) XX_Malloc(sizeof(t_Fm));
    if (!p_Fm)
    {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FM driver structure"));
        return NULL;
    }
    memset(p_Fm, 0, sizeof(t_Fm));

    /* Initialize FM parameters which will be kept by the driver */
    p_Fm->fmId              = p_FmParam->fmId;
    p_Fm->partitionId              = p_FmParam->partitionId;
    for(i = 0;i<FM_MAX_NUM_OF_PORTS;i++)
        p_Fm->portsTypes[i] = e_FM_PORT_TYPE_DUMMY;

    /* build the FM guest partition IPC address */
    memset(p_Fm->fmModuleName, 0, MODULE_NAME_SIZE);
    if(Sprint (p_Fm->fmModuleName, "FM-%d-%d",p_Fm->fmId, p_Fm->partitionId) != (p_Fm->partitionId<10 ? 6:7))
    {
        XX_Free(p_Fm);
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("Sprint failed"));
        return NULL;
    }

    /* build the FM master partition IPC address */
    memset(p_Fm->fmMasterModuleName, 0, MODULE_NAME_SIZE);
    if(Sprint (p_Fm->fmMasterModuleName, "FM-%d-Master",p_Fm->fmId) != 11)
    {
        XX_Free(p_Fm);
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("Sprint failed"));
        return NULL;
    }

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
    t_Error                 err;
    int                     i;

    for(i=0;i<e_FM_EV_DUMMY_LAST;i++)
        p_Fm->intrMng[i].f_Isr = UnimplementedIsr;

    err = XX_SendMessage(((t_Fm*)h_Fm)->fmMasterModuleName, FM_GET_CLK_FREQ, (uint8_t*)&(p_Fm->fmClkFreq), NULL, NULL);
    if(err )
        REPORT_ERROR(MINOR, err, NO_MSG);

    err = XX_RegisterMessageHandler(p_Fm->fmModuleName, FmHandleIpcMsg, p_Fm);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

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

    XX_Free(p_Fm);

    return E_OK;
}

/*************************************************/
/*       API Advanced Init unit functions        */
/*************************************************/

/****************************************************/
/*       API Run-time Control uint functions        */
/****************************************************/
t_Handle FM_GetPcdHandle(t_Handle h_Fm)
{
    SANITY_CHECK_RETURN_VALUE(h_Fm, E_INVALID_HANDLE, NULL);

    return ((t_Fm*)h_Fm)->h_Pcd;
}

void FM_GetRevision(t_Handle h_Fm, t_FmRevisionInfo *p_FmRevisionInfo)
{
    t_Error err;

    SANITY_CHECK_RETURN(h_Fm, E_INVALID_HANDLE);

    err = XX_SendMessage(((t_Fm*)h_Fm)->fmMasterModuleName, FM_GET_REV, (uint8_t*)p_FmRevisionInfo, NULL, NULL);
    if(err )
        REPORT_ERROR(MINOR, err, NO_MSG);
}

uint32_t  FM_GetCounter(t_Handle h_Fm, e_FmCounters counter)
{
    t_FmIpcGetCounter   counterParams;
    t_Error             err;

    SANITY_CHECK_RETURN_VALUE(h_Fm, E_INVALID_HANDLE, 0);

    counterParams.id = (e_FmIpcCounters)counter;
    err = XX_SendMessage(((t_Fm*)h_Fm)->fmMasterModuleName, FM_GET_COUNTER, (uint8_t*)&counterParams, NULL, NULL);
    if(err )
        REPORT_ERROR(MINOR, err, NO_MSG);

    return counterParams.val;
}

void FM_ErrorIsr(t_Handle h_Fm, uint32_t pending)
{
    t_Fm                    *p_Fm = (t_Fm*)h_Fm;
    uint32_t                pending;

    SANITY_CHECK_RETURN(h_Fm, E_INVALID_HANDLE);

    /* error interrupts */
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

void FM_EventIsr(t_Handle h_Fm, uint32_t pending)
{
    t_Fm                    *p_Fm = (t_Fm*)h_Fm;
    uint32_t                pending;

    SANITY_CHECK_RETURN(h_Fm, E_INVALID_HANDLE);

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

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
t_Error FM_DumpRegs(t_Handle h_Fm)
{
    SANITY_CHECK_RETURN_ERROR(h_Fm, E_INVALID_HANDLE);

    return XX_SendMessage(((t_Fm*)h_Fm)->fmMasterModuleName, FM_DUMP_REGS, NULL, NULL, NULL);
}
#endif /* (defined(DEBUG_ERRORS) && ... */
