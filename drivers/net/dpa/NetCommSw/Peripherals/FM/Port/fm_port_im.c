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
 @File          fm_port_im.c

 @Description   FM Port Independent-Mode ...
*//***************************************************************************/
#include "std_ext.h"
#include "string_ext.h"
#include "error_ext.h"
#include "fm_muram_ext.h"

#include "fm_port.h"


#define TX_CONF_STATUS_UNSENT 0x1

#ifdef CORE_8BIT_ACCESS_ERRATA
#define MY_WRITE_UINT16(addr, val)                          \
do {                                                        \
    if (addr%4)                                             \
    {                                                       \
        uint32_t    newAddr = (uint32_t)((addr>>2)<<2);     \
        uint32_t    tmp = GET_UINT32(newAddr);              \
        tmp = (uint32_t)((tmp & 0xffff0000) | val);         \
        WRITE_UINT32(newAddr, tmp);                         \
    }                                                       \
    else                                                    \
    {                                                       \
        uint32_t    tmp = GET_UINT32(addr);                 \
        tmp = (uint32_t)((tmp & 0x0000ffff) | (((uint32_t)val)<<16));\
        WRITE_UINT32(addr, tmp);                            \
    }                                                       \
} while (0)

#define MY_WRITE_UINT8(addr,val) MY_MY_WRITE_UINT8(&addr,val)
#define MY_GET_UINT8(addr) MY_MY_GET_UINT8(&addr)
#else
#define MY_WRITE_UINT16 WRITE_UINT16
#define MY_GET_UINT16   GET_UINT16
#endif /* CORE_8BIT_ACCESS_ERRATA */

typedef enum e_TxConfType
{
     e_TX_CONF_TYPE_CHECK      = 0  /**< check if all the buffers were touched by the muxator, no confirmation callback */
    ,e_TX_CONF_TYPE_CALLBACK   = 1  /**< confirm to user all the available sent buffers */
    ,e_TX_CONF_TYPE_FLUSH      = 3  /**< confirm all buffers plus the unsent one with an appropriate status */
} e_TxConfType;


static void DiscardCurrentTxFrame(t_FmPort *p_FmPort)
{
    uint16_t   cleanBdId = p_FmPort->im.firstBdOfFrameId;

    if ((cleanBdId == IM_ILEGAL_BD_ID) ||
        (cleanBdId == p_FmPort->im.currBdId))
        return;

    /* Since firstInFrame is not NULL, one buffer at least has already been
       inserted into the BD ring. Using do-while covers the situation of a
       frame spanned throughout the whole Tx BD ring (p_CleanBd is incremented
       prior to testing whether or not it's equal to TxBd). */
    do
    {
        BD_STATUS_AND_LENGTH_SET(BD_GET(cleanBdId), 0);
        /* Advance BD pointer */
        cleanBdId = GetNextBdId(p_FmPort, cleanBdId);
    } while (cleanBdId != p_FmPort->im.currBdId);

    p_FmPort->im.firstBdOfFrameId = IM_ILEGAL_BD_ID;
}

static t_Error FmPortTxConf(t_FmPort *p_FmPort, e_TxConfType confType)
{
    t_Error             retVal = E_BUSY;
    uint32_t            bdStatus;
    uint16_t            savedStartBdId;

    ASSERT_COND(p_FmPort);

//    if (confType==e_TX_CONF_TYPE_CHECK)
//        return (WfqEntryIsQueueEmpty(p_FmPort->im.h_WfqEntry) ? E_OK : E_BUSY);

    savedStartBdId = p_FmPort->im.currBdId;
    bdStatus = BD_STATUS_AND_LENGTH(BD_GET(p_FmPort->im.currBdId));

    /* if R bit is set, we don't enter, or we break.
    we run till we get to R, or complete the loop */
    while ((!(bdStatus & BD_R_E) || (confType == e_TX_CONF_TYPE_FLUSH)) && (retVal != E_OK))
    {
        if (confType & e_TX_CONF_TYPE_CALLBACK) /* if it is confirmation with user callbacks */
            BD_STATUS_AND_LENGTH_SET(BD_GET(p_FmPort->im.currBdId), 0);

        /* case 1: R bit is 0 and Length is set -> confirm! */
        if ((confType & e_TX_CONF_TYPE_CALLBACK) && (bdStatus & BD_LENGTH_MASK))
        {
            if (p_FmPort->im.f_TxConfCB)
            {
                if ((confType == e_TX_CONF_TYPE_FLUSH) && (bdStatus & BD_R_E))
                    p_FmPort->im.f_TxConfCB(p_FmPort->im.h_App,
                                            BD_BUFFER(BD_GET(p_FmPort->im.currBdId)),
                                            TX_CONF_STATUS_UNSENT,
                                            p_FmPort->im.p_BdShadow[p_FmPort->im.currBdId]);
                else
                    p_FmPort->im.f_TxConfCB(p_FmPort->im.h_App,
                                            BD_BUFFER(BD_GET(p_FmPort->im.currBdId)),
                                            0,
                                            p_FmPort->im.p_BdShadow[p_FmPort->im.currBdId]);
            }
        }
        /* case 2: R bit is 0 and Length is 0 -> not used yet, nop! */

        p_FmPort->im.currBdId = GetNextBdId(p_FmPort, p_FmPort->im.currBdId);
        if (p_FmPort->im.currBdId == savedStartBdId)
            retVal = E_OK;
        bdStatus = BD_STATUS_AND_LENGTH(BD_GET(p_FmPort->im.currBdId));
    }

    MY_WRITE_UINT16(p_FmPort->im.p_FmPortImPram->txQd.offsetIn, (uint16_t)(p_FmPort->im.currBdId<<4));

    return retVal;
}


t_Error FmPortImRx(t_FmPort *p_FmPort)
{
//    e_ExceptionsSelect      exceptions = e_EX_NONE;
    t_Handle                h_CurrUserPriv, h_NewUserPriv;
    uint32_t                bdStatus;
    volatile uint8_t        buffPos;
    uint16_t                length;
    uint16_t                errors/*, reportErrors*/;
    uint8_t                 *p_CurData, *p_Data;

    ASSERT_COND(p_FmPort);

    bdStatus = BD_STATUS_AND_LENGTH(BD_GET(p_FmPort->im.currBdId));

    while (!(bdStatus & BD_R_E)) /* while there is data in the Rx BD */
    {
        if (p_FmPort->im.firstBdOfFrameId == IM_ILEGAL_BD_ID)
            p_FmPort->im.firstBdOfFrameId = p_FmPort->im.currBdId;

        errors = 0;
        p_CurData = BD_BUFFER(BD_GET(p_FmPort->im.currBdId));
        h_CurrUserPriv = p_FmPort->im.p_BdShadow[p_FmPort->im.currBdId];
        length = (uint16_t)((bdStatus & BD_L) ?
                            ((bdStatus & BD_LENGTH_MASK) - p_FmPort->im.rxFrameAccumLength):
                            (bdStatus & BD_LENGTH_MASK));
        p_FmPort->im.rxFrameAccumLength += length;

        /* determine whether buffer is first, last, first and last (single  */
        /* buffer frame) or middle (not first and not last)                 */
        buffPos = (uint8_t)((p_FmPort->im.currBdId == p_FmPort->im.firstBdOfFrameId) ?
                            ((bdStatus & BD_L) ? SINGLE_BUF : FIRST_BUF) :
                            ((bdStatus & BD_L) ? LAST_BUF : MIDDLE_BUF));

        if (bdStatus & BD_L)
        {
            p_FmPort->im.rxFrameAccumLength = 0;
            p_FmPort->im.firstBdOfFrameId = IM_ILEGAL_BD_ID;
        }

        if ((p_Data = p_FmPort->im.rxPool.f_GetBuf(p_FmPort->im.rxPool.h_BufferPool, &h_NewUserPriv)) == NULL)
            RETURN_ERROR(MAJOR, E_NOT_AVAILABLE, ("Data buffer"));

        BD_BUFFER_SET(BD_GET(p_FmPort->im.currBdId), p_Data);

        BD_STATUS_AND_LENGTH_SET(BD_GET(p_FmPort->im.currBdId), BD_R_E);

//#warning "add here support for errors!!!"
        errors = (uint16_t)((bdStatus & BD_RX_ERRORS) >> 16);
#if 0
        /* find out which errors the user wants reported. The BD will
        still be passed to the user, but first f_Exceptions will be called */
        reportErrors = (uint16_t)(errors & p_FmPort->im.bdErrorsReport);
        if(reportErrors)
        {
            QUEUE_GET_EXCEPTIONS(reportErrors, exceptions);
            p_FmPort->im.f_Exceptions(p_FmPort->im.h_App, exceptions, 0);
        }
#endif /* 0 */

        /* Pass the buffer if one of the conditions is true:
        - There are no errors
        - This is a part of a larger frame ( the application has already received some buffers )
        - There is an error, but it was defined to be passed anyway. */
        if ((buffPos != SINGLE_BUF) || !errors || (errors & (uint16_t)(BD_ERROR_PASS_FRAME>>16)))
        {
            p_FmPort->im.f_RxStoreCB(p_FmPort->im.h_App,
                                     p_CurData,
                                     length,
                                     errors,
                                     buffPos,
                                     h_CurrUserPriv);
        }
        else if (p_FmPort->im.rxPool.f_PutBuf(p_FmPort->im.rxPool.h_BufferPool,
                                              p_CurData,
                                              h_CurrUserPriv))
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Failed freeing data buffer"));

        p_FmPort->im.p_BdShadow[p_FmPort->im.currBdId] = h_NewUserPriv;

        p_FmPort->im.currBdId = GetNextBdId(p_FmPort, p_FmPort->im.currBdId);
        bdStatus = BD_STATUS_AND_LENGTH(BD_GET(p_FmPort->im.currBdId));
        MY_WRITE_UINT16(p_FmPort->im.p_FmPortImPram->rxQd.offsetOut, (uint16_t)(p_FmPort->im.currBdId<<4));
    }

    return E_OK;
}

t_Error FmPortConfigIM (t_FmPort *p_FmPort, t_FmPortParams *p_FmPortParams)
{
    ASSERT_COND(p_FmPort);

    SANITY_CHECK_RETURN_ERROR(p_FmPort->p_FmPortDriverParam, E_INVALID_HANDLE);

    p_FmPort->im.h_App                          = p_FmPortParams->specificParams.imRxTxParams.h_App;
    p_FmPort->im.h_FmMuram                      = p_FmPortParams->specificParams.imRxTxParams.h_FmMuram;
    p_FmPort->p_FmPortDriverParam->partitionId  = p_FmPortParams->specificParams.imRxTxParams.partitionId;
    p_FmPort->im.dataMemId                      = p_FmPortParams->specificParams.imRxTxParams.dataMemId;
    p_FmPort->im.dataMemAttributes              = p_FmPortParams->specificParams.imRxTxParams.dataMemAttributes;

    p_FmPort->im.fwExtStructsMemId      = DEFAULT_PORT_ImfwExtStructsMemId;
    p_FmPort->im.fwExtStructsMemAttr    = DEFAULT_PORT_Im_fwExtStructsMemAttr;

    if ((p_FmPort->portType == e_FM_PORT_TYPE_RX) ||
        (p_FmPort->portType == e_FM_PORT_TYPE_RX_10G))
    {
        p_FmPort->im.rxPool.h_BufferPool    = p_FmPortParams->specificParams.imRxTxParams.rxPoolParams.h_BufferPool;
        p_FmPort->im.rxPool.f_GetBuf        = p_FmPortParams->specificParams.imRxTxParams.rxPoolParams.f_GetBuf;
        p_FmPort->im.rxPool.f_PutBuf        = p_FmPortParams->specificParams.imRxTxParams.rxPoolParams.f_PutBuf;
        p_FmPort->im.rxPool.bufferSize      = p_FmPortParams->specificParams.imRxTxParams.rxPoolParams.bufferSize;
        p_FmPort->im.f_RxStoreCB            = p_FmPortParams->specificParams.imRxTxParams.f_RxStoreCB;

        p_FmPort->im.mrblr                  = DEFAULT_PORT_ImMaxRxBufLength;
        p_FmPort->im.bdRingSize             = DEFAULT_PORT_rxBdRingLength;
    }
    else
    {
        p_FmPort->im.f_TxConfCB             = p_FmPortParams->specificParams.imRxTxParams.f_TxConfCB;

        p_FmPort->im.bdRingSize             = DEFAULT_PORT_txBdRingLength;
    }

    return E_OK;
}

t_Error FmPortImCheckInitParameters(t_FmPort *p_FmPort)
{
    if ((p_FmPort->portType != e_FM_PORT_TYPE_RX) &&
        (p_FmPort->portType != e_FM_PORT_TYPE_RX_10G) &&
        (p_FmPort->portType != e_FM_PORT_TYPE_TX) &&
        (p_FmPort->portType != e_FM_PORT_TYPE_TX_10G))
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, NO_MSG);

    if ((p_FmPort->portType == e_FM_PORT_TYPE_RX) ||
        (p_FmPort->portType == e_FM_PORT_TYPE_RX_10G))
    {
        if (!POWER_OF_2(p_FmPort->im.mrblr))
            RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("max Rx buffer length must be power of 2!!!"));
        if (p_FmPort->im.mrblr < 256)
            RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("max Rx buffer length must at least 256!!!"));
        if(p_FmPort->p_FmPortDriverParam->partitionId >= FM_MAX_NUM_OF_PARTITIONS)
             RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("partitionId can't be larger than %d", FM_MAX_NUM_OF_PARTITIONS-1));
//#warning "add checks"
    }
    else
    {
//#warning "add checks"
    }

    return E_OK;
}

t_Error FmPortImInit(t_FmPort *p_FmPort)
{
    t_FmImBd    *p_Bd=NULL;
    t_Handle    h_UserPriv;
    uint64_t    tmpPhysBase;
    uint16_t    log2Num;
    uint8_t     *p_Data/*, *p_Tmp*/;
    int         i;

    ASSERT_COND(p_FmPort);

    p_FmPort->im.p_FmPortImPram =
        (t_FmPortImPram *)FM_MURAM_AllocMem(p_FmPort->im.h_FmMuram, sizeof(t_FmPortImPram), IM_PRAM_ALIGN);
    if (!p_FmPort->im.p_FmPortImPram)
        RETURN_ERROR(MAJOR, E_NO_MEMORY, ("Independent-Mode Parameter-RAM!!!"));
    WRITE_BLOCK(p_FmPort->im.p_FmPortImPram, 0, sizeof(t_FmPortImPram));

    if ((p_FmPort->portType == e_FM_PORT_TYPE_RX) ||
        (p_FmPort->portType == e_FM_PORT_TYPE_RX_10G))
    {
        p_FmPort->im.p_BdRing = (t_FmImBd *)XX_MallocSmart((uint32_t)(sizeof(t_FmImBd)*p_FmPort->im.bdRingSize), p_FmPort->im.fwExtStructsMemId, 4);
        if (!p_FmPort->im.p_BdRing)
            RETURN_ERROR(MAJOR, E_NO_MEMORY, ("Independent-Mode Rx BD ring!!!"));
        memset(p_FmPort->im.p_BdRing, 0, (uint32_t)(sizeof(t_FmImBd)*p_FmPort->im.bdRingSize));

        p_FmPort->im.p_BdShadow = (t_Handle *)XX_Malloc((uint32_t)(sizeof(t_Handle)*p_FmPort->im.bdRingSize));
        if (!p_FmPort->im.p_BdShadow)
            RETURN_ERROR(MAJOR, E_NO_MEMORY, ("Independent-Mode Rx BD shadow!!!"));
        memset(p_FmPort->im.p_BdShadow, 0, (uint32_t)(sizeof(t_Handle)*p_FmPort->im.bdRingSize));

        /* Initialize the Rx-BD ring */
        for (i=0; i<p_FmPort->im.bdRingSize; i++)
        {
            p_Bd = BD_GET(i);
            BD_STATUS_AND_LENGTH_SET (p_Bd, BD_R_E);

            if ((p_Data = p_FmPort->im.rxPool.f_GetBuf(p_FmPort->im.rxPool.h_BufferPool, &h_UserPriv)) == NULL)
                RETURN_ERROR(MAJOR, E_NOT_AVAILABLE, ("Data buffer"));
            BD_BUFFER_SET(p_Bd, p_Data);
            p_FmPort->im.p_BdShadow[i] = h_UserPriv;
        }

        if (p_FmPort->im.dataMemAttributes & MEMORY_ATTR_CACHEABLE)
            WRITE_UINT32(p_FmPort->im.p_FmPortImPram->mode, IM_MODE_GBL);

        WRITE_UINT32(p_FmPort->im.p_FmPortImPram->rxQdPtr,
                     (uint32_t)(CAST_POINTER_TO_UINT64(XX_VirtToPhys(p_FmPort->im.p_FmPortImPram)) -
                                p_FmPort->p_FmPortDriverParam->fmMuramPhysBaseAddr + 0x20));

        LOG2((uint64_t)p_FmPort->im.mrblr, log2Num);
        MY_WRITE_UINT16(p_FmPort->im.p_FmPortImPram->mrblr, log2Num);

        /* Initialize Rx QD */
        tmpPhysBase = CAST_POINTER_TO_UINT64(XX_VirtToPhys(p_FmPort->im.p_BdRing));
        SET_ADDR(&p_FmPort->im.p_FmPortImPram->rxQd.bdRingBase, tmpPhysBase);
        MY_WRITE_UINT16(p_FmPort->im.p_FmPortImPram->rxQd.bdRingSize, (uint16_t)(sizeof(t_FmImBd)*p_FmPort->im.bdRingSize));

        /* Update the IM PRAM address in the BMI */
        WRITE_UINT32(p_FmPort->p_FmPortBmiRegs->rxPortBmiRegs.fmbm_rfqid,
                     (uint32_t)(CAST_POINTER_TO_UINT64(XX_VirtToPhys(p_FmPort->im.p_FmPortImPram)) -
                                p_FmPort->p_FmPortDriverParam->fmMuramPhysBaseAddr));
    }
    else
    {
        p_FmPort->im.p_BdRing = (t_FmImBd *)XX_MallocSmart((uint32_t)(sizeof(t_FmImBd)*p_FmPort->im.bdRingSize), p_FmPort->im.fwExtStructsMemId, 4);
        if (!p_FmPort->im.p_BdRing)
            RETURN_ERROR(MAJOR, E_NO_MEMORY, ("Independent-Mode Tx BD ring!!!"));
        memset(p_FmPort->im.p_BdRing, 0, (uint32_t)(sizeof(t_FmImBd)*p_FmPort->im.bdRingSize));

        p_FmPort->im.p_BdShadow = (t_Handle *)XX_Malloc((uint32_t)(sizeof(t_Handle)*p_FmPort->im.bdRingSize));
        if (!p_FmPort->im.p_BdShadow)
            RETURN_ERROR(MAJOR, E_NO_MEMORY, ("Independent-Mode Rx BD shadow!!!"));
        memset(p_FmPort->im.p_BdShadow, 0, (uint32_t)(sizeof(t_Handle)*p_FmPort->im.bdRingSize));
        p_FmPort->im.firstBdOfFrameId = IM_ILEGAL_BD_ID;

        if (p_FmPort->im.dataMemAttributes & MEMORY_ATTR_CACHEABLE)
            WRITE_UINT32(p_FmPort->im.p_FmPortImPram->mode, IM_MODE_GBL);

        WRITE_UINT32(p_FmPort->im.p_FmPortImPram->txQdPtr,
                     (uint32_t)(CAST_POINTER_TO_UINT64(XX_VirtToPhys(p_FmPort->im.p_FmPortImPram)) -
                                p_FmPort->p_FmPortDriverParam->fmMuramPhysBaseAddr + 0x40));

        /* Initialize Tx QD */
        tmpPhysBase = CAST_POINTER_TO_UINT64(XX_VirtToPhys(p_FmPort->im.p_BdRing));
        SET_ADDR(&p_FmPort->im.p_FmPortImPram->txQd.bdRingBase, tmpPhysBase);
        MY_WRITE_UINT16(p_FmPort->im.p_FmPortImPram->txQd.bdRingSize, (uint16_t)(sizeof(t_FmImBd)*p_FmPort->im.bdRingSize));

        /* Update the IM PRAM address in the BMI */
        WRITE_UINT32(p_FmPort->p_FmPortBmiRegs->txPortBmiRegs.fmbm_tcfqid,
                     (uint32_t)(CAST_POINTER_TO_UINT64(XX_VirtToPhys(p_FmPort->im.p_FmPortImPram)) -
                                p_FmPort->p_FmPortDriverParam->fmMuramPhysBaseAddr));
    }

    return E_OK;
}

void FmPortImFree(t_FmPort *p_FmPort)
{
    uint32_t    bdStatus;
    uint8_t     *p_CurData;

    ASSERT_COND(p_FmPort);

    if (p_FmPort->im.p_FmPortImPram)
        FM_MURAM_FreeMem(p_FmPort->im.h_FmMuram, p_FmPort->im.p_FmPortImPram);

    if ((p_FmPort->portType == e_FM_PORT_TYPE_RX) ||
        (p_FmPort->portType == e_FM_PORT_TYPE_RX_10G))
    {
        /* Try first clean what has recieved */
        FmPortImRx(p_FmPort);

        /* Now, get rid of the the empty buffer! */
        bdStatus = BD_STATUS_AND_LENGTH(BD_GET(p_FmPort->im.currBdId));

        while (bdStatus & BD_R_E) /* while there is data in the Rx BD */
        {
            p_CurData = BD_BUFFER(BD_GET(p_FmPort->im.currBdId));

            BD_BUFFER_SET(BD_GET(p_FmPort->im.currBdId), NULL);
            BD_STATUS_AND_LENGTH_SET(BD_GET(p_FmPort->im.currBdId), 0);

            p_FmPort->im.rxPool.f_PutBuf(p_FmPort->im.rxPool.h_BufferPool,
                                         p_CurData,
                                         p_FmPort->im.p_BdShadow[p_FmPort->im.currBdId]);

            p_FmPort->im.currBdId = GetNextBdId(p_FmPort, p_FmPort->im.currBdId);
            bdStatus = BD_STATUS_AND_LENGTH(BD_GET(p_FmPort->im.currBdId));
        }
    }
    else
        FmPortTxConf(p_FmPort, e_TX_CONF_TYPE_FLUSH);

    if (p_FmPort->im.p_BdShadow)
        XX_Free(p_FmPort->im.p_BdShadow);

    if (p_FmPort->im.p_BdRing)
        XX_FreeSmart(p_FmPort->im.p_BdRing);
}


t_Error FM_PORT_ConfigIMMaxRxBufLength(t_Handle h_FmPort, uint16_t newVal)
{
    t_FmPort *p_FmPort = (t_FmPort*)h_FmPort;

    SANITY_CHECK_RETURN_ERROR(p_FmPort, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPort->imEn, E_INVALID_STATE);
    SANITY_CHECK_RETURN_ERROR(p_FmPort->p_FmPortDriverParam, E_INVALID_HANDLE);

    p_FmPort->im.mrblr = newVal;

    return E_OK;
}

t_Error FM_PORT_ConfigIMRxBdRingLength(t_Handle h_FmPort, uint16_t newVal)
{
    t_FmPort *p_FmPort = (t_FmPort*)h_FmPort;

    SANITY_CHECK_RETURN_ERROR(p_FmPort, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPort->imEn, E_INVALID_STATE);
    SANITY_CHECK_RETURN_ERROR(p_FmPort->p_FmPortDriverParam, E_INVALID_HANDLE);

    p_FmPort->im.bdRingSize = newVal;

    return E_OK;
}

t_Error FM_PORT_ConfigIMTxBdRingLength(t_Handle h_FmPort, uint16_t newVal)
{
    t_FmPort *p_FmPort = (t_FmPort*)h_FmPort;

    SANITY_CHECK_RETURN_ERROR(p_FmPort, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPort->imEn, E_INVALID_STATE);
    SANITY_CHECK_RETURN_ERROR(p_FmPort->p_FmPortDriverParam, E_INVALID_HANDLE);

    p_FmPort->im.bdRingSize = newVal;

    return E_OK;
}


t_Error  FM_PORT_ImTx( t_Handle               h_FmPort,
                       uint8_t                *p_Data,
                       uint16_t               length,
                       bool                   lastBuffer,
                       t_Handle               h_UserPriv)
{
    t_FmPort            *p_FmPort = (t_FmPort*)h_FmPort;
    uint16_t            nextBdId;
    uint32_t            bdStatus, nextBdStatus;
    bool                firstBuffer;

    SANITY_CHECK_RETURN_ERROR(p_FmPort, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPort->imEn, E_INVALID_STATE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPort->p_FmPortDriverParam, E_INVALID_HANDLE);

    bdStatus = BD_STATUS_AND_LENGTH(BD_GET(p_FmPort->im.currBdId));
    nextBdId = GetNextBdId(p_FmPort, p_FmPort->im.currBdId);
    nextBdStatus = BD_STATUS_AND_LENGTH(BD_GET(nextBdId));

    if (!(bdStatus & BD_R_E) && !(nextBdStatus & BD_R_E))
    {
        /* Confirm the current BD - BD is available */
        if ((bdStatus & BD_LENGTH_MASK) && (p_FmPort->im.f_TxConfCB))
            p_FmPort->im.f_TxConfCB (p_FmPort->im.h_App,
                                     BD_BUFFER(BD_GET(p_FmPort->im.currBdId)),
                                     0,
                                     p_FmPort->im.p_BdShadow[p_FmPort->im.currBdId]);

        bdStatus |= length;

        /* if this is the first BD of a frame */
        if (p_FmPort->im.firstBdOfFrameId == IM_ILEGAL_BD_ID)
        {
            firstBuffer = TRUE;
            p_FmPort->im.txFirstBdStatus = (bdStatus | BD_R_E);

            if (!lastBuffer)
                p_FmPort->im.firstBdOfFrameId = p_FmPort->im.currBdId;
        }
        else
            firstBuffer = FALSE;

        BD_BUFFER_SET(BD_GET(p_FmPort->im.currBdId), p_Data);
        p_FmPort->im.p_BdShadow[p_FmPort->im.currBdId] = h_UserPriv;

        /* deal with last */
        if (lastBuffer)
        {
            /* if single buffer frame */
            if (firstBuffer)
                BD_STATUS_AND_LENGTH_SET(BD_GET(p_FmPort->im.currBdId), p_FmPort->im.txFirstBdStatus | BD_L);
            else
            {
                /* Set the last BD of the frame */
                BD_STATUS_AND_LENGTH_SET (BD_GET(p_FmPort->im.currBdId), (bdStatus | BD_R_E | BD_L));
                /* Set the first BD of the frame */
                BD_STATUS_AND_LENGTH_SET(BD_GET(p_FmPort->im.firstBdOfFrameId), p_FmPort->im.txFirstBdStatus);
                p_FmPort->im.firstBdOfFrameId = IM_ILEGAL_BD_ID;
            }
        }
        else if (!firstBuffer) /* mid frame buffer */
            BD_STATUS_AND_LENGTH_SET (BD_GET(p_FmPort->im.currBdId), bdStatus | BD_R_E);

        p_FmPort->im.currBdId = GetNextBdId(p_FmPort, p_FmPort->im.currBdId);
        MY_WRITE_UINT16(p_FmPort->im.p_FmPortImPram->txQd.offsetIn, (uint16_t)(p_FmPort->im.currBdId<<4));
    }
    else
    {
        /* Discard current frame. Return error.   */
        if (p_FmPort->im.firstBdOfFrameId != IM_ILEGAL_BD_ID)
        {
            ASSERT_COND(p_FmPort->im.firstBdOfFrameId != p_FmPort->im.currBdId);
            /* Error:    No free BD */
            /* Response: Discard current frame. Return error.   */
            DiscardCurrentTxFrame(p_FmPort);
        }

        return E_FULL;
    }

    return E_OK;
}

void FM_PORT_ImTxConf(t_Handle h_FmPort)
{
    t_FmPort *p_FmPort = (t_FmPort*)h_FmPort;

    SANITY_CHECK_RETURN(p_FmPort, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN(p_FmPort->imEn, E_INVALID_STATE);
    SANITY_CHECK_RETURN(!p_FmPort->p_FmPortDriverParam, E_INVALID_HANDLE);

    FmPortTxConf(p_FmPort, e_TX_CONF_TYPE_CALLBACK);
}

t_Error  FM_PORT_ImRx(t_Handle h_FmPort)
{
    t_FmPort *p_FmPort = (t_FmPort*)h_FmPort;

    SANITY_CHECK_RETURN_ERROR(p_FmPort, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPort->imEn, E_INVALID_STATE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPort->p_FmPortDriverParam, E_INVALID_HANDLE);

    return FmPortImRx(p_FmPort);
}
