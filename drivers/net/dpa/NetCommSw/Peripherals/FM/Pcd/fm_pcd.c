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
 @File          fm_pcd.c

 @Description   FM PCD ...
*//***************************************************************************/
#include "std_ext.h"
#include "error_ext.h"
#include "string_ext.h"
#include "xx_ext.h"
#include "sprint_ext.h"
#include "debug_ext.h"
#include "net_ext.h"
#include "fm_ext.h"
#include "fm_pcd_ext.h"
#include "fm_hc.h"
#ifdef FM_MASTER_PARTITION
#include "fm_pcd_ipc.h"
#endif /* FM_MASTER_PARTITION */
#include "fm_pcd.h"


#ifndef CONFIG_GUEST_PARTITION
static t_Error CheckFmPcdParameters(t_FmPcd *p_FmPcd)
{
    if(p_FmPcd->p_FmPcdKg && !p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Something WRONG"));

    if(p_FmPcd->p_FmPcdPlcr && !p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Something WRONG"));

    if(!p_FmPcd->h_Fm)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("h_Fm has to be initialized"));

    if(!p_FmPcd->f_FmPcdException)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("f_FmPcdExceptions has to be initialized"));

    if((!p_FmPcd->f_FmPcdIndexedException) && (p_FmPcd->p_FmPcdPlcr || p_FmPcd->p_FmPcdKg))
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("f_FmPcdIndexedException has to be initialized"));

   if(p_FmPcd->p_FmPcdDriverParam->prsMaxParseCycleLimit > PRS_MAX_CYCLE_LIMIT)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("prsMaxParseCycleLimit has to be less than 8191"));


    return E_OK;
}
#endif  /* !CONFIG_GUEST_PARTITION */

#ifdef FM_MASTER_PARTITION
t_Error  FmPcdHandleIpcMsg(t_Handle h_FmPcd, uint32_t msgId, uint8_t msgBody[MSG_BODY_SIZE])
{
    t_FmPcd                     *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint8_t                     schemeId;
    t_Error                     err;
    t_FmPcdKgInterModuleClsPlanSet           clsPlanSet;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);

    switch(msgId)
    {
    case (FM_PCD_MASTER_IS_ALIVE):
        return E_OK;
    case (FM_PCD_MASTER_IS_ENABLED):
        return p_FmPcd->enabled;
        //case (FM_PCD_CLEAR_PORT_PARAMS):
            //return FmPcdDeletePortParams(h_FmPcd, (t_FmPcdInterModulePortDeleteParams*)msgBody);
        case (FM_PCD_ALLOC_KG_RSRC):
            {
                err = FmPcdKgAllocSchemes(h_FmPcd,
                                            ((t_FmPcdIpcKgAllocParams*)msgBody)->numOfSchemes,
                                            ((t_FmPcdIpcKgAllocParams*)msgBody)->partitionId,
                                            ((t_FmPcdIpcKgAllocParams*)msgBody)->schemesIds);
                if(err)
                    RETURN_ERROR(MINOR, err, NO_MSG);

                err = FmPcdKgAllocClsPlanEntries(h_FmPcd,
                                            ((t_FmPcdIpcKgAllocParams*)msgBody)->numOfClsPlanEntries,
                                            ((t_FmPcdIpcKgAllocParams*)msgBody)->partitionId,
                                            &((t_FmPcdIpcKgAllocParams*)msgBody)->clsPlanBase);
                if(err)
                    RETURN_ERROR(MINOR, err, NO_MSG);
                /* build vectors of 0xFFFFFFFF for creating a private clsPlan group */

            }
            break;

        case (FM_PCD_ALLOC_CLS_PLAN_EMPTY_GRP):
            {
                memset(clsPlanSet.vectors, 0xFF, CLS_PLAN_NUM_PER_GRP*sizeof(uint32_t));
                clsPlanSet.baseEntry = *(uint8_t*)msgBody;
                clsPlanSet.numOfClsPlanEntries = CLS_PLAN_NUM_PER_GRP;
                KgSetClsPlan(h_FmPcd, &clsPlanSet);
            }
            break;
        case (FM_PCD_FREE_KG_RSRC):
            {
                err = FmPcdKgFreeSchemes(h_FmPcd,
                                            ((t_FmPcdIpcKgAllocParams*)msgBody)->numOfSchemes,
                                            ((t_FmPcdIpcKgAllocParams*)msgBody)->partitionId,
                                            ((t_FmPcdIpcKgAllocParams*)msgBody)->schemesIds);
                if(err)
                    RETURN_ERROR(MINOR, err, NO_MSG);

                err = FmPcdKgFreeClsPlanEntries(h_FmPcd,
                                            ((t_FmPcdIpcKgAllocParams*)msgBody)->numOfClsPlanEntries,
                                            ((t_FmPcdIpcKgAllocParams*)msgBody)->partitionId,
                                            ((t_FmPcdIpcKgAllocParams*)msgBody)->clsPlanBase);
                if(err)
                    RETURN_ERROR(MINOR, err, NO_MSG);
                /* build vectors of 0xFFFFFFFF for creating a private clsPlan group */

                if(((t_FmPcdIpcKgAllocParams*)msgBody)->isDriverClsPlanGrp)
                {
                    memset(clsPlanSet.vectors, 0x00, CLS_PLAN_NUM_PER_GRP*sizeof(uint32_t));
                    clsPlanSet.baseEntry = ((t_FmPcdIpcKgAllocParams*)msgBody)->clsPlanBase;
                    clsPlanSet.numOfClsPlanEntries = CLS_PLAN_NUM_PER_GRP;
                    KgSetClsPlan(h_FmPcd, &clsPlanSet);
                }
            }
            break;
        case (FM_PCD_ALLOC_PROFILES):
            return PlcrAllocProfiles(h_FmPcd,
                                        ((t_FmPcdIpcPlcrAllocParams*)msgBody)->hardwarePortId,
                                        ((t_FmPcdIpcPlcrAllocParams*)msgBody)->num,
                                        &((t_FmPcdIpcPlcrAllocParams*)msgBody)->plcrProfilesBase);
        case (FM_PCD_FREE_PROFILES):
            return PlcrFreeProfiles(h_FmPcd,
                                        ((t_FmPcdIpcPlcrAllocParams*)msgBody)->hardwarePortId,
                                        ((t_FmPcdIpcPlcrAllocParams*)msgBody)->num,
                                        ((t_FmPcdIpcPlcrAllocParams*)msgBody)->plcrProfilesBase);

        case (FM_PCD_ALLOC_SHARED_PROFILES):
            return PlcrAllocSharedProfiles(h_FmPcd,
                                        ((t_FmPcdIpcSharedPlcrAllocParams*)msgBody)->num,
                                        ((t_FmPcdIpcSharedPlcrAllocParams*)msgBody)->profilesIds);
        case (FM_PCD_FREE_SHARED_PROFILES):
            PlcrFreeSharedProfiles(h_FmPcd,
                                        ((t_FmPcdIpcSharedPlcrAllocParams*)msgBody)->num,
                                        ((t_FmPcdIpcSharedPlcrAllocParams*)msgBody)->profilesIds);
            break;
        case (FM_PCD_GET_PHYS_MURAM_BASE):
            return FmGetPhysicalMuramBase(p_FmPcd->h_Fm, (t_FmPhysAddr*)msgBody);
        case(FM_PCD_GET_SW_PRS_OFFSET):
            ((t_FmPcdIpcSwPrsLable*)msgBody)->offset = FmPcdGetSwPrsOffset(h_FmPcd, ((t_FmPcdIpcSwPrsLable*)msgBody)->hdr, ((t_FmPcdIpcSwPrsLable*)msgBody)->indexPerHdr);
            if(((t_FmPcdIpcSwPrsLable*)msgBody)->offset == ILLEGAL_BASE)
                RETURN_ERROR(MINOR, E_INVALID_VALUE, NO_MSG);


            break;
       /* case(FM_PCD_GET_SET_KG_SCHEME_HC_PARAMS):
            return FmPcdKgGetSetSchemeParams(h_FmPcd, (t_FmPcdInterModuleKgSchemeParams*)msgBody);*/
        case(FM_PCD_FREE_KG_SCHEME_HC):
            schemeId = *(uint8_t*)msgBody;
            FmPcdKgInvalidateSchemeSw(h_FmPcd, schemeId);
            break;
        default:
            RETURN_ERROR(MINOR, E_INVALID_SELECTION, ("command not found!!!"));
    }
    return E_OK;
}
#endif /* FM_MASTER_PARTITION */

t_Error PcdGetVectorForOpt(t_FmPcd *p_FmPcd, uint8_t netEnvId, protocolOpt_t opt, uint32_t *p_Vector)
{
    uint8_t     j,k;

    *p_Vector = 0;

    for(j=0;(p_FmPcd->netEnvs[netEnvId].units[j].hdrs[0].hdr != HEADER_TYPE_NONE)
            && (j < FM_PCD_MAX_NUM_OF_DISTINCTION_UNITS) ;j++)
        for(k=0;(p_FmPcd->netEnvs[netEnvId].units[j].hdrs[k].hdr != HEADER_TYPE_NONE)
                && (k < FM_PCD_MAX_NUM_OF_INTERCHANGABLE_HDRS) ;k++)
        {
            if(p_FmPcd->netEnvs[netEnvId].units[j].hdrs[k].opt == opt)
                *p_Vector |= p_FmPcd->netEnvs[netEnvId].unitsVectors[j];
        }

    if (!*p_Vector)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Requested option was not defined for this Network Environment Characteristics module"));
    else
        return E_OK;
}

t_Error PcdGetUnitsVector(t_FmPcd *p_FmPcd, t_NetEnvParams *p_Params)
{
    int                     i;

    p_Params->vector = 0;
    for(i=0; i<p_Params->numOfDistinctionUnits ;i++)
    {
        if(p_FmPcd->netEnvs[p_Params->netEnvId].units[p_Params->unitIds[i]].hdrs[0].hdr == HEADER_TYPE_NONE)
            RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Requested unit was not defined for this Network Environment Characteristics module"));
        ASSERT_COND(p_FmPcd->netEnvs[p_Params->netEnvId].unitsVectors[p_Params->unitIds[i]]);
        p_Params->vector |= p_FmPcd->netEnvs[p_Params->netEnvId].unitsVectors[p_Params->unitIds[i]];
    }

    return E_OK;
}

bool PcdNetEnvIsUnitWithoutOpts(t_FmPcd *p_FmPcd, uint8_t netEnvId, uint32_t unitVector)
{
    int     i=0, k;
    /* check whether a given unit may be used by non-clsPlan users. */
    /* first, recognize the unit by its vector */
    while (p_FmPcd->netEnvs[netEnvId].units[i].hdrs[0].hdr != HEADER_TYPE_NONE)
    {
        if (p_FmPcd->netEnvs[netEnvId].unitsVectors[i] == unitVector)
        {
            for (k=0;
                 ((k < FM_PCD_MAX_NUM_OF_INTERCHANGABLE_HDRS) &&
                  (p_FmPcd->netEnvs[netEnvId].units[i].hdrs[k].hdr != HEADER_TYPE_NONE));
                 k++)
                /* check that no option exists */
                if((protocolOpt_t)p_FmPcd->netEnvs[netEnvId].units[i].hdrs[k].opt)
                    return FALSE;
            break;
        }
        i++;
    }
    /* assert that a unit was found to mach the vector */
    ASSERT_COND(p_FmPcd->netEnvs[netEnvId].units[i].hdrs[0].hdr != HEADER_TYPE_NONE);

    return TRUE;
}

void   FmPcdPortRegister(t_Handle h_FmPcd, t_Handle h_FmPort, uint8_t hardwarePortId)
{
    t_FmPcd         *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint16_t        pcdPortId;
    uint8_t         portsTable[] = PCD_PORTS_TABLE;

    GET_FM_PCD_PORTID_FROM_GLOBAL_PORTID(pcdPortId, portsTable, hardwarePortId)

    p_FmPcd->p_FmPcdPlcr->portsMapping[pcdPortId].h_FmPort = h_FmPort;
}

uint32_t FmPcdGetLcv(t_Handle h_FmPcd, uint32_t netEnvId, uint8_t hdrNum)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;

    return p_FmPcd->netEnvs[netEnvId].lcvs[hdrNum];
}

#ifdef CONFIG_MULTI_PARTITION_SUPPORT
uint8_t FmPcdGetPartitionId(t_Handle h_FmPcd)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;

    return p_FmPcd->partitionId;
}
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */

t_Handle FmPcdGetHcHandle(t_Handle h_FmPcd)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;

    return p_FmPcd->h_Hc;
}

void FmPcdIncNetEnvOwners(t_Handle h_FmPcd, uint8_t netEnvId)
{
    ((t_FmPcd*)h_FmPcd)->netEnvs[netEnvId].owners++;
}

void FmPcdDecNetEnvOwners(t_Handle h_FmPcd, uint8_t netEnvId)
{
    ASSERT_COND(((t_FmPcd*)h_FmPcd)->netEnvs[netEnvId].owners);
    ((t_FmPcd*)h_FmPcd)->netEnvs[netEnvId].owners--;
}

t_Error FmPcdTryLock(t_Handle h_FmPcd)
{
    TRY_LOCK_RET_ERR(((t_FmPcd*)h_FmPcd)->lock);
    return E_OK;
}

void FmPcdReleaseLock(t_Handle h_FmPcd)
{
    RELEASE_LOCK(((t_FmPcd*)h_FmPcd)->lock);
}

/**********************************************************************************************************/
/*              API                                                                                       */
/**********************************************************************************************************/

t_Handle FM_PCD_Config(t_FmPcdParams *p_FmPcdParams)
{
    t_FmPcd *p_Pcd = NULL;

    SANITY_CHECK_RETURN_VALUE(p_FmPcdParams, E_INVALID_HANDLE,NULL);

    p_Pcd = (t_FmPcd *) XX_Malloc(sizeof(t_FmPcd));
    if (!p_Pcd)
    {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FM Pcd"));
        return NULL;
    }
    memset(p_Pcd, 0, sizeof(t_FmPcd));

    p_Pcd->p_FmPcdDriverParam = (t_FmPcdDriverParam *) XX_Malloc(sizeof(t_FmPcdDriverParam));
    if (!p_Pcd)
    {
        XX_Free(p_Pcd);
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FM Pcd Driver Param"));
        return NULL;
    }
    memset(p_Pcd->p_FmPcdDriverParam, 0, sizeof(t_FmPcdDriverParam));

    p_Pcd->h_Fm = p_FmPcdParams->h_Fm;

#ifndef CONFIG_MULTI_PARTITION_SUPPORT
    if (p_FmPcdParams->useHostCommand)
#endif  /* !CONFIG_MULTI_PARTITION_SUPPORT */
    {
        t_FmHcParams    hcParams;

        memset(&hcParams, 0, sizeof(hcParams));
        hcParams.h_Fm = p_Pcd->h_Fm;
        hcParams.h_FmPcd = (t_Handle)p_Pcd;
        memcpy((uint8_t*)&hcParams.params, (uint8_t*)&p_FmPcdParams->hc, sizeof(t_FmPcdHcParams));
        p_Pcd->h_Hc = FmHcConfigAndInit(&hcParams);
        if (!p_Pcd->h_Hc)
        {
            REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FM Pcd HC"));
            FM_PCD_Free(p_Pcd);
            return NULL;
        }
    }

    if(p_FmPcdParams->kgSupport)
    {
        p_Pcd->p_FmPcdKg = (t_FmPcdKg *)KgConfig(p_Pcd, p_FmPcdParams);
        if(!p_Pcd->p_FmPcdKg)
        {
            REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FM Pcd Keygen"));
            FM_PCD_Free(p_Pcd);
            return NULL;
        }
    }

    if(p_FmPcdParams->ccSupport)
    {
        p_Pcd->p_FmPcdCc = (t_FmPcdCc *)CcConfig(p_Pcd, p_FmPcdParams);
        if(!p_Pcd->p_FmPcdCc)
        {
            REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FM Pcd Cc"));
            FM_PCD_Free(p_Pcd);
            return NULL;
        }
    }

    if(p_FmPcdParams->plcrSupport)
    {
        p_Pcd->p_FmPcdPlcr = (t_FmPcdPlcr *)PlcrConfig(p_Pcd, p_FmPcdParams);
        if(!p_Pcd->p_FmPcdPlcr)
        {
            REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FM Pcd Policer"));
            FM_PCD_Free(p_Pcd);
            return NULL;
        }

    }

    if(p_FmPcdParams->prsSupport)
    {
        p_Pcd->p_FmPcdPrs = (t_FmPcdPrs *)PrsConfig(p_Pcd, p_FmPcdParams);
        if(!p_Pcd->p_FmPcdPrs)
        {
            REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FM Pcd Parser"));
            FM_PCD_Free(p_Pcd);
            return NULL;
        }
    }

#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    p_Pcd->partitionId                                      = p_FmPcdParams->partitionId;
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */

#ifndef CONFIG_GUEST_PARTITION
    p_Pcd->h_App                                            = p_FmPcdParams->h_App;
    p_Pcd->f_FmPcdException                                 = p_FmPcdParams->f_FmPcdException;
    p_Pcd->f_FmPcdIndexedException                          = p_FmPcdParams->f_FmPcdIdException;
#endif  /* !CONFIG_GUEST_PARTITION */

    return p_Pcd;
}

t_Error FM_PCD_Init(t_Handle h_FmPcd)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;
    t_Error     err;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdDriverParam, E_INVALID_HANDLE);


#ifdef CONFIG_GUEST_PARTITION
    err = XX_SendMessage(p_FmPcd->fmPcdModuleName, FM_PCD_MASTER_IS_ALIVE, NULL, NULL, NULL);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);
#else
    CHECK_INIT_PARAMETERS(p_FmPcd, CheckFmPcdParameters);
#endif /* CONFIG_GUEST_PARTITION */

    if(p_FmPcd->p_FmPcdKg)
    {
        err = KgInit(p_FmPcd);
        if(err)
            RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    if(p_FmPcd->p_FmPcdPlcr)
    {
        err = PlcrInit(p_FmPcd);
        if(err)
            RETURN_ERROR(MAJOR, err, NO_MSG);
    }

#ifndef CONFIG_GUEST_PARTITION
    if(p_FmPcd->p_FmPcdPrs)
    {
        err = PrsInit(p_FmPcd);
        if(err)
            RETURN_ERROR(MAJOR, err, NO_MSG);
    }
#endif  /* CONFIG_GUEST_PARTITION */
#ifdef FM_MASTER_PARTITION
     /* register to inter-core messaging mechanism */
    memset(p_FmPcd->fmPcdModuleName, 0, MODULE_NAME_SIZE);
    if(Sprint (p_FmPcd->fmPcdModuleName, "FM-%d.PCD",FmGetId(p_FmPcd->h_Fm)) != 8)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Sprint failed"));
    err = XX_RegisterMessageHandler(p_FmPcd->fmPcdModuleName, FmPcdHandleIpcMsg, p_FmPcd);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);
#endif /* FM_MASTER_PARTITION */

    XX_Free(p_FmPcd->p_FmPcdDriverParam);
    p_FmPcd->p_FmPcdDriverParam = NULL;

    FmRegisterPcd(p_FmPcd->h_Fm, p_FmPcd);

    return E_OK;
}

t_Error FM_PCD_Free(t_Handle h_FmPcd)
{
    t_FmPcd                             *p_FmPcd =(t_FmPcd *)h_FmPcd;
#ifdef CONFIG_GUEST_PARTITION
    t_FmPcdIpcSharedPlcrAllocParams     ipcSharedPlcrParams;
    t_FmPcdIpcKgAllocParams             kgAlloc;
#endif /* CONFIG_GUEST_PARTITION */
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    t_Error                             err;
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */


    if(p_FmPcd->p_FmPcdDriverParam)
    {
        XX_Free(p_FmPcd->p_FmPcdDriverParam);
        p_FmPcd->p_FmPcdDriverParam = NULL;
    }
    if(p_FmPcd->p_FmPcdKg)
    {

#ifndef CONFIG_GUEST_PARTITION
        if(p_FmPcd->p_FmPcdKg->isDriverEmptyClsPlanGrp)
            FmPcdKgDestroyClsPlanGrp(p_FmPcd, p_FmPcd->p_FmPcdKg->emptyClsPlanGrpId);
#endif
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
#ifdef CONFIG_GUEST_PARTITION
    kgAlloc.numOfSchemes = p_FmPcd->p_FmPcdKg->numOfSchemes;
    kgAlloc.partitionId = p_FmPcd->partitionId;
    kgAlloc.numOfClsPlanEntries = p_FmPcd->p_FmPcdKg->numOfClsPlanEntries;
    kgAlloc.isDriverClsPlanGrp = p_FmPcd->p_FmPcdKg->isDriverEmptyClsPlanGrp;
    kgAlloc.clsPlanBase = p_FmPcd->p_FmPcdKg->clsPlanBase;
    memcpy(kgAlloc.schemesIds, p_FmPcd->p_FmPcdKg->schemesIds , kgAlloc.numOfSchemes);
    err = XX_SendMessage(p_FmPcd->fmPcdModuleName, FM_PCD_FREE_KG_RSRC, (uint8_t*)&kgAlloc, NULL, NULL);
    if(err)
        RETURN_ERROR(MINOR, err, NO_MSG);
#else /* master */
    err = FmPcdKgFreeSchemes(p_FmPcd,
                                p_FmPcd->p_FmPcdKg->numOfSchemes,
                                p_FmPcd->partitionId,
                                p_FmPcd->p_FmPcdKg->schemesIds);
    if(err)
        RETURN_ERROR(MINOR, err, NO_MSG);
    err = FmPcdKgFreeClsPlanEntries(p_FmPcd,
                                p_FmPcd->p_FmPcdKg->numOfClsPlanEntries,
                                p_FmPcd->partitionId,
                                p_FmPcd->p_FmPcdKg->clsPlanBase);
#endif /* CONFIG_GUEST_PARTITION */
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
        XX_Free(p_FmPcd->p_FmPcdKg);
        p_FmPcd->p_FmPcdKg = NULL;
    }
    if(p_FmPcd->p_FmPcdPlcr)
    {
        if(p_FmPcd->p_FmPcdPlcr->numOfSharedProfiles)
#ifdef CONFIG_GUEST_PARTITION
    /* Alloc resources using IPC messaging */
    ipcSharedPlcrParams.num = p_FmPcd->p_FmPcdPlcr->numOfSharedProfiles;
    memcpy(ipcSharedPlcrParams.profilesIds,p_FmPcd->p_FmPcdPlcr->sharedProfilesIds, p_FmPcd->p_FmPcdPlcr->numOfSharedProfiles*sizeof(uint16_t));
    err = XX_SendMessage(p_FmPcd->fmPcdModuleName, FM_PCD_FREE_SHARED_PROFILES, (uint8_t*)&ipcSharedPlcrParams, NULL, NULL);
    if(err)
        RETURN_ERROR(MAJOR, err,NO_MSG);
#else /* master */
        PlcrFreeSharedProfiles(p_FmPcd, p_FmPcd->p_FmPcdPlcr->numOfSharedProfiles, p_FmPcd->p_FmPcdPlcr->sharedProfilesIds);
#endif /* CONFIG_GUEST_PARTITION */
        XX_Free(p_FmPcd->p_FmPcdPlcr);
        p_FmPcd->p_FmPcdPlcr = NULL;
    }
    if(p_FmPcd->p_FmPcdPrs)
    {
        XX_Free(p_FmPcd->p_FmPcdPrs);
        p_FmPcd->p_FmPcdPrs = NULL;
    }
    if(p_FmPcd->p_FmPcdCc)
    {
        CcFree(p_FmPcd->p_FmPcdCc);
        XX_Free(p_FmPcd->p_FmPcdCc);
        p_FmPcd->p_FmPcdCc = NULL;
    }

    if (p_FmPcd->h_Hc)
    {
        FmHcFree(p_FmPcd->h_Hc);
        p_FmPcd->h_Hc = NULL;
    }

#ifdef FM_MASTER_PARTITION
    XX_UnregisterMessageHandler(p_FmPcd->fmPcdModuleName);
#endif /* FM_MASTER_PARTITION */

    XX_Free(p_FmPcd);
    return E_OK;
}

t_Error FM_PCD_Enable(t_Handle h_FmPcd)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd*)h_FmPcd;
    t_Error             err;

#ifndef CONFIG_GUEST_PARTITION
    if(p_FmPcd->p_FmPcdKg)
    {
        err = KgEnable(p_FmPcd);
        if(err)
            RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    if(p_FmPcd->p_FmPcdPlcr)
    {
        err = PlcrEnable(p_FmPcd);
        if(err)
            RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    if(p_FmPcd->p_FmPcdPrs)
    {
        err = PrsEnable(p_FmPcd);
        if(err)
            RETURN_ERROR(MAJOR, err, NO_MSG);
    }
    p_FmPcd->enabled = TRUE;

    return E_OK;

#else
    return XX_SendMessage(p_FmPcd->fmPcdModuleName, FM_PCD_MASTER_IS_ENABLED, NULL, NULL, NULL);
#endif /* !CONFIG_GUEST_PARTITION */
}

t_Error FM_PCD_Disable(t_Handle h_FmPcd)
{
/*    t_FmPcd             *p_FmPcd = (t_FmPcd*)h_FmPcd;*/

    RETURN_ERROR(MINOR, E_NOT_SUPPORTED, NO_MSG);

    return E_OK;
}

t_Handle FM_PCD_SetNetEnvCharacteristics(t_Handle h_FmPcd, t_FmPcdNetEnvParams  *p_NetEnvParams)
{
    t_FmPcd                 *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint8_t                 bitId = 0;
    uint8_t                 privateBitId = 0;
    uint8_t                 i, j, k;
    uint8_t                 netEnvCurrId;
    uint8_t                 ipsecAhUnit = 0,ipsecEspUnit = 0;
    bool                    ipsecAhExists = FALSE,ipsecEspExists = FALSE;
    uint8_t                 hdrNum;

    SANITY_CHECK_RETURN_VALUE(h_FmPcd, E_INVALID_STATE, NULL);
    SANITY_CHECK_RETURN_VALUE(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE, NULL);

    TRY_LOCK_RET_NULL(p_FmPcd->lock);

    /* find a new netEnv */
    for(i = 0;i<PCD_MAX_NUM_OF_PORTS;i++)
        if(!p_FmPcd->netEnvs[i].used)
            break;

    if(i== PCD_MAX_NUM_OF_PORTS)
    {
        REPORT_ERROR(MAJOR, E_FULL,("No more than %d netEnv's allowed.", PCD_MAX_NUM_OF_PORTS));
        RELEASE_LOCK(p_FmPcd->lock);
        return NULL;
    }

    p_FmPcd->netEnvs[i].used = TRUE;

    TRY_LOCK_RET_NULL(p_FmPcd->netEnvs[i].lock);
    RELEASE_LOCK(p_FmPcd->lock);

    netEnvCurrId = (uint8_t)i;

    /* clear from previous use */
    memset(&p_FmPcd->netEnvs[netEnvCurrId].units, 0, FM_PCD_MAX_NUM_OF_DISTINCTION_UNITS);
    memcpy(&p_FmPcd->netEnvs[netEnvCurrId].units, p_NetEnvParams->units, p_NetEnvParams->numOfDistinctionUnits*sizeof(t_FmPcdDistinctionUnit));

    /* check that header with opt is not interchanged with the same header */
    for(i=0;(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[0].hdr != HEADER_TYPE_NONE)
            && (i < FM_PCD_MAX_NUM_OF_DISTINCTION_UNITS) ;i++)
        for(k=0;(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[k].hdr != HEADER_TYPE_NONE)
                && (k < FM_PCD_MAX_NUM_OF_INTERCHANGABLE_HDRS) ;k++)
        {
            /* if an option exists, check that other headers are not the same header
            without option */
            if(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[k].opt)
            {
                for(j=0;(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[j].hdr != HEADER_TYPE_NONE)
                        && (j < FM_PCD_MAX_NUM_OF_INTERCHANGABLE_HDRS) ;j++)
                    if((p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[j].hdr == p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[k].hdr) &&
                        !p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[j].opt)
                    {
                        REPORT_ERROR(MINOR, E_FULL, ("Illegal unit - header with opt may not be interchangable with the same header without opt"));
                        RELEASE_LOCK(p_FmPcd->netEnvs[netEnvCurrId].lock);
                        return NULL;
                    }
            }
        }

    /* IPSEC_AH and IPSEC_SPI can't be 2 units,  */
    /* check that header with opt is not interchanged with the same header */
    for(i=0;(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[0].hdr != HEADER_TYPE_NONE)
            && (i < FM_PCD_MAX_NUM_OF_DISTINCTION_UNITS) ;i++)
        for(k=0;(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[k].hdr != HEADER_TYPE_NONE)
                && (k < FM_PCD_MAX_NUM_OF_INTERCHANGABLE_HDRS) ;k++)
        {
            /* Some headers pairs may not be defined on different units as the parser
            doesn't distingush  */
            if(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[k].hdr == HEADER_TYPE_IPSEC_AH)
            {
                if (ipsecEspExists && (ipsecEspUnit != i))
                {
                    REPORT_ERROR(MINOR, E_INVALID_STATE, ("HEADER_TYPE_IPSEC_AH and HEADER_TYPE_IPSEC_ESP may not be defined in separate units"));
                    RELEASE_LOCK(p_FmPcd->netEnvs[netEnvCurrId].lock);
                   return NULL;
                }
                else
                {
                    ipsecAhUnit = i;
                    ipsecAhExists = TRUE;
                }
            }
            if(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[k].hdr == HEADER_TYPE_IPSEC_ESP)
            {
                if (ipsecAhExists && (ipsecAhUnit != i))
                {
                    REPORT_ERROR(MINOR, E_INVALID_STATE, ("HEADER_TYPE_IPSEC_AH and HEADER_TYPE_IPSEC_ESP may not be defined in separate units"));
                    RELEASE_LOCK(p_FmPcd->netEnvs[netEnvCurrId].lock);
                    return NULL;
                }
                else
                {
                    ipsecEspUnit = i;
                    ipsecEspExists = TRUE;
                }
            }
       }


    /* if private header (shim), check that no other headers specified */
    for(i=0;(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[0].hdr != HEADER_TYPE_NONE)
            && (i < FM_PCD_MAX_NUM_OF_DISTINCTION_UNITS) ;i++)
    {
        if(IS_PRIVATE_HEADER(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[0].hdr))
            if(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[1].hdr != HEADER_TYPE_NONE)
            {
                REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("SHIM header may not be interchangesd with other headers"));
                RELEASE_LOCK(p_FmPcd->netEnvs[netEnvCurrId].lock);
                return NULL;
            }
    }

    for(i=0; i<p_NetEnvParams->numOfDistinctionUnits;i++)
    {
        if (IS_PRIVATE_HEADER(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[0].hdr))
            p_FmPcd->netEnvs[netEnvCurrId].unitsVectors[i] = (uint32_t)(0x00000001 << privateBitId++);
        else
            p_FmPcd->netEnvs[netEnvCurrId].unitsVectors[i] = (uint32_t)(0x80000000 >> bitId++);
    }

    /* define a set of hardware parser LCV's according to the defined netenv */

    /* set an array of LCV's for each header in the netEnv */
    for(i=0;(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[0].hdr != HEADER_TYPE_NONE)
            && (i < FM_PCD_MAX_NUM_OF_DISTINCTION_UNITS) ;i++)
        /* private headers have no LCV in the hard parser */
        if (!IS_PRIVATE_HEADER(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[0].hdr))
        {
            for(k=0;(p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[k].hdr != HEADER_TYPE_NONE)
                    && (k < FM_PCD_MAX_NUM_OF_INTERCHANGABLE_HDRS) ;k++)
            {
                GET_PRS_HDR_NUM(hdrNum, p_FmPcd->netEnvs[netEnvCurrId].units[i].hdrs[k].hdr);
                if (hdrNum == ILLEGAL_HDR_NUM)
                {
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, NO_MSG);
                    RELEASE_LOCK(p_FmPcd->netEnvs[netEnvCurrId].lock);
                    return NULL;
                }
                p_FmPcd->netEnvs[netEnvCurrId].lcvs[hdrNum] |= p_FmPcd->netEnvs[netEnvCurrId].unitsVectors[i];
            }
        }

    RELEASE_LOCK(p_FmPcd->netEnvs[netEnvCurrId].lock);

    return CAST_UINT32_TO_POINTER((uint32_t)netEnvCurrId+1);
}

t_Error FM_PCD_DeleteNetEnvCharacteristics(t_Handle h_FmPcd, t_Handle h_NetEnv)
{
    t_FmPcd                 *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint8_t                 netEnvId = (uint8_t)(CAST_POINTER_TO_UINT32(h_NetEnv)-1);
    SANITY_CHECK_RETURN_ERROR(h_FmPcd, E_INVALID_STATE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE);

    TRY_LOCK_RET_ERR(p_FmPcd->netEnvs[netEnvId].lock);
    /* check that no port is bound to this netEnv */
    if(p_FmPcd->netEnvs[netEnvId].owners)
       RETURN_ERROR(MINOR, E_INVALID_STATE, ("Trying to delete a netEnv that has ports/schemes/trees/clsPlanGrps bound to"));

    p_FmPcd->netEnvs[netEnvId].used= FALSE;

    RELEASE_LOCK(p_FmPcd->netEnvs[netEnvId].lock);

    return E_OK;
}

void FM_PCD_HcTxConf(t_Handle h_FmPcd, t_FmFD *p_Fd)
{
    t_FmPcd                 *p_FmPcd = (t_FmPcd*)h_FmPcd;

    SANITY_CHECK_RETURN(h_FmPcd, E_INVALID_STATE);

    FmHcTxConf(p_FmPcd->h_Hc, p_Fd);
}

#ifndef CONFIG_GUEST_PARTITION
t_Error FM_PCD_ConfigException(t_Handle h_FmPcd, e_FmPcdExceptions exception, bool enable)
{
    t_FmPcd         *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint32_t        bitMask = 0;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);

    GET_FM_PCD_EXCEPTION_FLAG(bitMask, exception);
    if(bitMask)
    {
        if (enable)
            p_FmPcd->exceptions |= bitMask;
        else
            p_FmPcd->exceptions &= ~bitMask;
   }
    else
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Undefined exception"));

    return E_OK;
}

t_Error FM_PCD_SetException(t_Handle h_FmPcd, e_FmPcdExceptions exception, bool enable)
{
    t_FmPcd         *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint32_t        bitMask = 0, tmpReg;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE);

    GET_FM_PCD_EXCEPTION_FLAG(bitMask, exception);

    if(bitMask)
    {
        if (enable)
            p_FmPcd->exceptions |= bitMask;
        else
            p_FmPcd->exceptions &= ~bitMask;

        switch(exception)
        {
            case(e_FM_PCD_KG_ERR_EXCEPTION_DOUBLE_ECC):
            case(e_FM_PCD_KG_ERR_EXCEPTION_KEYSIZE_OVERFLOW):
                if(!p_FmPcd->p_FmPcdKg)
                    RETURN_ERROR(MINOR, E_INVALID_STATE, ("Can't ask for this interrupt - keygen is not working"));
                break;
            case(e_FM_PCD_PLCR_ERR_EXCEPTION_DOUBLE_ECC):
            case(e_FM_PCD_PLCR_ERR_EXCEPTION_INIT_ENTRY_ERROR):
            case(e_FM_PCD_PLCR_EXCEPTION_PRAM_SELF_INIT_COMPLETE):
            case(e_FM_PCD_PLCR_EXCEPTION_ATOMIC_ACTION_COMPLETE):
                if(!p_FmPcd->p_FmPcdPlcr)
                    RETURN_ERROR(MINOR, E_INVALID_STATE, ("Can't ask for this interrupt - policer is not working"));
            break;
            case(e_FM_PCD_PRS_EXCEPTION_DOUBLE_ECC):
            case(e_FM_PCD_PRS_EXCEPTION_SINGLE_ECC):
            case(e_FM_PCD_PRS_EXCEPTION_ILLEGAL_ACCESS):
            case(e_FM_PCD_PRS_EXCEPTION_PORT_ILLEGAL_ACCESS):
                if(!p_FmPcd->p_FmPcdPrs)
                    RETURN_ERROR(MINOR, E_INVALID_STATE, ("Can't ask for this interrupt - parser is not working"));
            break;
            default:
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("Unsupported exception"));

        }

        switch(exception)
        {
            case(e_FM_PCD_KG_ERR_EXCEPTION_DOUBLE_ECC):
                tmpReg = GET_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgeeer);
                if(enable)
                    tmpReg |= FM_PCD_KG_DOUBLE_ECC;
                else
                    tmpReg &= ~FM_PCD_KG_DOUBLE_ECC;
                WRITE_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgeeer, tmpReg);
                break;
            case(e_FM_PCD_KG_ERR_EXCEPTION_KEYSIZE_OVERFLOW):
                tmpReg = GET_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgeeer);
                if(enable)
                    tmpReg |= FM_PCD_KG_KEYSIZE_OVERFLOW;
                else
                    tmpReg &= ~FM_PCD_KG_KEYSIZE_OVERFLOW;
                WRITE_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgeeer, tmpReg);
                break;
            case(e_FM_PCD_PRS_EXCEPTION_DOUBLE_ECC):
                tmpReg = GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->perer);
                if(enable)
                    tmpReg |= FM_PCD_PRS_DOUBLE_ECC;
                else
                    tmpReg &= ~FM_PCD_PRS_DOUBLE_ECC;
                WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->perer, tmpReg);
                break;
            case(e_FM_PCD_PRS_EXCEPTION_ILLEGAL_ACCESS):
                tmpReg = GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->perer);
                if(enable)
                    tmpReg |= FM_PCD_PRS_ILLEGAL_ACCESS;
                else
                    tmpReg &= ~FM_PCD_PRS_ILLEGAL_ACCESS;
                WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->perer, tmpReg);
                break;
            case(e_FM_PCD_PRS_EXCEPTION_PORT_ILLEGAL_ACCESS):
                tmpReg = GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->perer);
                if(enable)
                    tmpReg |= FM_PCD_PRS_PORT_ILLEGAL_ACCESS;
                else
                    tmpReg &= ~FM_PCD_PRS_PORT_ILLEGAL_ACCESS;
                WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->perer, tmpReg);
                break;
            case(e_FM_PCD_PRS_EXCEPTION_SINGLE_ECC):
                tmpReg = GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->pever);
                if(enable)
                    tmpReg |= FM_PCD_PRS_SINGLE_ECC;
                else
                    tmpReg &= ~FM_PCD_PRS_SINGLE_ECC;
                WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->pever, tmpReg);
                break;
            case(e_FM_PCD_PLCR_ERR_EXCEPTION_DOUBLE_ECC):
                tmpReg = GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_eier);
                if(enable)
                    tmpReg |= FM_PCD_PLCR_DOUBLE_ECC;
                else
                    tmpReg &= ~FM_PCD_PLCR_DOUBLE_ECC;
                WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_eier, tmpReg);
                break;
            case(e_FM_PCD_PLCR_ERR_EXCEPTION_INIT_ENTRY_ERROR):
                tmpReg = GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_eier);
                if(enable)
                    tmpReg |= FM_PCD_PLCR_INIT_ENTRY_ERROR;
                else
                    tmpReg &= ~FM_PCD_PLCR_INIT_ENTRY_ERROR;
                WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_eier, tmpReg);
                break;
            case(e_FM_PCD_PLCR_EXCEPTION_PRAM_SELF_INIT_COMPLETE):
                tmpReg = GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_ier);
                if(enable)
                    tmpReg |= FM_PCD_PLCR_PRAM_SELF_INIT_COMPLETE;
                else
                    tmpReg &= ~FM_PCD_PLCR_PRAM_SELF_INIT_COMPLETE;
                WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_ier, tmpReg);
                break;
            case(e_FM_PCD_PLCR_EXCEPTION_ATOMIC_ACTION_COMPLETE):
                tmpReg = GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_ier);
                if(enable)
                    tmpReg |= FM_PCD_PLCR_ATOMIC_ACTION_COMPLETE;
                else
                    tmpReg &= ~FM_PCD_PLCR_ATOMIC_ACTION_COMPLETE;
                WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_ier, tmpReg);
                break;
             default:
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("Unsupported exception"));
        }
        /* for ECC exceptions driver automatically enables ECC mechanism, if disabled.
           Driver does NOT disables them automatically, as we do not control which
           of the rams are enabled and which arn't */
        if(enable && ( (exception == e_FM_PCD_KG_ERR_EXCEPTION_DOUBLE_ECC) |
                       (exception == e_FM_PCD_PLCR_ERR_EXCEPTION_DOUBLE_ECC) |
                       (exception == e_FM_PCD_PRS_EXCEPTION_DOUBLE_ECC) |
                       (exception == e_FM_PCD_PRS_EXCEPTION_SINGLE_ECC)))
            if(!FmRamsEccIsExternalCtl(p_FmPcd->h_Fm))
                FM_EnableRamsEcc(p_FmPcd->h_Fm);
    }
    else
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Undefined exception"));

    return E_OK;
}

uint32_t FM_PCD_GetCounter(t_Handle h_FmPcd, e_FmPcdCounters counter)
{
    t_FmPcd            *p_FmPcd = (t_FmPcd*)h_FmPcd;

    SANITY_CHECK_RETURN_VALUE(h_FmPcd, E_INVALID_HANDLE, 0);
    SANITY_CHECK_RETURN_VALUE(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE, 0);

    switch(counter)
    {
        case(e_FM_PCD_KG_COUNTERS_TOTAL):
            if(!p_FmPcd->p_FmPcdKg)
            {
                REPORT_ERROR(MINOR, E_INVALID_STATE, ("Can't ask for this counters"));
                return 0;
            }
            break;
        case(e_FM_PCD_PLCR_COUNTERS_YELLOW):
        case(e_FM_PCD_PLCR_COUNTERS_RED):
        case(e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_RED):
        case(e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_YELLOW):
        case(e_FM_PCD_PLCR_COUNTERS_TOTAL):
        case(e_FM_PCD_PLCR_COUNTERS_LENGTH_MISMATCH):
            if(!p_FmPcd->p_FmPcdPlcr)
            {
                REPORT_ERROR(MINOR, E_INVALID_STATE, ("Can't ask for this counters"));
                return 0;
            }
            /* check that counters are enabled */
            if(!(GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_gcr) & FM_PCD_PLCR_GCR_STEN))
            {
                REPORT_ERROR(MINOR, E_INVALID_STATE, ("Requested counter was not enabled"));
                return 0;
            }
            break;
        case(e_FM_PCD_PRS_COUNTERS_PARSE_DISPATCH):
        case(e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED):
        case(e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED):
        case(e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED):
        case(e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED):
        case(e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED_WITH_ERR):
        case(e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED_WITH_ERR):
        case(e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED_WITH_ERR):
        case(e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED_WITH_ERR):
        case(e_FM_PCD_PRS_COUNTERS_SOFT_PRS_CYCLES):
        case(e_FM_PCD_PRS_COUNTERS_SOFT_PRS_STALL_CYCLES):
        case(e_FM_PCD_PRS_COUNTERS_HARD_PRS_CYCLE_INCL_STALL_CYCLES):
        case(e_FM_PCD_PRS_COUNTERS_MURAM_READ_CYCLES):
        case(e_FM_PCD_PRS_COUNTERS_MURAM_READ_STALL_CYCLES):
        case(e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_CYCLES):
        case(e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_STALL_CYCLES):
        case(e_FM_PCD_PRS_COUNTERS_FPM_COMMAND_STALL_CYCLES):
            if(!p_FmPcd->p_FmPcdPrs)
            {
                REPORT_ERROR(MINOR, E_INVALID_STATE, ("Can't ask for this counters"));
                return 0;
            }
            break;
        default:
            REPORT_ERROR(MINOR, E_INVALID_STATE, ("Unsupported type of counter"));
            return 0;
    }
    switch(counter)
    {
        case(e_FM_PCD_PRS_COUNTERS_PARSE_DISPATCH):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->pds);
        case(e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->l2rrs);
        case(e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->l3rrs);
        case(e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->l4rrs);
        case(e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->srrs);
        case(e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED_WITH_ERR):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->l2rres);
        case(e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED_WITH_ERR):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->l3rres);
        case(e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED_WITH_ERR):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->l4rres);
        case(e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED_WITH_ERR):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->srres);
        case(e_FM_PCD_PRS_COUNTERS_SOFT_PRS_CYCLES):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->spcs);
        case(e_FM_PCD_PRS_COUNTERS_SOFT_PRS_STALL_CYCLES):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->spscs);
        case(e_FM_PCD_PRS_COUNTERS_HARD_PRS_CYCLE_INCL_STALL_CYCLES):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->hxscs);
        case(e_FM_PCD_PRS_COUNTERS_MURAM_READ_CYCLES):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->mrcs);
        case(e_FM_PCD_PRS_COUNTERS_MURAM_READ_STALL_CYCLES):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->mrscs);
        case(e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_CYCLES):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->mwcs);
        case(e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_STALL_CYCLES):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->mwscs);
        case(e_FM_PCD_PRS_COUNTERS_FPM_COMMAND_STALL_CYCLES):
               return GET_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->fcscs);
        case(e_FM_PCD_KG_COUNTERS_TOTAL):
               return GET_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgtpc);

        /*Policer statictics*/
        case(e_FM_PCD_PLCR_COUNTERS_YELLOW):
                return GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_ypcnt);
        case(e_FM_PCD_PLCR_COUNTERS_RED):
                return GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_rpcnt);
        case(e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_RED):
                return GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_rrpcnt);
        case(e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_YELLOW):
                return GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_rypcnt);
        case(e_FM_PCD_PLCR_COUNTERS_TOTAL):
                return GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_tpcnt);
        case(e_FM_PCD_PLCR_COUNTERS_LENGTH_MISMATCH):
                return GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_flmcnt);

        default:
            REPORT_ERROR(MINOR, E_INVALID_STATE, ("Unsupported type of counter"));
            return 0;
    }
}

t_Error FM_PCD_SetCounter(t_Handle h_FmPcd, e_FmPcdCounters counter, uint32_t value)
{
    t_FmPcd            *p_FmPcd = (t_FmPcd*)h_FmPcd;

    SANITY_CHECK_RETURN_ERROR(h_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE);

    switch(counter)
    {
        case(e_FM_PCD_KG_COUNTERS_TOTAL):
            if(!p_FmPcd->p_FmPcdKg)
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("Can't ask for this counters - keygen is not working"));
            break;
        case(e_FM_PCD_PLCR_COUNTERS_YELLOW):
        case(e_FM_PCD_PLCR_COUNTERS_RED):
        case(e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_RED):
        case(e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_YELLOW):
        case(e_FM_PCD_PLCR_COUNTERS_TOTAL):
        case(e_FM_PCD_PLCR_COUNTERS_LENGTH_MISMATCH):
            if(!p_FmPcd->p_FmPcdPlcr)
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("Can't ask for this counters - Policer is not working"));
            if(!(GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_gcr) & FM_PCD_PLCR_GCR_STEN))
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("Requested counter was not enabled"));
            break;
        case(e_FM_PCD_PRS_COUNTERS_PARSE_DISPATCH):
        case(e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED):
        case(e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED):
        case(e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED):
        case(e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED):
        case(e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED_WITH_ERR):
        case(e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED_WITH_ERR):
        case(e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED_WITH_ERR):
        case(e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED_WITH_ERR):
        case(e_FM_PCD_PRS_COUNTERS_SOFT_PRS_CYCLES):
        case(e_FM_PCD_PRS_COUNTERS_SOFT_PRS_STALL_CYCLES):
        case(e_FM_PCD_PRS_COUNTERS_HARD_PRS_CYCLE_INCL_STALL_CYCLES):
        case(e_FM_PCD_PRS_COUNTERS_MURAM_READ_CYCLES):
        case(e_FM_PCD_PRS_COUNTERS_MURAM_READ_STALL_CYCLES):
        case(e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_CYCLES):
        case(e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_STALL_CYCLES):
        case(e_FM_PCD_PRS_COUNTERS_FPM_COMMAND_STALL_CYCLES):
            if(!p_FmPcd->p_FmPcdPrs)
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("Unsupported type of counter"));
            break;
        default:
            RETURN_ERROR(MINOR, E_INVALID_STATE, ("Unsupported type of counter"));
    }
    switch(counter)
    {
        case(e_FM_PCD_PRS_COUNTERS_PARSE_DISPATCH):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->pds, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->l2rrs, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->l3rrs, value);
             break;
       case(e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->l4rrs, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->srrs, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED_WITH_ERR):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->l2rres, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED_WITH_ERR):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->l3rres, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED_WITH_ERR):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->l4rres, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED_WITH_ERR):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->srres, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_SOFT_PRS_CYCLES):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->spcs, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_SOFT_PRS_STALL_CYCLES):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->spscs, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_HARD_PRS_CYCLE_INCL_STALL_CYCLES):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->hxscs, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_MURAM_READ_CYCLES):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->mrcs, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_MURAM_READ_STALL_CYCLES):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->mrscs, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_CYCLES):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->mwcs, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_STALL_CYCLES):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->mwscs, value);
            break;
        case(e_FM_PCD_PRS_COUNTERS_FPM_COMMAND_STALL_CYCLES):
               WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->fcscs, value);
             break;
        case(e_FM_PCD_KG_COUNTERS_TOTAL):
            WRITE_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgtpc,value);
            break;

        /*Policer counters*/
        case(e_FM_PCD_PLCR_COUNTERS_YELLOW):
            WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_ypcnt, value);
            break;
        case(e_FM_PCD_PLCR_COUNTERS_RED):
            WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_rpcnt, value);
            break;
        case(e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_RED):
             WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_rrpcnt, value);
            break;
        case(e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_YELLOW):
              WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_rypcnt, value);
            break;
        case(e_FM_PCD_PLCR_COUNTERS_TOTAL):
              WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_tpcnt, value);
            break;
        case(e_FM_PCD_PLCR_COUNTERS_LENGTH_MISMATCH):
              WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_flmcnt, value);
            break;
        default:
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Unsupported type of counter"));
    }

return E_OK;
}



t_Error FM_PCD_ForceIntr (t_Handle h_FmPcd, e_FmPcdExceptions exception)
{
    t_FmPcd            *p_FmPcd = (t_FmPcd*)h_FmPcd;

    SANITY_CHECK_RETURN_ERROR(h_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE);

    switch(exception)
    {
        case(e_FM_PCD_KG_ERR_EXCEPTION_DOUBLE_ECC):
        case(e_FM_PCD_KG_ERR_EXCEPTION_KEYSIZE_OVERFLOW):
            if(!p_FmPcd->p_FmPcdKg)
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("Can't ask for this interrupt - keygen is not working"));
            break;
        case(e_FM_PCD_PLCR_ERR_EXCEPTION_DOUBLE_ECC):
        case(e_FM_PCD_PLCR_ERR_EXCEPTION_INIT_ENTRY_ERROR):
        case(e_FM_PCD_PLCR_EXCEPTION_PRAM_SELF_INIT_COMPLETE):
        case(e_FM_PCD_PLCR_EXCEPTION_ATOMIC_ACTION_COMPLETE):
            if(!p_FmPcd->p_FmPcdPlcr)
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("Can't ask for this interrupt - policer is not working"));
            break;
        case(e_FM_PCD_PRS_EXCEPTION_DOUBLE_ECC):
        case(e_FM_PCD_PRS_EXCEPTION_SINGLE_ECC):
        case(e_FM_PCD_PRS_EXCEPTION_ILLEGAL_ACCESS):
        case(e_FM_PCD_PRS_EXCEPTION_PORT_ILLEGAL_ACCESS):
            if(!p_FmPcd->p_FmPcdPrs)
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("Can't ask for this interrupt -parsrer is not working"));
            break;
        default:
            RETURN_ERROR(MINOR, E_INVALID_STATE, ("Invalid interrupt requested"));

    }
    switch(exception)
    {
        case e_FM_PCD_PRS_EXCEPTION_DOUBLE_ECC:
            if (!(p_FmPcd->exceptions & FM_PCD_EX_PRS_DOUBLE_ECC))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->perfr, FM_PCD_PRS_DOUBLE_ECC);
            break;
        case e_FM_PCD_PRS_EXCEPTION_ILLEGAL_ACCESS:
            if (!(p_FmPcd->exceptions & FM_PCD_EX_PRS_ILLEGAL_ACCESS))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->perfr, FM_PCD_PRS_ILLEGAL_ACCESS);
            break;
        case e_FM_PCD_PRS_EXCEPTION_PORT_ILLEGAL_ACCESS:
            if (!(p_FmPcd->exceptions & FM_PCD_EX_PRS_PORT_ILLEGAL_ACCESS))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->perfr, /*FM_PCD_PRS_PORT_ILLEGAL_ACCESS*/0x80000000);
            WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->perfr, /*FM_PCD_PRS_PORT_ILLEGAL_ACCESS*/0x00800000);
            WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->perfr, FM_PCD_PRS_PORT_ILLEGAL_ACCESS);
            break;
        case e_FM_PCD_PRS_EXCEPTION_SINGLE_ECC:
            if (!(p_FmPcd->exceptions & FM_PCD_EX_PRS_SINGLE_ECC))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_FmPcd->p_FmPcdPrs->p_FmPcdPrsRegs->pevfr, FM_PCD_PRS_SINGLE_ECC);
            break;
        case e_FM_PCD_KG_ERR_EXCEPTION_DOUBLE_ECC:
            if (!(p_FmPcd->exceptions & FM_PCD_EX_KG_DOUBLE_ECC))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgfeer, FM_PCD_KG_DOUBLE_ECC);
            break;
        case e_FM_PCD_KG_ERR_EXCEPTION_KEYSIZE_OVERFLOW:
            if (!(p_FmPcd->exceptions & FM_PCD_EX_KG_KEYSIZE_OVERFLOW))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgfeer, FM_PCD_KG_KEYSIZE_OVERFLOW);
            break;
        case e_FM_PCD_PLCR_ERR_EXCEPTION_DOUBLE_ECC:
            if (!(p_FmPcd->exceptions & FM_PCD_EX_PLCR_DOUBLE_ECC))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_eifr, FM_PCD_PLCR_DOUBLE_ECC);
            break;
        case e_FM_PCD_PLCR_ERR_EXCEPTION_INIT_ENTRY_ERROR:
            if (!(p_FmPcd->exceptions & FM_PCD_EX_PLCR_INIT_ENTRY_ERROR))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_eifr, FM_PCD_PLCR_INIT_ENTRY_ERROR);
            break;
        case e_FM_PCD_PLCR_EXCEPTION_PRAM_SELF_INIT_COMPLETE:
            if (!(p_FmPcd->exceptions & FM_PCD_EX_PLCR_PRAM_SELF_INIT_COMPLETE))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_ifr, FM_PCD_PLCR_PRAM_SELF_INIT_COMPLETE);
            break;
        case e_FM_PCD_PLCR_EXCEPTION_ATOMIC_ACTION_COMPLETE:
            if (!(p_FmPcd->exceptions & FM_PCD_EX_PLCR_ATOMIC_ACTION_COMPLETE))
                RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception is masked"));
            WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_ifr, FM_PCD_PLCR_ATOMIC_ACTION_COMPLETE);
            break;
        default:
            RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("The selected exception may not be forced"));
    }

    return E_OK;
}

#ifdef VERIFICATION_SUPPORT
void FM_PCD_BackdoorSet (t_Handle h_FmPcd, e_ModuleId moduleId, uint32_t offset, uint32_t value)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint32_t            base;

    SANITY_CHECK_RETURN(h_FmPcd, E_INVALID_HANDLE);

    switch(moduleId)
    {
        case e_MODULE_ID_FM1_PRS:
        case e_MODULE_ID_FM2_PRS:
            base = FmGetPcdPrsBaseAddr(p_FmPcd);
            break;
        case e_MODULE_ID_FM1_PLCR:
        case e_MODULE_ID_FM2_PLCR:
            base = FmGetPcdPlcrBaseAddr(p_FmPcd);
            break;
        case e_MODULE_ID_FM1_KG:
        case e_MODULE_ID_FM2_KG:
            base = FmGetPcdKgBaseAddr(p_FmPcd);
            break;
        default:
            REPORT_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
            return;

    }
    WRITE_UINT32(*(uint32_t*)(base+offset), value);
}

uint32_t      FM_PCD_BackdoorGet(t_Handle h_FmPcd, e_ModuleId moduleId, uint32_t offset)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint32_t            base;

    SANITY_CHECK_RETURN_ERROR(h_FmPcd, E_INVALID_HANDLE);

    switch(moduleId)
    {
        case e_MODULE_ID_FM1_PRS:
        case e_MODULE_ID_FM2_PRS:
            base = FmGetPcdPrsBaseAddr(p_FmPcd);
            break;
        case e_MODULE_ID_FM1_PLCR:
        case e_MODULE_ID_FM2_PLCR:
            base = FmGetPcdPlcrBaseAddr(p_FmPcd);
            break;
        case e_MODULE_ID_FM1_KG:
        case e_MODULE_ID_FM2_KG:
            base = FmGetPcdKgBaseAddr(p_FmPcd);
            break;
        default:
            REPORT_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
            return 0;
    }

    return GET_UINT32(*(uint32_t*)(base+offset));
}
#endif /*VERIFICATION_SUPPORT*/

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
t_Error FM_PCD_DumpRegs(t_Handle h_FmPcd)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd*)h_FmPcd;

    DECLARE_DUMP;

    SANITY_CHECK_RETURN_ERROR(h_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE);

    if (p_FmPcd->p_FmPcdKg)
        FM_PCD_KgDumpRegs(h_FmPcd);
    if (p_FmPcd->p_FmPcdPlcr)
        FM_PCD_PlcrDumpRegs(h_FmPcd);
    if (p_FmPcd->p_FmPcdPrs)
        FM_PCD_PrsDumpRegs(h_FmPcd);
    return E_OK;
 }
#endif /* (defined(DEBUG_ERRORS) && ... */
#endif /* ! CONFIG_GUEST_PARTITION */

