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
 @File          fm_plcr.c

 @Description   FM PCD POLICER...
*//***************************************************************************/
#include "std_ext.h"
#include "error_ext.h"
#include "string_ext.h"
#include "debug_ext.h"
#include "net_ext.h"
#include "fm_ext.h"
#include "fm_pcd.h"
#include "fm_hc.h"


static bool    FmPcdPlcrIsProfileShared(t_Handle h_FmPcd, uint16_t absoluteProfileId)
{
    t_FmPcd         *p_FmPcd            = (t_FmPcd*)h_FmPcd;
    uint16_t        i;

    SANITY_CHECK_RETURN_VALUE(p_FmPcd, E_INVALID_HANDLE, FALSE);

    for(i=0;i<p_FmPcd->p_FmPcdPlcr->numOfSharedProfiles;i++)
        if(p_FmPcd->p_FmPcdPlcr->sharedProfilesIds[i] == absoluteProfileId)
            return TRUE;
    return FALSE;
}

static t_Error SetProfileNia(t_FmPcd *p_FmPcd, e_FmPcdEngine nextEngine, u_FmPcdPlcrNextEngineParams *p_NextEngineParams, uint32_t *nextAction)
{
    uint32_t    nia;
    uint16_t    absoluteProfileId = (uint16_t)(CAST_POINTER_TO_UINT32(p_NextEngineParams->h_Profile)-1);
    uint8_t     relativeSchemeId, physicatSchemeId;

    nia = FM_PCD_PLCR_NIA_VALID;

    switch (nextEngine)
    {
        case e_FM_PCD_DONE :
            switch (p_NextEngineParams->action)
            {
                case e_FM_PCD_PLCR_DROP_FRAME :
                    nia |= (NIA_ENG_BMI | NIA_BMI_AC_DISCARD);
                    break;
                case e_FM_PCD_PLCR_ENQ_FRAME:
                    nia |= (NIA_ENG_BMI | NIA_BMI_AC_ENQ_FRAME);
                    break;
                default:
                    RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
            }
            break;
        case e_FM_PCD_KG:
            physicatSchemeId = (uint8_t)(CAST_POINTER_TO_UINT32(p_NextEngineParams->h_DirectScheme)-1);
            relativeSchemeId = FmPcdKgGetRelativeSchemeId(p_FmPcd, physicatSchemeId);
            if(relativeSchemeId == FM_PCD_KG_NUM_OF_SCHEMES)
                RETURN_ERROR(MAJOR, E_NOT_IN_RANGE, NO_MSG);
            if (!FmPcdKgIsSchemeValidSw(p_FmPcd, relativeSchemeId))
                 RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Invalid direct scheme."));
            if(!KgIsSchemeAlwaysDirect(p_FmPcd, relativeSchemeId))
                RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Policer Profile may point only to a scheme that is always direct."));
            nia |= NIA_ENG_KG | NIA_KG_DIRECT | physicatSchemeId;
            break;
        case e_FM_PCD_PLCR:
             if(!FmPcdPlcrIsProfileShared(p_FmPcd, absoluteProfileId))
               RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Next profile must be a shared profile"));
             if(!FmPcdPlcrIsProfileValid(p_FmPcd, absoluteProfileId))
               RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Invalid profile "));
            nia |= NIA_ENG_PLCR | NIA_PLCR_ABSOLUTE | absoluteProfileId;
            break;
        default:
            RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
    }

    *nextAction =  nia;

    return E_OK;
}

static uint32_t FPP_Function(uint32_t fpp)
{
    if(fpp > 15)
        return 15 - (0x1f - fpp);
    else
        return 16 + fpp;
}

static uint64_t Rate2Sample(e_FmPcdPlcrRateMode rateMode, uint32_t rate, uint64_t timeStampPeriod, uint32_t count)
{
uint64_t temp;
uint32_t tmp;

    tmp = FPP_Function(count);
    if (rateMode == e_FM_PCD_PLCR_BYTE_MODE)
    {
        temp = ((uint64_t)rate) << (16+tmp);    /* Move it left 16 Bit to the fix point position
                                             + 16 Bit to set the time stamp period */
        temp = temp / 1000000000;         /* Change it from KBit/sec into KBit/(Nano Sec) */
        temp = (temp * 1000) / 8;         /* Change it from KBit/(Nano Sec) into Byte/(Nano Sec) */
        temp =  temp / (timeStampPeriod); /* Change it from Byte/(Nano Sec) into Byte/TimeStamp Units*/
    }
    else
    {
        temp = ((uint64_t)rate) << (16+tmp);  /* Move it left 16 bit to the fix point position
                                           + 16 Bit to set the time stamp period */
        temp = temp / 1000000000;       /* Change it from Packet/sec into Packet/(Nano Sec) */
        temp =  temp / (timeStampPeriod); /* Change it from Packet/Nano into Packet/TimeStamp */
    }

    return temp;

}


static void CheckValidRateRange(t_FmPcd *p_FmPcd)
{
    uint64_t    timeStampPeriod;
//    uint64_t    maxVal = 0xffffffff0000LL; /* [bytes per timeStamp unit] max value which will be legal  for fpp adjustement calculation - for not being "too big"*/
    uint32_t    minVal = 0x00010000;     /*  [bytes per timeStamp unit] min value which will be legal  for fpp adjustement calculation - for not being "too small"*/
//    uint64_t    tempMaxLimit;
    uint64_t    tempMinLimit;

    timeStampPeriod = (uint64_t)FmGetTimeStampPeriod(p_FmPcd->h_Fm);               /* TimeStamp per nano seconds units */

    /*With current timestamp configuration there can not be a MaxLimit,
      which means that any rate above MinLimit can be served by Policer*/
    /*transform  [bytes per timeStamp unit] into  [Kbit/s]*/
    //tempMaxLimit = maxVal * timeStampPeriod;   /*[Byte/TimeStamp] -> [Byte/Nano]*/
    //tempMaxLimit = tempMaxLimit * 8 / 1000;    /*[Byte/NanoSec] -> [KBits/NanoSec]*/
    //tempMaxLimit = tempMaxLimit * 1000000000;  /*[KBits/NanoSec] ->[KBits/Sec]*/
    //tempMaxLimit = tempMaxLimit >> 32;         /*16 (Cir/pir register presicion) + 16 (number of shifts done to fpp by default)*/

    /*transform  [bytes per timeStamp unit] into  [Kbit/s]*/
    tempMinLimit = minVal * timeStampPeriod;   /*[Byte/TimeStamp] -> [Byte/Nano]*/
    tempMinLimit = tempMinLimit * 8 / 1000;    /*[Byte/NanoSec] -> [KBits/NanoSec]*/
    tempMinLimit = tempMinLimit * 1000000000;  /*[KBits/NanoSec] ->[KBits/Sec]*/
    tempMinLimit = tempMinLimit >> 32;         /*16 (Cir/pir register presicion) + 16 (number of shifts done to fpp by default)*/

    XX_Print("Valid range for ByteMode RateSelection is min 0x%x ", (uint32_t)tempMinLimit);

    //tempMaxLimit = maxVal * timeStampPeriod;   /*[Packets/TimeStamp] -> [Packets/Nano]*/
    //tempMaxLimit = maxVal * 1000000000;        /*[Packets/Nano] -> [Packets/Sec]*/
    //tempMaxLimit = tempMaxLimit >> 32;         /*16 (Cir/pir register presicion) + 0 (number of shifts done to fpp)*/

    tempMinLimit = minVal * timeStampPeriod;   /*[Packets/TimeStamp] -> [Packets/Nano]*/
    tempMinLimit = minVal * 1000000000;        /*[Packets/Nano] -> [Packets/Sec]*/
    tempMinLimit = tempMinLimit >> 32;         /*16 (Cir/pir register presicion) + 16 (number of shifts done to fpp by default)*/

    XX_Print("Valid range for PacketMode RateSelection is min 0x%x ", (uint32_t)tempMinLimit);
}

/* .......... */

static void calcRates(t_Handle h_FmPcd, t_FmPcdPlcrNonPassthroughAlgParams *p_NonPassthroughAlgParam,
                        uint32_t *cir, uint32_t *cbs, uint32_t *pir_eir, uint32_t *pbs_ebs, uint32_t *fpp)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint64_t    timeStampPeriod;
    uint64_t    tempCir, tempPir_Eir;
    uint32_t    temp, count;
    bool        big;

    timeStampPeriod = (uint64_t)FmGetTimeStampPeriod(p_FmPcd->h_Fm);               /* TimeStamp per nano seconds units */

    /* First round to calculate precision */
    if (p_NonPassthroughAlgParam->comittedInfoRate > p_NonPassthroughAlgParam->peakOrAccessiveInfoRate)
        tempCir = Rate2Sample(p_NonPassthroughAlgParam->rateMode, p_NonPassthroughAlgParam->comittedInfoRate, timeStampPeriod, 0);
    else
        tempCir = Rate2Sample(p_NonPassthroughAlgParam->rateMode, p_NonPassthroughAlgParam->peakOrAccessiveInfoRate, timeStampPeriod, 0);

    /* Base on result calculate the FPP and re calculate cir, pir_eir */
    count = 0;
    if ((tempCir > 0xFFFFFFFF))
    {
        /* Overflow need to shrink number */
        big = TRUE;
        temp = (uint32_t)(tempCir >> 32);
        while (temp > 0)
        {
            temp = temp >> 1;
            count++;
        }
        if(count > 16)
        {
            REPORT_ERROR(MAJOR, E_INVALID_SELECTION, ("timeStampPeriod to Information rate ratio is too big"));
            CheckValidRateRange(p_FmPcd);
            return;
        }
    }
    else
    {
        /* Underflow need to improve accuracy */
        big = FALSE;
        temp = (uint32_t)(tempCir & 0x00000000FFFFFFFF);
        if(temp != 0)
        {
        while ((temp & 0x80000000) == 0)
        {
            temp = temp << 1;
            count++;
        }
        if(count > 15)
        {
            REPORT_ERROR(MAJOR, E_INVALID_SELECTION, ("timeStampPeriod to Information rate ratio is too small"));
                CheckValidRateRange(p_FmPcd);
            return;
            }
        }
    }

    /* Second round based on precision do the roght calculation */
    if (count > 0)
    {
        if (big)
        {
           *fpp = (uint32_t)(0x1F - (count - 1));
 //           tempCir     = Rate2Sample(p_NonPassthroughAlgParam->rateMode, p_NonPassthroughAlgParam->comittedInfoRate, timeStampPeriod, *fpp);
 //           tempPir_Eir = Rate2Sample(p_NonPassthroughAlgParam->rateMode, p_NonPassthroughAlgParam->peakOrAccessiveInfoRate , timeStampPeriod, *fpp);
           // *fpp = (uint32_t)(0x1F - ((1 << (count - 1)) + 1));
        }
        else
        {
            *fpp = (uint32_t)count;
 //           tempCir     = Rate2Sample(p_NonPassthroughAlgParam->rateMode, p_NonPassthroughAlgParam->comittedInfoRate , timeStampPeriod, *fpp);
 //           tempPir_Eir = Rate2Sample(p_NonPassthroughAlgParam->rateMode, p_NonPassthroughAlgParam->peakOrAccessiveInfoRate , timeStampPeriod, *fpp);
          //  *fpp = (uint32_t)(1 << (count - 1));
        }
    }
    else
    {
            *fpp = 0;
 //           tempCir     = Rate2Sample(p_NonPassthroughAlgParam->rateMode, p_NonPassthroughAlgParam->comittedInfoRate, timeStampPeriod, *fpp);
 //           tempPir_Eir = Rate2Sample(p_NonPassthroughAlgParam->rateMode, p_NonPassthroughAlgParam->peakOrAccessiveInfoRate, timeStampPeriod, *fpp);
    }

    tempCir     = Rate2Sample(p_NonPassthroughAlgParam->rateMode, p_NonPassthroughAlgParam->comittedInfoRate, timeStampPeriod, *fpp);
    tempPir_Eir = Rate2Sample(p_NonPassthroughAlgParam->rateMode, p_NonPassthroughAlgParam->peakOrAccessiveInfoRate , timeStampPeriod, *fpp);

    if (p_NonPassthroughAlgParam->rateMode == e_FM_PCD_PLCR_BYTE_MODE)
    {
        *cbs     = (1000 * p_NonPassthroughAlgParam->comittedBurstSize / 8);
        *pbs_ebs = (1000 * p_NonPassthroughAlgParam->peakOrAccessiveBurstSize) / 8; /* 8=Bits->Bytes 1000=KB->B */
    }
    else
    {
        *cbs     =  p_NonPassthroughAlgParam->comittedBurstSize;
        *pbs_ebs =  p_NonPassthroughAlgParam->peakOrAccessiveBurstSize;
    }

    *cir     = (uint32_t)tempCir;
    *pir_eir = (uint32_t)tempPir_Eir;
}

#ifndef CONFIG_GUEST_PARTITION
static void WritePar(t_FmPcd *p_FmPcd, uint32_t par)
{
    t_FmPcdPlcrRegs *p_FmPcdPlcrRegs    = p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs;

    WRITE_UINT32(p_FmPcdPlcrRegs->fmpl_par, par);

    while(GET_UINT32(p_FmPcdPlcrRegs->fmpl_par) & FM_PCD_PLCR_PAR_GO) ;

}
#endif /* CONFIG_GUEST_PARTITION */

/*********************************************/
/*............Policer Exception..............*/
/*********************************************/
#ifndef CONFIG_GUEST_PARTITION
static void PcdPlcrException(t_Handle h_FmPcd)
{
    t_FmPcd *p_FmPcd = (t_FmPcd *)h_FmPcd;
    uint32_t event;

    event = GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_evr);
    event &= GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_ier);

    WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_evr, event);

    if(event & FM_PCD_PLCR_PRAM_SELF_INIT_COMPLETE)
        p_FmPcd->f_FmPcdException(p_FmPcd->h_App,e_FM_PCD_PLCR_EXCEPTION_PRAM_SELF_INIT_COMPLETE);
    if(event & FM_PCD_PLCR_ATOMIC_ACTION_COMPLETE)
        p_FmPcd->f_FmPcdException(p_FmPcd->h_App,e_FM_PCD_PLCR_EXCEPTION_ATOMIC_ACTION_COMPLETE);

}

/* ..... */

static void PcdPlcrErrorException(t_Handle h_FmPcd)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd *)h_FmPcd;
    uint32_t            event, captureReg;

    event = GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_eevr);
    event &= GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_eier);

    WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_eevr, event);

    if(event & FM_PCD_PLCR_DOUBLE_ECC)
        p_FmPcd->f_FmPcdException(p_FmPcd->h_App,e_FM_PCD_PLCR_ERR_EXCEPTION_DOUBLE_ECC);
    if(event & FM_PCD_PLCR_INIT_ENTRY_ERROR)
    {
        captureReg = GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_upcr);
        /*ASSERT_COND(captureReg & PLCR_ERR_UNINIT_CAP);
        p_UnInitCapt->profileNum = (uint8_t)(captureReg & PLCR_ERR_UNINIT_NUM_MASK);
        p_UnInitCapt->portId = (uint8_t)((captureReg & PLCR_ERR_UNINIT_PID_MASK) >>PLCR_ERR_UNINIT_PID_SHIFT) ;
        p_UnInitCapt->absolute = (bool)(captureReg & PLCR_ERR_UNINIT_ABSOLUTE_MASK);*/
        p_FmPcd->f_FmPcdIndexedException(p_FmPcd->h_App,e_FM_PCD_PLCR_ERR_EXCEPTION_INIT_ENTRY_ERROR,(uint16_t)(captureReg & PLCR_ERR_UNINIT_NUM_MASK));
        //WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_upcr, PLCR_ERR_UNINIT_CAP);
    }
}

#endif /* !CONFIG_GUEST_PARTITION */

t_Error  FmPcdPlcrAllocProfiles(t_Handle h_FmPcd, uint8_t hardwarePortId, uint16_t numOfProfiles)
{
    t_FmPcd                     *p_FmPcd = (t_FmPcd*)h_FmPcd;
#ifdef CONFIG_GUEST_PARTITION
    t_FmPcdIpcPlcrAllocParams   ipcPlcrParams;
#endif /* CONFIG_GUEST_PARTITION */
    t_Error                     err;
    uint16_t                    base;
    uint16_t                    pcdPortId;
    uint8_t                     portsTable[]        = PCD_PORTS_TABLE;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);

#ifdef CONFIG_GUEST_PARTITION
    /* Alloc resources using IPC messaging */
    ipcPlcrParams.num = numOfProfiles;
    ipcPlcrParams.hardwarePortId = hardwarePortId;
    err = XX_SendMessage(p_FmPcd->fmPcdModuleName, FM_PCD_ALLOC_PROFILES, (uint8_t*)&ipcPlcrParams, NULL, NULL);
    if(err)
        RETURN_ERROR(MAJOR, err,NO_MSG);
    base = ipcPlcrParams.plcrProfilesBase;
#else /* master */
    err = PlcrAllocProfiles(p_FmPcd, hardwarePortId, numOfProfiles, &base);
    if(err)
        RETURN_ERROR(MAJOR, err,NO_MSG);
#endif /* CONFIG_GUEST_PARTITION */

    GET_FM_PCD_PORTID_FROM_GLOBAL_PORTID(pcdPortId, portsTable, hardwarePortId)

    p_FmPcd->p_FmPcdPlcr->portsMapping[pcdPortId].numOfProfiles = numOfProfiles;
    p_FmPcd->p_FmPcdPlcr->portsMapping[pcdPortId].profilesBase = base;

    return E_OK;

}

t_Error  FmPcdPlcrFreeProfiles(t_Handle h_FmPcd, uint8_t hardwarePortId)
{
    t_FmPcd                     *p_FmPcd = (t_FmPcd*)h_FmPcd;
#ifdef CONFIG_GUEST_PARTITION
    t_FmPcdIpcPlcrAllocParams   ipcPlcrParams;
#endif /* CONFIG_GUEST_PARTITION */
    t_Error                     err;
    uint16_t                    pcdPortId;
    uint8_t                     portsTable[]        = PCD_PORTS_TABLE;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);

    GET_FM_PCD_PORTID_FROM_GLOBAL_PORTID(pcdPortId, portsTable, hardwarePortId)


#ifdef CONFIG_GUEST_PARTITION
    /* Alloc resources using IPC messaging */
    ipcPlcrParams.num = p_FmPcd->p_FmPcdPlcr->portsMapping[pcdPortId].numOfProfiles;
    ipcPlcrParams.hardwarePortId = hardwarePortId;
    ipcPlcrParams.plcrProfilesBase = p_FmPcd->p_FmPcdPlcr->portsMapping[pcdPortId].profilesBase;
    err = XX_SendMessage(p_FmPcd->fmPcdModuleName, FM_PCD_FREE_PROFILES, (uint8_t*)&ipcPlcrParams, NULL, NULL);
    if(err)
        RETURN_ERROR(MAJOR, err,NO_MSG);
#else /* master */
    err = PlcrFreeProfiles(p_FmPcd, hardwarePortId, p_FmPcd->p_FmPcdPlcr->portsMapping[pcdPortId].numOfProfiles, p_FmPcd->p_FmPcdPlcr->portsMapping[pcdPortId].profilesBase);
    if(err)
        RETURN_ERROR(MAJOR, err,NO_MSG);
#endif /* CONFIG_GUEST_PARTITION */

    p_FmPcd->p_FmPcdPlcr->portsMapping[pcdPortId].numOfProfiles = 0;
    p_FmPcd->p_FmPcdPlcr->portsMapping[pcdPortId].profilesBase = 0;

    return E_OK;

}

bool    FmPcdPlcrIsProfileValid(t_Handle h_FmPcd, uint16_t absoluteProfileId)
{
    t_FmPcd         *p_FmPcd            = (t_FmPcd*)h_FmPcd;
    t_FmPcdPlcr     *p_FmPcdPlcr        = p_FmPcd->p_FmPcdPlcr;

    return p_FmPcdPlcr->profiles[absoluteProfileId].valid;
}


#ifndef CONFIG_GUEST_PARTITION
t_Error  PlcrAllocProfiles(t_FmPcd *p_FmPcd, uint8_t hardwarePortId, uint16_t numOfProfiles, uint16_t *p_Base)
{
    t_FmPcdPlcrRegs *p_Regs = p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs;
    uint32_t        profilesFound, log2Num, tmpReg32;
    uint16_t        first, i;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);

    ASSERT_COND(hardwarePortId);

    if (numOfProfiles>FM_PCD_PLCR_NUM_ENTRIES)
        RETURN_ERROR(MINOR, E_INVALID_VALUE, ("numProfiles is too big."));

    if (!POWER_OF_2(numOfProfiles))
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("numProfiles must be a power of 2."));

    TRY_LOCK_RET_ERR(p_FmPcd->lock);

    if(GET_UINT32(p_Regs->fmpl_pmr[hardwarePortId-1]) & FM_PCD_PLCR_PMR_V)
    {
        RELEASE_LOCK(p_FmPcd->lock);
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("The requesting port has already an allocated profiles window."));
    }

    first = 0;
    profilesFound = 0;
    for(i=0;i<FM_PCD_PLCR_NUM_ENTRIES;)
    {
        if(!p_FmPcd->p_FmPcdPlcr->profiles[i].profilesMng.allocated)
        {
            profilesFound++;
            i++;
            if(profilesFound == numOfProfiles)
                break;
        }
        else
        {
            profilesFound = 0;
            /* advance i to the next aligned address */
            first = i = (uint8_t)(first + numOfProfiles);
        }
    }
    if(profilesFound == numOfProfiles)
    {
        for(i = first; i<first + numOfProfiles; i++)
        {
            p_FmPcd->p_FmPcdPlcr->profiles[i].profilesMng.allocated = TRUE;
            p_FmPcd->p_FmPcdPlcr->profiles[i].profilesMng.ownerId = hardwarePortId;
        }
    }
    else
    {
        RELEASE_LOCK(p_FmPcd->lock);
        RETURN_ERROR(MINOR, E_FULL, ("No profiles."));
    }


    /**********************FMPL_PMRx******************/
    LOG2((uint64_t)numOfProfiles, log2Num);
    tmpReg32 = first;
    tmpReg32 |= log2Num << 16;
    tmpReg32 |= FM_PCD_PLCR_PMR_V;
    WRITE_UINT32(p_Regs->fmpl_pmr[hardwarePortId-1], tmpReg32);

    *p_Base = first;

    RELEASE_LOCK(p_FmPcd->lock);

    return E_OK;

}

t_Error  PlcrAllocSharedProfiles(t_FmPcd *p_FmPcd, uint16_t numOfProfiles, uint16_t *profilesIds)
{
    uint32_t        profilesFound;
    uint16_t        i, k=0;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);

    if (numOfProfiles>FM_PCD_PLCR_NUM_ENTRIES)
        RETURN_ERROR(MINOR, E_INVALID_VALUE, ("numProfiles is too big."));

    profilesFound = 0;
    for(i=0;i<FM_PCD_PLCR_NUM_ENTRIES; i++)
    {
        if(!p_FmPcd->p_FmPcdPlcr->profiles[i].profilesMng.allocated)
        {
            profilesFound++;
            profilesIds[k] = i;
            k++;
            if(profilesFound == numOfProfiles)
                break;
        }
    }
    if(profilesFound != numOfProfiles)
        RETURN_ERROR(MAJOR, E_INVALID_STATE,NO_MSG);
    for(i = 0;i<k;i++)
    {
        p_FmPcd->p_FmPcdPlcr->profiles[profilesIds[i]].profilesMng.allocated = TRUE;
        p_FmPcd->p_FmPcdPlcr->profiles[profilesIds[i]].profilesMng.ownerId = 0;
    }

    return E_OK;

}

t_Error  PlcrFreeProfiles(t_FmPcd *p_FmPcd, uint8_t hardwarePortId, uint16_t numOfProfiles, uint16_t base)
{
    t_FmPcdPlcrRegs *p_Regs = p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs;
    uint16_t        i;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_HANDLE);

#if 0 //- default
        else
        {
            tmpReg32 = GET_UINT32(p_Regs->fmpl_dpmr);
            log2Num = (uint8_t)(tmpReg32 >> 16);
            first = (uint16_t)tmpReg32;
            numProfiles = (uint16_t)(1<<log2Num);
            ASSERT_COND(p_FmPcd->p_FmPcdPlcr->profiles[first].ownerId == hardwarePortId);
            /* 1 profile is still allocated for default window - HW limitaion. */
            tmpReg32 = first;
            WRITE_UINT32(p_Regs->fmpl_dpmr, tmpReg32);
            first++;
            numProfiles--;
        }
#endif

    WRITE_UINT32(p_Regs->fmpl_pmr[hardwarePortId-1], 0);

    for(i = base; i<base+numOfProfiles;i++)
    {
        ASSERT_COND(p_FmPcd->p_FmPcdPlcr->profiles[i].profilesMng.ownerId == hardwarePortId);
        ASSERT_COND(p_FmPcd->p_FmPcdPlcr->profiles[i].profilesMng.allocated);

        p_FmPcd->p_FmPcdPlcr->profiles[i].profilesMng.allocated = FALSE;
        p_FmPcd->p_FmPcdPlcr->profiles[i].profilesMng.ownerId = 0;
    }
    return E_OK;

}

void  PlcrFreeSharedProfiles(t_FmPcd *p_FmPcd, uint16_t numOfProfiles, uint16_t *profilesIds)
{
    uint16_t        i;

    SANITY_CHECK_RETURN(p_FmPcd, E_INVALID_HANDLE);

    for(i=0;i<numOfProfiles; i++)
    {
        ASSERT_COND(p_FmPcd->p_FmPcdPlcr->profiles[profilesIds[i]].profilesMng.allocated);
        p_FmPcd->p_FmPcdPlcr->profiles[profilesIds[i]].profilesMng.allocated = FALSE;
    }
}

#endif /* ! CONFIG_GUEST_PARTITION */

t_Error FmPcdPlcrBuildProfile(t_Handle h_FmPcd, t_FmPcdPlcrProfileParams *p_Profile, t_FmPcdPlcrInterModuleProfileRegs *p_PlcrRegs)
{

    t_FmPcd         *p_FmPcd            = (t_FmPcd*)h_FmPcd;
    t_Error err;
    uint32_t        pemode, gnia, ynia, rnia;

/* Set G, Y, R Nia */
    err = SetProfileNia(p_FmPcd, p_Profile->nextEngineOnGreen,  &(p_Profile->paramsOnGreen), &gnia);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);
    err = SetProfileNia(p_FmPcd, p_Profile->nextEngineOnYellow, &(p_Profile->paramsOnYellow), &ynia);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);
    err = SetProfileNia(p_FmPcd, p_Profile->nextEngineOnRed,    &(p_Profile->paramsOnRed), &rnia);
   if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);


/* Mode fmpl_pemode */
    pemode = FM_PCD_PLCR_PEMODE_PI;

    switch (p_Profile->algSelection)
    {
        case    e_FM_PCD_PLCR_PASS_THROUGH:
            p_PlcrRegs->fmpl_pecir         = 0;
            p_PlcrRegs->fmpl_pecbs         = 0;
            p_PlcrRegs->fmpl_pepepir_eir   = 0;
            p_PlcrRegs->fmpl_pepbs_ebs     = 0;
            p_PlcrRegs->fmpl_pelts         = 0;
            p_PlcrRegs->fmpl_pects         = 0;
            p_PlcrRegs->fmpl_pepts_ets     = 0;
            pemode &= ~FM_PCD_PLCR_PEMODE_ALG_MASK;
            switch (p_Profile->colorMode)
            {
                case    e_FM_PCD_PLCR_COLOR_BLIND:
                    pemode |= FM_PCD_PLCR_PEMODE_CBLND;
                    switch (p_Profile->color.dfltColor)
                    {
                        case e_FM_PCD_PLCR_GREEN:
                            pemode &= ~FM_PCD_PLCR_PEMODE_DEFC_MASK;
                            break;
                        case e_FM_PCD_PLCR_YELLOW:
                            pemode |= FM_PCD_PLCR_PEMODE_DEFC_Y;
                            break;
                        case e_FM_PCD_PLCR_RED:
                            pemode |= FM_PCD_PLCR_PEMODE_DEFC_R;
                            break;
                        case e_FM_PCD_PLCR_OVERRIDE:
                            pemode |= FM_PCD_PLCR_PEMODE_DEFC_OVERRIDE;
                            break;
                        default:
                            RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
                    }

                    break;
                case    e_FM_PCD_PLCR_COLOR_AWARE:
                    pemode &= ~FM_PCD_PLCR_PEMODE_CBLND;
                    break;
                default:
                    RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
            }
            break;

        case    e_FM_PCD_PLCR_RFC_2698:
            /* Select algorithm MODE[ALG] = ‘01’ */
            pemode |= FM_PCD_PLCR_PEMODE_ALG_RFC2698;
            goto cont_rfc;
        case    e_FM_PCD_PLCR_RFC_4115:
            /* Select algorithm MODE[ALG] = ‘10’ */
            pemode |= FM_PCD_PLCR_PEMODE_ALG_RFC4115;
cont_rfc:
            /* Select Color-Blind / Color-Aware operation (MODE[CBLND]) */
            switch (p_Profile->colorMode)
            {
                case    e_FM_PCD_PLCR_COLOR_BLIND:
                    pemode |= FM_PCD_PLCR_PEMODE_CBLND;
                    break;
                case    e_FM_PCD_PLCR_COLOR_AWARE:
                    pemode &= ~FM_PCD_PLCR_PEMODE_CBLND;
                    /*In color aware more select override color interpretation (MODE[OVCLR]) */
                    switch (p_Profile->color.override)
                    {
                        case e_FM_PCD_PLCR_GREEN:
                            pemode &= ~FM_PCD_PLCR_PEMODE_OVCLR_MASK;
                            break;
                        case e_FM_PCD_PLCR_YELLOW:
                            pemode |= FM_PCD_PLCR_PEMODE_OVCLR_Y;
                            break;
                        case e_FM_PCD_PLCR_RED:
                            pemode |= FM_PCD_PLCR_PEMODE_OVCLR_R;
                            break;
                        case e_FM_PCD_PLCR_OVERRIDE:
                            pemode |= FM_PCD_PLCR_PEMODE_OVCLR_G_NC;
                            break;
                        default:
                            RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
                    }
                    break;
                default:
                    RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
            }
            /* Select Measurement Unit Mode to BYTE or PACKET (MODE[PKT]) */
            switch (p_Profile->nonPassthroughAlgParams.rateMode)
            {
                case e_FM_PCD_PLCR_BYTE_MODE :
                    pemode &= ~FM_PCD_PLCR_PEMODE_PKT;
                        switch (p_Profile->nonPassthroughAlgParams.byteModeParams.frameLengthSelection)
                        {
                            case e_FM_PCD_PLCR_L2_FRM_LEN:
                                pemode |= FM_PCD_PLCR_PEMODE_FLS_L2;
                                break;
                            case e_FM_PCD_PLCR_L3_FRM_LEN:
                                pemode |= FM_PCD_PLCR_PEMODE_FLS_L3;
                                break;
                            case e_FM_PCD_PLCR_L4_FRM_LEN:
                                pemode |= FM_PCD_PLCR_PEMODE_FLS_L4;
                                break;
                            case e_FM_PCD_PLCR_FULL_FRM_LEN:
                                pemode |= FM_PCD_PLCR_PEMODE_FLS_FULL;
                                break;
                            default:
                                RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
                        }
                        switch (p_Profile->nonPassthroughAlgParams.byteModeParams.rollBackFrameSelection)
                        {
                            case e_FM_PCD_PLCR_ROLLBACK_L2_FRM_LEN:
                                pemode &= ~FM_PCD_PLCR_PEMODE_RBFLS;
                                break;
                            case e_FM_PCD_PLCR_ROLLBACK_FULL_FRM_LEN:
                                pemode |= FM_PCD_PLCR_PEMODE_RBFLS;
                                break;
                            default:
                                RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
                        }
                    break;
                case e_FM_PCD_PLCR_PACKET_MODE :
                    pemode |= FM_PCD_PLCR_PEMODE_PKT;
                    break;
                default:
                    RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
            }
            /* Select timeStamp floating point position (MODE[FPP]) to fit the actual traffic rates. For PACKET
               mode with low traffic rates move the fixed point to the left to increase fraction accuracy. For BYTE
               mode with high traffic rates move the fixed point to the right to increase integer accuracy. */

            /* Configure Traffic Parameters*/
            {
                uint32_t cir=0, cbs=0, pir_eir=0, pbs_ebs=0, fpp=0;

                calcRates(h_FmPcd, &p_Profile->nonPassthroughAlgParams, &cir, &cbs, &pir_eir, &pbs_ebs, &fpp);

                /*  Set Committed Information Rate (CIR) */
                p_PlcrRegs->fmpl_pecir = cir;
                /*  Set Committed Burst Size (CBS). */
                p_PlcrRegs->fmpl_pecbs =  cbs;
                /*  Set Peak Information Rate (PIR_EIR used as PIR) */
                p_PlcrRegs->fmpl_pepepir_eir = pir_eir;
                /*   Set Peak Burst Size (PBS_EBS used as PBS) */
                p_PlcrRegs->fmpl_pepbs_ebs = pbs_ebs;

                /* Initialize the Metering Buckets to be full (write them with 0xFFFFFFFF. */
                /* Peak Rate Token Bucket Size (PTS_ETS used as PTS) */
                p_PlcrRegs->fmpl_pepts_ets = 0xFFFFFFFF;
                /* Committed Rate Token Bucket Size (CTS) */
                p_PlcrRegs->fmpl_pects = 0xFFFFFFFF;

                /* Set the FPP based on calculation */
                pemode |= (fpp << FM_PCD_PLCR_PEMODE_FPP_SHIFT);
            }
            break;  /* FM_PCD_PLCR_PEMODE_ALG_RFC2698 , FM_PCD_PLCR_PEMODE_ALG_RFC4115 */
        default:
            RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
    }

    p_PlcrRegs->fmpl_pemode = pemode;


    p_PlcrRegs->fmpl_pegnia = gnia;
    p_PlcrRegs->fmpl_peynia = ynia;
    p_PlcrRegs->fmpl_pernia = rnia;


/* Zero Counters */
    p_PlcrRegs->fmpl_pegpc     = 0;
    p_PlcrRegs->fmpl_peypc     = 0;
    p_PlcrRegs->fmpl_perpc     = 0;
    p_PlcrRegs->fmpl_perypc    = 0;
    p_PlcrRegs->fmpl_perrpc    = 0;

    return E_OK;
}

void  FmPcdPlcrValidateProfileSw(t_Handle h_FmPcd, uint16_t absoluteProfileId)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;

    ASSERT_COND(!p_FmPcd->p_FmPcdPlcr->profiles[absoluteProfileId].valid);
    p_FmPcd->p_FmPcdPlcr->profiles[absoluteProfileId].valid = TRUE;
}

void  FmPcdPlcrInvalidateProfileSw(t_Handle h_FmPcd, uint16_t absoluteProfileId)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;

    ASSERT_COND(p_FmPcd->p_FmPcdPlcr->profiles[absoluteProfileId].valid);
    p_FmPcd->p_FmPcdPlcr->profiles[absoluteProfileId].valid = FALSE;
}

t_Handle PlcrConfig(t_FmPcd *p_FmPcd, t_FmPcdParams *p_FmPcdParams)
{
    t_FmPcdPlcr *p_FmPcdPlcr;
    /*uint8_t i=0;*/

    UNUSED(p_FmPcd);
    UNUSED(p_FmPcdParams);

    p_FmPcdPlcr = (t_FmPcdPlcr *) XX_Malloc(sizeof(t_FmPcdPlcr));
    if (!p_FmPcdPlcr)
    {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FM Policer structure allocation FAILED"));
        return NULL;
    }
    memset(p_FmPcdPlcr, 0, sizeof(t_FmPcdPlcr));

#ifndef CONFIG_GUEST_PARTITION
    p_FmPcdPlcr->p_FmPcdPlcrRegs  = CAST_UINT64_TO_POINTER_TYPE(t_FmPcdPlcrRegs , (FmGetPcdPlcrBaseAddr(p_FmPcdParams->h_Fm)));
    p_FmPcd->p_FmPcdDriverParam->plcrAutoRefresh    = DEFAULT_plcrAutoRefresh;
    p_FmPcd->exceptions |= (DEFAULT_fmPcdPlcrExceptions | DEFAULT_fmPcdPlcrErrorExceptions);
#endif /* !CONFIG_GUEST_PARTITION */

    p_FmPcdPlcr->numOfSharedProfiles = DEFAULT_numOfSharedPlcrProfiles;

    return p_FmPcdPlcr;
}

t_Error PlcrInit(t_FmPcd *p_FmPcd)
{
    t_FmPcdDriverParam          *p_Param = p_FmPcd->p_FmPcdDriverParam;
    t_FmPcdPlcr                 *p_FmPcdPlcr = p_FmPcd->p_FmPcdPlcr;
    uint32_t                    tmpReg32 = 0;
    t_Error                     err;
#ifndef CONFIG_GUEST_PARTITION
    t_FmPcdPlcrRegs             *p_Regs = p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs;
#else
    t_FmPcdIpcSharedPlcrAllocParams    ipcSharedPlcrParams;
#endif /* !CONFIG_GUEST_PARTITION */

#ifdef CONFIG_GUEST_PARTITION
    /* Alloc resources using IPC messaging */
    ipcSharedPlcrParams.num = p_FmPcdPlcr->numOfSharedProfiles;
    err = XX_SendMessage(p_FmPcd->fmPcdModuleName, FM_PCD_ALLOC_SHARED_PROFILES, (uint8_t*)&ipcSharedPlcrParams, NULL, NULL);
    if(err)
        RETURN_ERROR(MAJOR, err,NO_MSG);
    memcpy(p_FmPcd->p_FmPcdPlcr->sharedProfilesIds,ipcSharedPlcrParams.profilesIds, sizeof(uint16_t)*ipcSharedPlcrParams.num);
#else /* master */
    err = PlcrAllocSharedProfiles(p_FmPcd, p_FmPcdPlcr->numOfSharedProfiles, p_FmPcd->p_FmPcdPlcr->sharedProfilesIds);
    if(err)
        RETURN_ERROR(MAJOR, err,NO_MSG);
#endif /* CONFIG_GUEST_PARTITION */

#ifndef CONFIG_GUEST_PARTITION

    /**********************FMPL_GCR******************/
    tmpReg32 = 0;
    tmpReg32 |= FM_PCD_PLCR_GCR_STEN;
    if(p_Param->plcrAutoRefresh)
        tmpReg32 |= FM_PCD_PLCR_GCR_DAR;
    tmpReg32 |= NIA_ENG_BMI | NIA_BMI_AC_ENQ_FRAME;

    WRITE_UINT32(p_Regs->fmpl_gcr, tmpReg32);
    /**********************FMPL_GCR******************/

    /**********************FMPL_EEVR******************/
    WRITE_UINT32(p_Regs->fmpl_eevr, (FM_PCD_PLCR_DOUBLE_ECC | FM_PCD_PLCR_INIT_ENTRY_ERROR));
    /**********************FMPL_EEVR******************/
    /**********************FMPL_EIER******************/
    tmpReg32 = 0;
    if(p_FmPcd->exceptions & FM_PCD_EX_PLCR_DOUBLE_ECC)
    {
        if(!FmRamsEccIsExternalCtl(p_FmPcd->h_Fm))
            FM_EnableRamsEcc(p_FmPcd->h_Fm);
        tmpReg32 |= FM_PCD_PLCR_DOUBLE_ECC;
    }
    if(p_FmPcd->exceptions & FM_PCD_EX_PLCR_INIT_ENTRY_ERROR)
        tmpReg32 |= FM_PCD_PLCR_INIT_ENTRY_ERROR;
    WRITE_UINT32(p_Regs->fmpl_eier, tmpReg32);
    /**********************FMPL_EIER******************/

    /**********************FMPL_EVR******************/
    WRITE_UINT32(p_Regs->fmpl_evr, (FM_PCD_PLCR_PRAM_SELF_INIT_COMPLETE | FM_PCD_PLCR_ATOMIC_ACTION_COMPLETE));
    /**********************FMPL_EVR******************/
    /**********************FMPL_IER******************/
    tmpReg32 = 0;
    if(p_FmPcd->exceptions & FM_PCD_EX_PLCR_PRAM_SELF_INIT_COMPLETE)
        tmpReg32 |= FM_PCD_PLCR_PRAM_SELF_INIT_COMPLETE;
    if(p_FmPcd->exceptions & FM_PCD_EX_PLCR_ATOMIC_ACTION_COMPLETE )
        tmpReg32 |= FM_PCD_PLCR_ATOMIC_ACTION_COMPLETE;
    WRITE_UINT32(p_Regs->fmpl_ier, tmpReg32);
    /**********************FMPL_IER******************/

    /* register even if no interrupts enabled, to allow future enablement */
    FmRegisterIntr(p_FmPcd->h_Fm, e_FM_EV_ERR_PLCR, PcdPlcrErrorException, p_FmPcd);
    FmRegisterIntr(p_FmPcd->h_Fm, e_FM_EV_PLCR, PcdPlcrException, p_FmPcd);

    /* driver initializes one DFLT profile at the last entry*/
    p_FmPcd->p_FmPcdPlcr->profiles[FM_PCD_PLCR_NUM_ENTRIES-1].profilesMng.allocated = TRUE;

    /**********************FMPL_DPMR******************/
    tmpReg32 = FM_PCD_PLCR_NUM_ENTRIES-1;
    WRITE_UINT32(p_Regs->fmpl_dpmr, tmpReg32);

#endif /* CONFIG_GUEST_PARTITION */
    return E_OK;
}

#ifndef CONFIG_GUEST_PARTITION
t_Error PlcrEnable(t_FmPcd *p_FmPcd)
{
    t_FmPcdPlcrRegs             *p_Regs = p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs;

    WRITE_UINT32(p_Regs->fmpl_gcr, GET_UINT32(p_Regs->fmpl_gcr) | FM_PCD_PLCR_GCR_EN);

    return E_OK;
}
#endif /* CONFIG_GUEST_PARTITION */

t_Error FmPcdPlcrGetAbsoluteProfileId(t_Handle h_FmPcd,
                                e_FmPcdProfileTypeSelection profileType,
                                t_Handle  h_FmPort,
                                uint16_t relativeProfile,
                                uint16_t *p_AbsoluteId)
{
    t_FmPcd         *p_FmPcd            = (t_FmPcd*)h_FmPcd;
    t_FmPcdPlcr     *p_FmPcdPlcr        = p_FmPcd->p_FmPcdPlcr;
    uint8_t         i;

    switch (profileType)
    {
        case e_FM_PCD_PLCR_PORT_PRIVATE:
            /* get port PCD id from port handle */
            for(i=0;i<PCD_MAX_NUM_OF_PORTS;i++)
                if(p_FmPcd->p_FmPcdPlcr->portsMapping[i].h_FmPort == h_FmPort)
                    break;
            if (i ==  PCD_MAX_NUM_OF_PORTS)
                RETURN_ERROR(MAJOR, E_INVALID_STATE , ("Invalid port handle."));

            if(!p_FmPcd->p_FmPcdPlcr->portsMapping[i].numOfProfiles)
                RETURN_ERROR(MAJOR, E_INVALID_SELECTION , ("Port has no allocated profiles"));
            if(relativeProfile >= p_FmPcd->p_FmPcdPlcr->portsMapping[i].numOfProfiles)
                RETURN_ERROR(MAJOR, E_INVALID_SELECTION , ("Profile id is out of range"));
            *p_AbsoluteId = (uint16_t)(p_FmPcd->p_FmPcdPlcr->portsMapping[i].profilesBase + relativeProfile);
            break;
        case e_FM_PCD_PLCR_SHARED:
            if(relativeProfile >= p_FmPcdPlcr->numOfSharedProfiles)
                RETURN_ERROR(MAJOR, E_INVALID_SELECTION , ("Profile id is out of range"));
            *p_AbsoluteId = (uint16_t)(p_FmPcdPlcr->sharedProfilesIds[relativeProfile]);
            break;
        default:
            RETURN_ERROR(MAJOR, E_INVALID_SELECTION, ("Invalid policer profile type"));
    }
    return E_OK;
}

uint16_t FmPcdPlcrGetPortProfilesBase(t_Handle h_FmPcd, uint8_t hardwarePortId)
{
    t_FmPcd         *p_FmPcd = (t_FmPcd *)h_FmPcd;
    uint16_t        pcdPortId;
    uint8_t         portsTable[] = PCD_PORTS_TABLE;

    GET_FM_PCD_PORTID_FROM_GLOBAL_PORTID(pcdPortId, portsTable, hardwarePortId)

    return p_FmPcd->p_FmPcdPlcr->portsMapping[pcdPortId].profilesBase;
}

uint16_t FmPcdPlcrGetPortNumOfProfiles(t_Handle h_FmPcd, uint8_t hardwarePortId)
{
    t_FmPcd         *p_FmPcd = (t_FmPcd *)h_FmPcd;
    uint16_t        pcdPortId;
    uint8_t         portsTable[] = PCD_PORTS_TABLE;

    GET_FM_PCD_PORTID_FROM_GLOBAL_PORTID(pcdPortId, portsTable, hardwarePortId)

    return p_FmPcd->p_FmPcdPlcr->portsMapping[pcdPortId].numOfProfiles;

}
uint32_t FmPcdPlcrBuildWritePlcrActionReg(uint16_t absoluteProfileId)
{
    return (uint32_t)(FM_PCD_PLCR_PAR_GO |
                        (absoluteProfileId << FM_PCD_PLCR_PAR_PNUM_SHIFT));
}

uint32_t FmPcdPlcrBuildWritePlcrActionRegs(uint16_t absoluteProfileId)
{
    return (uint32_t)(FM_PCD_PLCR_PAR_GO |
                        (absoluteProfileId << FM_PCD_PLCR_PAR_PNUM_SHIFT) |
                        FM_PCD_PLCR_PAR_PWSEL_MASK);
}

bool    FmPcdPlcrHwProfileIsValid(uint32_t profileModeReg)
{

    if(profileModeReg & FM_PCD_PLCR_PEMODE_PI)
        return TRUE;
    else
        return FALSE;
}

uint32_t FmPcdPlcrBuildReadPlcrActionReg(uint16_t absoluteProfileId)
{
    return (uint32_t)(FM_PCD_PLCR_PAR_GO |
                        FM_PCD_PLCR_PAR_R |
                        (absoluteProfileId << FM_PCD_PLCR_PAR_PNUM_SHIFT) |
                        FM_PCD_PLCR_PAR_PWSEL_MASK);
}

uint32_t FmPcdPlcrBuildCounterProfileReg(e_FmPcdPlcrProfileCounters counter)
{
    switch(counter)
    {
        case(e_FM_PCD_PLCR_PROFILE_GREEN_PACKET_TOTAL_COUNTER):
            return FM_PCD_PLCR_PAR_PWSEL_PEGPC;
        case(e_FM_PCD_PLCR_PROFILE_YELLOW_PACKET_TOTAL_COUNTER):
            return FM_PCD_PLCR_PAR_PWSEL_PEYPC;
        case(e_FM_PCD_PLCR_PROFILE_RED_PACKET_TOTAL_COUNTER) :
            return FM_PCD_PLCR_PAR_PWSEL_PERPC;
        case(e_FM_PCD_PLCR_PROFILE_RECOLOURED_YELLOW_PACKET_TOTAL_COUNTER) :
            return FM_PCD_PLCR_PAR_PWSEL_PERYPC;
        case(e_FM_PCD_PLCR_PROFILE_RECOLOURED_RED_PACKET_TOTAL_COUNTER) :
            return FM_PCD_PLCR_PAR_PWSEL_PERRPC;
       default:
            REPORT_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
            return 0;

    }
}

t_Error FmPcdPlcrProfileTryLock(t_Handle h_FmPcd, uint16_t profileId)
{
    TRY_LOCK_RET_ERR(((t_FmPcd*)h_FmPcd)->p_FmPcdPlcr->profiles[profileId].lock);
    return E_OK;
}

void FmPcdPlcrReleaseProfileLock(t_Handle h_FmPcd, uint16_t profileId)
{
    RELEASE_LOCK(((t_FmPcd*)h_FmPcd)->p_FmPcdPlcr->profiles[profileId].lock);
}

/**************************************************/
/*............Policer API.........................*/
/**************************************************/

t_Handle FM_PCD_PlcrSetProfile(t_Handle     h_FmPcd,
                              t_FmPcdPlcrProfileParams *p_Profile)
{
    t_FmPcd                             *p_FmPcd            = (t_FmPcd*)h_FmPcd;
#ifndef CONFIG_MULTI_PARTITION_SUPPORT
    t_FmPcdPlcr                         *p_FmPcdPlcr        = p_FmPcd->p_FmPcdPlcr;
    t_FmPcdPlcrRegs                     *p_FmPcdPlcrRegs    = p_FmPcdPlcr->p_FmPcdPlcrRegs;
    t_FmPcdPlcrInterModuleProfileRegs   plcrProfileReg;
    uint16_t                            absoluteProfileId;
    t_Error                             err;
    uint32_t                            tmpReg32;
#endif /* !CONFIG_MULTI_PARTITION_SUPPORT */

    SANITY_CHECK_RETURN_VALUE(p_FmPcd, E_INVALID_HANDLE, NULL);
    SANITY_CHECK_RETURN_VALUE(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE, NULL);

    if (p_FmPcd->h_Hc)
        return FmHcPcdPlcrSetProfile(p_FmPcd->h_Hc, p_Profile);
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    else
    {
        REPORT_ERROR(MAJOR, E_INVALID_HANDLE, ("No HC obj. must have HC in guest partition!"));
        return NULL;
    }

#else
    SANITY_CHECK_RETURN_VALUE(p_FmPcdPlcr, E_INVALID_HANDLE, NULL);
    SANITY_CHECK_RETURN_VALUE(p_FmPcdPlcrRegs, E_INVALID_HANDLE, NULL);


    if (p_Profile->modify)
    {
        absoluteProfileId = (uint16_t)(CAST_POINTER_TO_UINT32(p_Profile->id.h_Profile)-1);
        TRY_LOCK_RET_NULL(p_FmPcd->p_FmPcdPlcr->profiles[absoluteProfileId].lock);
    }
    else
    {
        TRY_LOCK_RET_NULL(p_FmPcd->lock);
        err = FmPcdPlcrGetAbsoluteProfileId(h_FmPcd,
                                            p_Profile->id.newParams.profileType,
                                            p_Profile->id.newParams.h_FmPort,
                                            p_Profile->id.newParams.relativeProfileId,
                                            &absoluteProfileId);
        if(err)
        {
            RELEASE_LOCK(p_FmPcd->lock);
            REPORT_ERROR(MAJOR, err, NO_MSG);
            return NULL;
        }
        TRY_LOCK_RET_NULL(p_FmPcd->p_FmPcdPlcr->profiles[absoluteProfileId].lock);

        RELEASE_LOCK(p_FmPcd->lock);
    }

    if (absoluteProfileId > FM_PCD_PLCR_NUM_ENTRIES)
    {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("profileId too Big "));
        return NULL;
    }

    /* if no override, check first that this scheme is unused */
    if(!p_Profile->modify)
    {
        /* read specified profile into profile registers */
        tmpReg32 = FmPcdPlcrBuildReadPlcrActionReg(absoluteProfileId);
        WritePar(p_FmPcd, tmpReg32);
        tmpReg32 = GET_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pemode);
        if (tmpReg32 & FM_PCD_PLCR_PEMODE_PI)
        {
            RELEASE_LOCK(p_FmPcd->p_FmPcdPlcr->profiles[absoluteProfileId].lock);
            REPORT_ERROR(MAJOR, E_ALREADY_EXISTS, ("Polcer Profile is already used"));
            return NULL;
        }
    }

    memset(&plcrProfileReg, 0, sizeof(t_FmPcdPlcrInterModuleProfileRegs));

    err =  FmPcdPlcrBuildProfile(h_FmPcd, p_Profile, &plcrProfileReg);
    if(err)
    {
        RELEASE_LOCK(p_FmPcd->p_FmPcdPlcr->profiles[absoluteProfileId].lock);
        REPORT_ERROR(MAJOR, err, NO_MSG);
        return NULL;
    }

    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pemode , plcrProfileReg.fmpl_pemode);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pegnia , plcrProfileReg.fmpl_pegnia);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_peynia , plcrProfileReg.fmpl_peynia);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pernia , plcrProfileReg.fmpl_pernia);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pecir  , plcrProfileReg.fmpl_pecir);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pecbs  , plcrProfileReg.fmpl_pecbs);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pepepir_eir,plcrProfileReg.fmpl_pepepir_eir);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pepbs_ebs,plcrProfileReg.fmpl_pepbs_ebs);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pelts  , plcrProfileReg.fmpl_pelts);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pects  , plcrProfileReg.fmpl_pects);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pepts_ets,plcrProfileReg.fmpl_pepts_ets);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pegpc  , plcrProfileReg.fmpl_pegpc);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_peypc  , plcrProfileReg.fmpl_peypc);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_perpc  , plcrProfileReg.fmpl_perpc);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_perypc , plcrProfileReg.fmpl_perypc);
    WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_perrpc , plcrProfileReg.fmpl_perrpc);

    tmpReg32 = FmPcdPlcrBuildWritePlcrActionRegs(absoluteProfileId);
    WritePar(p_FmPcd, tmpReg32);

    FmPcdPlcrValidateProfileSw(p_FmPcd,absoluteProfileId);

    RELEASE_LOCK(p_FmPcd->p_FmPcdPlcr->profiles[absoluteProfileId].lock);

    return CAST_UINT32_TO_POINTER((uint32_t)absoluteProfileId+1);
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
}

t_Error FM_PCD_PlcrDeleteProfile(t_Handle h_FmPcd, t_Handle h_Profile)
{
    t_FmPcd         *p_FmPcd            = (t_FmPcd*)h_FmPcd;
#ifndef CONFIG_MULTI_PARTITION_SUPPORT
    uint16_t        profileIndx = (uint16_t)(CAST_POINTER_TO_UINT32(h_Profile)-1);
    t_FmPcdPlcr     *p_FmPcdPlcr        = p_FmPcd->p_FmPcdPlcr;
    uint32_t        tmpReg32;
#endif /* !CONFIG_MULTI_PARTITION_SUPPORT */

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdPlcr, E_INVALID_HANDLE);

    if (p_FmPcd->h_Hc)
        return FmHcPcdPlcrDeleteProfile(p_FmPcd->h_Hc, h_Profile);
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    else
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("No HC obj. must have HC in guest partition!"));

#else
    SANITY_CHECK_RETURN_ERROR((profileIndx <= FM_PCD_PLCR_NUM_ENTRIES), E_INVALID_SELECTION);

    FmPcdPlcrInvalidateProfileSw(p_FmPcd,profileIndx);

    WRITE_UINT32(p_FmPcdPlcr->p_FmPcdPlcrRegs->profileRegs.fmpl_pemode, ~FM_PCD_PLCR_PEMODE_PI);

    tmpReg32 = FmPcdPlcrBuildWritePlcrActionRegs(profileIndx);
    WritePar(p_FmPcd, tmpReg32);

    return E_OK;
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
}

/* ......... */
/***************************************************/
/*............Policer Profile Counter..............*/
/***************************************************/
uint32_t FM_PCD_PlcrGetProfileCounter(t_Handle h_FmPcd, t_Handle h_Profile, e_FmPcdPlcrProfileCounters counter)
{
    t_FmPcd         *p_FmPcd            = (t_FmPcd*)h_FmPcd;
#ifndef CONFIG_MULTI_PARTITION_SUPPORT
    uint16_t        profileIndx = (uint16_t)(CAST_POINTER_TO_UINT32(h_Profile)-1);
    t_FmPcdPlcr     *p_FmPcdPlcr        = p_FmPcd->p_FmPcdPlcr;
    t_FmPcdPlcrRegs *p_FmPcdPlcrRegs    = p_FmPcdPlcr->p_FmPcdPlcrRegs;
#endif /* !CONFIG_MULTI_PARTITION_SUPPORT */

    SANITY_CHECK_RETURN_VALUE(p_FmPcd, E_INVALID_HANDLE,0);
    SANITY_CHECK_RETURN_VALUE(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE, 0);

    if (p_FmPcd->h_Hc)
        return FmHcPcdPlcrGetProfileCounter(p_FmPcd->h_Hc, h_Profile, counter);
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    else
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("No HC obj. must have HC in guest partition!"));

#else
    SANITY_CHECK_RETURN_VALUE(p_FmPcdPlcr, E_INVALID_HANDLE,0);
    SANITY_CHECK_RETURN_VALUE(p_FmPcdPlcrRegs, E_INVALID_HANDLE,0);

    if (profileIndx >= FM_PCD_PLCR_NUM_ENTRIES)
    {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("profileId too Big "));
        return 0;
    }

    WritePar(p_FmPcd, FmPcdPlcrBuildReadPlcrActionReg(profileIndx));

    if (!(GET_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pemode) & FM_PCD_PLCR_PEMODE_PI))
    {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("Uninitialized profile"));
        return 0;
    }

    switch (counter)
    {
        case e_FM_PCD_PLCR_PROFILE_GREEN_PACKET_TOTAL_COUNTER:
            return (GET_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pegpc));
            break;
        case e_FM_PCD_PLCR_PROFILE_YELLOW_PACKET_TOTAL_COUNTER:
            return  GET_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_peypc);
            break;
        case e_FM_PCD_PLCR_PROFILE_RED_PACKET_TOTAL_COUNTER:
            return  GET_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_perpc);
            break;
        case e_FM_PCD_PLCR_PROFILE_RECOLOURED_YELLOW_PACKET_TOTAL_COUNTER:
            return  GET_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_perypc);
            break;
        case e_FM_PCD_PLCR_PROFILE_RECOLOURED_RED_PACKET_TOTAL_COUNTER:
            return  GET_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_perrpc);
            break;
        default:
            REPORT_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
            return 0;
    }

    return 0;
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
}

t_Error FM_PCD_PlcrSetProfileCounter(t_Handle h_FmPcd, t_Handle h_Profile, e_FmPcdPlcrProfileCounters counter, uint32_t value)
{
    t_FmPcd         *p_FmPcd            = (t_FmPcd*)h_FmPcd;
#ifndef CONFIG_MULTI_PARTITION_SUPPORT
    uint16_t        profileIndx = (uint16_t)(CAST_POINTER_TO_UINT32(h_Profile)-1);
    t_FmPcdPlcr     *p_FmPcdPlcr        = p_FmPcd->p_FmPcdPlcr;
    t_FmPcdPlcrRegs *p_FmPcdPlcrRegs    = p_FmPcdPlcr->p_FmPcdPlcrRegs;
    uint32_t        tmpReg32;
#endif /* !CONFIG_MULTI_PARTITION_SUPPORT */

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE);

    if (p_FmPcd->h_Hc)
        return FmHcPcdPlcrSetProfileCounter(p_FmPcd->h_Hc, h_Profile, counter, value);
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    else
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("No HC obj. must have HC in guest partition!"));

#else
    SANITY_CHECK_RETURN_ERROR(p_FmPcdPlcr, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcdPlcrRegs, E_INVALID_HANDLE);

    switch (counter)
    {
        case e_FM_PCD_PLCR_PROFILE_GREEN_PACKET_TOTAL_COUNTER:
             WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_pegpc, value);
             break;
        case e_FM_PCD_PLCR_PROFILE_YELLOW_PACKET_TOTAL_COUNTER:
             WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_peypc, value);
             break;
        case e_FM_PCD_PLCR_PROFILE_RED_PACKET_TOTAL_COUNTER:
             WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_perpc, value);
             break;
        case e_FM_PCD_PLCR_PROFILE_RECOLOURED_YELLOW_PACKET_TOTAL_COUNTER:
             WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_perypc ,value);
             break;
        case e_FM_PCD_PLCR_PROFILE_RECOLOURED_RED_PACKET_TOTAL_COUNTER:
             WRITE_UINT32(p_FmPcdPlcrRegs->profileRegs.fmpl_perrpc ,value);
             break;
        default:
            RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
    }

    /*  Activate the atomic write action by writing FMPL_PAR with: GO=1, RW=1, PSI=0, PNUM =
     *  Profile Number, PWSEL=0xFFFF (select all words).
     */
    tmpReg32 = FmPcdPlcrBuildWritePlcrActionReg(profileIndx);
    tmpReg32 |= FmPcdPlcrBuildCounterProfileReg(counter);
    WritePar(p_FmPcd, tmpReg32);

     return E_OK;
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
}

t_Error FM_PCD_ConfigPlcrNumOfSharedProfiles(t_Handle h_FmPcd, uint16_t numOfSharedPlcrProfiles)
{
   t_FmPcd *p_FmPcd = (t_FmPcd*)h_FmPcd;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdDriverParam, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdPlcr, E_INVALID_HANDLE);

    p_FmPcd->p_FmPcdPlcr->numOfSharedProfiles = numOfSharedPlcrProfiles;

    return E_OK;
}

t_Error FM_PCD_SetPlcrStatistics(t_Handle h_FmPcd, bool enable)
{
   t_FmPcd  *p_FmPcd = (t_FmPcd*)h_FmPcd;
#ifndef CONFIG_GUEST_PARTITION
   uint32_t tmpReg32;
#else
    UNUSED(enable);
#endif /* !CONFIG_GUEST_PARTITION */

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdPlcr, E_INVALID_HANDLE);
#ifndef CONFIG_GUEST_PARTITION
    tmpReg32 =  GET_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_gcr);
    if(enable)
        tmpReg32 |= FM_PCD_PLCR_GCR_STEN;
    else
        tmpReg32 &= ~FM_PCD_PLCR_GCR_STEN;

    WRITE_UINT32(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_gcr, tmpReg32);
#else
    RETURN_ERROR(MINOR, E_NOT_SUPPORTED, NO_MSG);
#endif
    return E_OK;
}

#ifndef CONFIG_GUEST_PARTITION
t_Error FM_PCD_ConfigPlcrAutoRefreshMode(t_Handle h_FmPcd, bool enable)
{
   t_FmPcd *p_FmPcd = (t_FmPcd*)h_FmPcd;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdDriverParam, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdPlcr, E_INVALID_HANDLE);

    p_FmPcd->p_FmPcdDriverParam->plcrAutoRefresh = enable;

    return E_OK;
}

/* ... */

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
t_Error FM_PCD_PlcrDumpRegs(t_Handle h_FmPcd)
{
    t_FmPcd                             *p_FmPcd = (t_FmPcd*)h_FmPcd;
    t_FmPcdPlcrInterModuleProfileRegs   *p_ProfilesRegs;
    int                                 i = 0;
    uint32_t                            tmpReg;

    DECLARE_DUMP;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdPlcr, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE);

    DUMP_SUBTITLE(("\n"));
    DUMP_TITLE(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs, ("FmPcdPlcrRegs Regs"));

    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_gcr);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_gsr);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_evr);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_ier);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_ifr);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_eevr);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_eier);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_eifr);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_rpcnt);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_ypcnt);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_rrpcnt);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_rypcnt);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_tpcnt);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_flmcnt);

    p_ProfilesRegs = &p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->profileRegs;

    for(i = 0;i<FM_PCD_PLCR_NUM_ENTRIES; i++)
    {
        tmpReg = FmPcdPlcrBuildReadPlcrActionReg((uint16_t)i);
        WritePar(p_FmPcd, tmpReg);

        DUMP_TITLE(p_ProfilesRegs, ("Profile %d regs", i));

        DUMP_VAR(p_ProfilesRegs, fmpl_pemode);
        DUMP_VAR(p_ProfilesRegs, fmpl_pegnia);
        DUMP_VAR(p_ProfilesRegs, fmpl_peynia);
        DUMP_VAR(p_ProfilesRegs, fmpl_pernia);
        DUMP_VAR(p_ProfilesRegs, fmpl_pecir);
        DUMP_VAR(p_ProfilesRegs, fmpl_pecbs);
        DUMP_VAR(p_ProfilesRegs, fmpl_pepepir_eir);
        DUMP_VAR(p_ProfilesRegs, fmpl_pepbs_ebs);
        DUMP_VAR(p_ProfilesRegs, fmpl_pelts);
        DUMP_VAR(p_ProfilesRegs, fmpl_pects);
        DUMP_VAR(p_ProfilesRegs, fmpl_pepts_ets);
        DUMP_VAR(p_ProfilesRegs, fmpl_pegpc);
        DUMP_VAR(p_ProfilesRegs, fmpl_peypc);
        DUMP_VAR(p_ProfilesRegs, fmpl_perpc);
        DUMP_VAR(p_ProfilesRegs, fmpl_perypc);
        DUMP_VAR(p_ProfilesRegs, fmpl_perrpc);
    }

    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_serc);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_upcr);
    DUMP_VAR(p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs,fmpl_dpmr);


    DUMP_TITLE(&p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_pmr, ("fmpl_pmr"));
    DUMP_SUBSTRUCT_ARRAY(i, 63)
    {
        DUMP_MEMORY(&p_FmPcd->p_FmPcdPlcr->p_FmPcdPlcrRegs->fmpl_pmr[i], sizeof(uint32_t));
    }

    return E_OK;
}
#endif /* (defined(DEBUG_ERRORS) && ... */
#endif /* ! CONFIG_GUEST_PARTITION */
