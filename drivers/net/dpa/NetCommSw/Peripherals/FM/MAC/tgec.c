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
 @File          tgec.c

 @Description   FM 10G MAC ...
*//***************************************************************************/

#include "std_ext.h"
#include "string_ext.h"
#include "error_ext.h"
#include "xx_ext.h"
#include "endian_ext.h"
#include "crc_mac_addr_ext.h"
#include "debug_ext.h"

#include "tgec.h"


/*****************************************************************************/
/*                      Internal routines                                    */
/*****************************************************************************/

static t_Error CheckInitParameters(t_Tgec    *p_Tgec)
{
    if(ENET_SPEED_FROM_MODE(p_Tgec->enetMode) < e_ENET_SPEED_10000)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Ethernet 10G MAC driver only support 10G speed"));
    if(p_Tgec->macId >= FM_MAX_NUM_OF_10G_MACS)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("macId of 10 G can not be greater than 0"));
    if(p_Tgec->addr == 0)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Ethernet 10G MAC Must have a valid MAC Address"));

    return E_OK;
}

/* .............................................................................. */

static void SetDefaultParam(t_TgecDriverParam *p_TgecDriverParam)
{
    p_TgecDriverParam->wanModeEnable            = DEFAULT_wanModeEnable;
    p_TgecDriverParam->promiscuousModeEnable    = DEFAULT_promiscuousModeEnable;
    p_TgecDriverParam->padRemovalEnable         = DEFAULT_padRemovalEnable;
    p_TgecDriverParam->crcForwardEnable         = DEFAULT_crcForwardEnable;
    p_TgecDriverParam->pauseForwardEnable       = DEFAULT_pauseForwardEnable;
    p_TgecDriverParam->pauseIgnore              = DEFAULT_pauseIgnore;
    p_TgecDriverParam->txAddrInsEnable          = DEFAULT_txAddrInsEnable;

    p_TgecDriverParam->loopbackEnable           = DEFAULT_loopbackEnable;
    p_TgecDriverParam->cmdFrameEnable           = DEFAULT_cmdFrameEnable;
    p_TgecDriverParam->rxErrorDiscard           = DEFAULT_rxErrorDiscard;
    p_TgecDriverParam->phyTxenaOn               = DEFAULT_phyTxenaOn;
    p_TgecDriverParam->sendIdleEnable           = DEFAULT_sendIdleEnable;
    p_TgecDriverParam->noLengthCheckEnable      = DEFAULT_noLengthCheckEnable;
    p_TgecDriverParam->lgthCheckNostdr          = DEFAULT_lgthCheckNostdr;
    p_TgecDriverParam->timeStampEnable          = DEFAULT_timeStampEnable;
    p_TgecDriverParam->rxSfdAny                 = DEFAULT_rxSfdAny;
    p_TgecDriverParam->rxPblFwd                 = DEFAULT_rxPblFwd;
    p_TgecDriverParam->txPblFwd                 = DEFAULT_txPblFwd;

    p_TgecDriverParam->txIpgLength              = DEFAULT_txIpgLength;
    p_TgecDriverParam->statisticsEnable         = DEFAULT_statisticsEnable;
    p_TgecDriverParam->maxFrameLength           = DEFAULT_maxFrameLength;
    p_TgecDriverParam->padAndCrcEnable          = DEFAULT_padAndCrcEnable;

    p_TgecDriverParam->debugMode                = DEFAULT_debugMode;

    p_TgecDriverParam->pauseTime                = DEFAULT_pauseTime;

//Temp    p_TgecDriverParam->imask              = DEFAULT_imask;
}

/* ........................................................................... */

static void FreeInitResources(t_Tgec *p_Tgec)
{
    /* release the driver's group hash table */
    FreeHashTable(p_Tgec->p_MulticastAddrHash);
    p_Tgec->p_MulticastAddrHash =   NULL;

    /* release the driver's individual hash table */
    FreeHashTable(p_Tgec->p_UnicastAddrHash);
    p_Tgec->p_UnicastAddrHash =     NULL;
}

/* .............................................................................. */

static void HardwareClearAddrInPaddr(t_Tgec   *p_Tgec, uint8_t paddrNum)
{
    if (paddrNum != 0)
        return;             /* At this time MAC has only one address */

    WRITE_UINT32(p_Tgec->p_MemMap->mac_addr_2, 0x0);
    WRITE_UINT32(p_Tgec->p_MemMap->mac_addr_3, 0x0);
}

/* ........................................................................... */

static void HardwareAddAddrInPaddr(t_Tgec   *p_Tgec, uint64_t *p_Addr, uint8_t paddrNum)
{
    uint32_t tmpReg32 = 0;
    uint64_t addr = *p_Addr;
    t_TgecMemMap    *p_TgecMemMap = p_Tgec->p_MemMap;

    if (paddrNum != 0)
        return;             /* At this time MAC has only one address */

//    tmpReg32 = (uint32_t)(addr);
    tmpReg32 = (uint32_t)(addr>>16);
    SwapUint32P(&tmpReg32);
    WRITE_UINT32(p_TgecMemMap->mac_addr_2, tmpReg32);

//    tmpReg32 = (uint32_t)(addr>>32);
    tmpReg32 = (uint32_t)(addr);
    SwapUint32P(&tmpReg32);
    tmpReg32 >>= 16;
    WRITE_UINT32(p_TgecMemMap->mac_addr_3, tmpReg32);
}

/*****************************************************************************/
/*                     10G MAC API routines                                  */
/*****************************************************************************/

static t_Error TgecFree(t_Handle h_Tgec)
{
    t_Tgec       *p_Tgec = (t_Tgec *)h_Tgec;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);

    if (p_Tgec->p_TgecDriverParam)
    {
        XX_Free(p_Tgec->p_TgecDriverParam);
        p_Tgec->p_TgecDriverParam = NULL;
    }
    FreeInitResources(p_Tgec);
    XX_Free (p_Tgec);

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecInit(t_Handle h_Tgec)
{

    t_Tgec                  *p_Tgec = (t_Tgec *)h_Tgec;
    t_TgecDriverParam       *p_TgecDriverParam;
    t_TgecMemMap            *p_MemMap;
    uint32_t                tmpReg32;
    uint64_t                addr;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Tgec->p_TgecDriverParam, E_INVALID_STATE);
    SANITY_CHECK_RETURN_ERROR(p_Tgec->p_MemMap, E_INVALID_HANDLE);

    CHECK_INIT_PARAMETERS(p_Tgec, CheckInitParameters);

    p_TgecDriverParam = p_Tgec->p_TgecDriverParam;
    p_MemMap = p_Tgec->p_MemMap;

    /* MAC Address */

    addr = p_Tgec->addr;

//    tmpReg32 = (uint32_t)(addr);
    tmpReg32 = (uint32_t)(addr>>16);
    SwapUint32P(&tmpReg32);
    WRITE_UINT32(p_MemMap->mac_addr_0, tmpReg32);

//    tmpReg32 = (uint32_t)(addr>>32);
    tmpReg32 = (uint32_t)(addr);
    SwapUint32P(&tmpReg32);
    tmpReg32 >>= 16;
    WRITE_UINT32(p_MemMap->mac_addr_1, tmpReg32);

    /* Config */

    tmpReg32 = 0;
    if (p_TgecDriverParam->wanModeEnable)
       tmpReg32 |= CMD_CFG_WAN_MODE;
    if (p_TgecDriverParam->promiscuousModeEnable)
       tmpReg32 |= CMD_CFG_PROMIS_EN;
    if (p_TgecDriverParam->padRemovalEnable)
       tmpReg32 |= CMD_CFG_PAD_EN;
    if (p_TgecDriverParam->crcForwardEnable)
       tmpReg32 |= CMD_CFG_CRC_FWD;
    if (p_TgecDriverParam->pauseForwardEnable)
       tmpReg32 |= CMD_CFG_PAUSE_FWD;
    if (p_TgecDriverParam->pauseIgnore)
       tmpReg32 |= CMD_CFG_PAUSE_IGNORE;
    if (p_TgecDriverParam->txAddrInsEnable)
       tmpReg32 |= CMD_CFG_TX_ADDR_INS;
    if (p_TgecDriverParam->loopbackEnable)
        tmpReg32 |= CMD_CFG_LOOPBACK_EN;
    if (p_TgecDriverParam->cmdFrameEnable)
        tmpReg32 |= CMD_CFG_CMD_FRM_EN;
    if (p_TgecDriverParam->rxErrorDiscard)
        tmpReg32 |= CMD_CFG_RX_ER_DISC;
    if (p_TgecDriverParam->phyTxenaOn)
       tmpReg32 |= CMD_CFG_PHY_TX_EN;
    if (p_TgecDriverParam->sendIdleEnable)
       tmpReg32 |= CMD_CFG_SEND_IDLE;
    if (p_TgecDriverParam->noLengthCheckEnable)
       tmpReg32 |= CMD_CFG_NO_LEN_CHK;
    if (p_TgecDriverParam->lgthCheckNostdr)
       tmpReg32 |= CMD_CFG_LEN_CHK_NOSTDR;
    if (p_TgecDriverParam->timeStampEnable)
       tmpReg32 |= CMD_CFG_EN_TIMESTAMP;
    if (p_TgecDriverParam->rxSfdAny)
       tmpReg32 |= RX_SFD_ANY;
    if (p_TgecDriverParam->rxPblFwd)
       tmpReg32 |= CMD_CFG_RX_PBL_FWD;
    if (p_TgecDriverParam->txPblFwd)
       tmpReg32 |= CMD_CFG_TX_PBL_FWD;
    WRITE_UINT32(p_MemMap->cmd_conf_ctrl, tmpReg32);

    /* Max Frame Length */
    WRITE_UINT32(p_MemMap->maxfrm, (uint32_t)p_TgecDriverParam->maxFrameLength);

    /* Pause Time */
    WRITE_UINT32(p_MemMap->pause_quant, p_TgecDriverParam->pauseTime);

    p_Tgec->p_MulticastAddrHash = AllocHashTable(HASH_TABLE_SIZE);
    if(!p_Tgec->p_MulticastAddrHash)
    {
        RETURN_ERROR(MAJOR, E_NO_MEMORY, ("allocation hash table is FAILED"));
    }

    p_Tgec->p_UnicastAddrHash = AllocHashTable(HASH_TABLE_SIZE);
    if(!p_Tgec->p_UnicastAddrHash)
    {
        FreeInitResources(p_Tgec);
        RETURN_ERROR(MAJOR, E_NO_MEMORY, ("allocation hash table is FAILED"));
    }

    XX_Free(p_TgecDriverParam);
    p_Tgec->p_TgecDriverParam = NULL;

    return E_OK;
}


/*****************************************************************************/
/*                      Tgec Configs modification functions                 */
/*****************************************************************************/

/* .............................................................................. */

static t_Error TgecConfigStatistics(t_Handle h_Tgec, bool newVal)
{
    t_Tgec *p_Tgec = (t_Tgec *)h_Tgec;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Tgec->p_TgecDriverParam, E_INVALID_STATE);

    p_Tgec->p_TgecDriverParam->statisticsEnable = newVal;

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecConfigLoopback(t_Handle h_Tgec, bool newVal)
{
    t_Tgec *p_Tgec = (t_Tgec *)h_Tgec;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Tgec->p_TgecDriverParam, E_INVALID_STATE);

    p_Tgec->p_TgecDriverParam->loopbackEnable = newVal;

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecConfigWan(t_Handle h_Tgec, bool newVal)
{
    t_Tgec *p_Tgec = (t_Tgec *)h_Tgec;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Tgec->p_TgecDriverParam, E_INVALID_STATE);

    p_Tgec->p_TgecDriverParam->wanModeEnable = newVal;

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecConfigMaxFrameLength(t_Handle h_Tgec, uint16_t newVal)
{
    t_Tgec *p_Tgec = (t_Tgec *)h_Tgec;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Tgec->p_TgecDriverParam, E_INVALID_STATE);

    p_Tgec->p_TgecDriverParam->maxFrameLength = newVal;

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecConfigPadAndCrc(t_Handle h_Tgec, bool newVal)
{
    t_Tgec *p_Tgec = (t_Tgec *)h_Tgec;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Tgec->p_TgecDriverParam, E_INVALID_STATE);

    p_Tgec->p_TgecDriverParam->padAndCrcEnable = newVal;

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecConfigHugeFrames(t_Handle h_Tgec, bool newVal)
{
    t_Tgec *p_Tgec = (t_Tgec *)h_Tgec;
    UNUSED(newVal);

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Tgec->p_TgecDriverParam, E_INVALID_STATE);

    RETURN_ERROR(MINOR, E_NOT_SUPPORTED, NO_MSG);

    return E_OK;
}

/*****************************************************************************/
/*                      Tgec Run Time API functions                         */
/*****************************************************************************/

/* .............................................................................. */

static t_Error TgecEnable(t_Handle h_Tgec,  e_CommMode mode)
{
    t_Tgec *p_Tgec = (t_Tgec *)h_Tgec;
    t_TgecMemMap       *p_MemMap ;
    uint32_t            tmpReg32 = 0;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Tgec->p_MemMap, E_INVALID_HANDLE);

    p_MemMap= (t_TgecMemMap*)(p_Tgec->p_MemMap);

    tmpReg32 = GET_UINT32(p_MemMap->cmd_conf_ctrl);

    switch (mode)
    {
        case e_COMM_MODE_NONE:
            tmpReg32 &= ~(CMD_CFG_TX_EN | CMD_CFG_RX_EN);
            break;
        case e_COMM_MODE_RX :
            tmpReg32 |= CMD_CFG_RX_EN ;
            break;
        case e_COMM_MODE_TX :
            tmpReg32 |= CMD_CFG_TX_EN ;
            break;
        case e_COMM_MODE_RX_AND_TX:
            tmpReg32 |= (CMD_CFG_TX_EN | CMD_CFG_RX_EN);
            break;
    }

    WRITE_UINT32(p_MemMap->cmd_conf_ctrl, tmpReg32);

    return E_OK;
}

/* .............................................................................. */
static t_Error TgecDisable (t_Handle h_Tgec, e_CommMode mode)
{
    t_Tgec *p_Tgec = (t_Tgec *)h_Tgec;
    t_TgecMemMap       *p_MemMap ;
    uint32_t            tmpReg32 = 0;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Tgec->p_MemMap, E_INVALID_HANDLE);

    p_MemMap= (t_TgecMemMap*)(p_Tgec->p_MemMap);

    tmpReg32 = GET_UINT32(p_MemMap->cmd_conf_ctrl);

    switch (mode)
    {
        case e_COMM_MODE_RX :
            tmpReg32 &= ~CMD_CFG_RX_EN ;
            break;
        case e_COMM_MODE_TX :
            tmpReg32 &= ~CMD_CFG_TX_EN ;
            break;
        case e_COMM_MODE_RX_AND_TX:
            tmpReg32 &= ~(CMD_CFG_TX_EN | CMD_CFG_RX_EN);
        break;
        default:
            RETURN_ERROR(MINOR, E_INVALID_SELECTION, NO_MSG);
    }

    WRITE_UINT32(p_MemMap->cmd_conf_ctrl, tmpReg32);

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecTxMacPause(t_Handle h_Tgec, uint16_t pauseTime, uint16_t exPauseTime )
{
    t_Tgec      *p_Tgec = (t_Tgec *)h_Tgec;
    uint32_t    ptv = 0 ;
    t_TgecMemMap *p_MemMap;
    UNUSED(exPauseTime);

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_STATE);
    SANITY_CHECK_RETURN_ERROR(!p_Tgec->p_TgecDriverParam, E_INVALID_STATE);
    SANITY_CHECK_RETURN_ERROR(p_Tgec->p_MemMap, E_INVALID_STATE);

    p_MemMap = (t_TgecMemMap*)(p_Tgec->p_MemMap);

    ptv = (uint32_t)pauseTime ;

    WRITE_UINT32(p_MemMap->pause_quant, ptv);

    return E_OK;
}

/* Counters handling */
/* .............................................................................. */

static t_Error TgecGetStatistics(t_Handle h_Tgec, t_FmMacStatistics *p_Statistics)
{
    t_Tgec          *p_Tgec = (t_Tgec *)h_Tgec;
    t_TgecMemMap        *p_TgecMemMap = p_Tgec->p_MemMap;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Statistics, E_NULL_POINTER);



    p_Statistics->eStatPkts64           = GET_UINT64(p_TgecMemMap->R64);
    p_Statistics->eStatPkts65to127      = GET_UINT64(p_TgecMemMap->R127);
    p_Statistics->eStatPkts128to255     = GET_UINT64(p_TgecMemMap->R255);
    p_Statistics->eStatPkts256to511     = GET_UINT64(p_TgecMemMap->R511);
    p_Statistics->eStatPkts512to1023    = GET_UINT64(p_TgecMemMap->R1023);
    p_Statistics->eStatPkts1024to1518   = GET_UINT64(p_TgecMemMap->R1518);
    p_Statistics->eStatPkts1519to1522   = GET_UINT64(p_TgecMemMap->R1519X);
/* */
    p_Statistics->eStatFragments        = GET_UINT64(p_TgecMemMap->TRFRG);
    p_Statistics->eStatJabbers          = GET_UINT64(p_TgecMemMap->TRJBR);

    p_Statistics->eStatsDropEvents      = GET_UINT64(p_TgecMemMap->RDRP);
    p_Statistics->eStatCRCAlignErrors   = GET_UINT64(p_TgecMemMap->RALN);

    p_Statistics->eStatUndersizePkts    = GET_UINT64(p_TgecMemMap->TRUND);
    p_Statistics->eStatOversizePkts     = GET_UINT64(p_TgecMemMap->TROVR);
/* Pause */
    p_Statistics->reStatPause           = GET_UINT64(p_TgecMemMap->RXPF);
    p_Statistics->teStatPause           = GET_UINT64(p_TgecMemMap->TXPF);


/* MIB II */
    p_Statistics->ifInOctets            = GET_UINT64(p_TgecMemMap->ROCT);
    p_Statistics->ifInMcastPkts         = GET_UINT64(p_TgecMemMap->RMCA);
    p_Statistics->ifInBcastPkts         = GET_UINT64(p_TgecMemMap->RBCA);
    p_Statistics->ifInPkts              = GET_UINT64(p_TgecMemMap->RUCA)
                                        + p_Statistics->ifInMcastPkts
                                        + p_Statistics->ifInBcastPkts;
    p_Statistics->ifInDiscards          = 0;
    p_Statistics->ifInErrors            = GET_UINT64(p_TgecMemMap->RERR);

    p_Statistics->ifOutOctets           = GET_UINT64(p_TgecMemMap->TOCT);
    p_Statistics->ifOutMcastPkts        = GET_UINT64(p_TgecMemMap->TMCA);
    p_Statistics->ifOutBcastPkts        = GET_UINT64(p_TgecMemMap->TBCA);
    p_Statistics->ifOutPkts             = GET_UINT64(p_TgecMemMap->TUCA);
    p_Statistics->ifOutDiscards         = 0;
    p_Statistics->ifOutErrors           = GET_UINT64(p_TgecMemMap->TERR);

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecModifyMacAddress (t_Handle h_Tgec, t_EnetAddr *p_EnetAddr)
{
    t_Tgec              *p_Tgec = (t_Tgec *)h_Tgec;
    t_TgecMemMap        *p_TgecMemMap = p_Tgec->p_MemMap;
    uint32_t              tmpReg32 = 0;
    uint64_t              addr;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_TgecMemMap, E_INVALID_HANDLE);

    /*  Initialize MAC Station Address registers (1 & 2)    */
    /*  Station address have to be swapped (big endian to little endian */

    addr = ((*(uint64_t *)p_EnetAddr) >> 16);
    p_Tgec->addr = addr;

    tmpReg32 = 0;
    tmpReg32 |= (uint32_t)(addr & 0xFFFFFFFF);
    WRITE_UINT32(p_TgecMemMap->mac_addr_0, tmpReg32);

    tmpReg32 = 0;
    tmpReg32 |= (uint32_t) (addr >> 32);

    WRITE_UINT32(p_TgecMemMap->mac_addr_1, tmpReg32);

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecResetCounters (t_Handle h_Tgec)
{
    t_Tgec *p_Tgec = (t_Tgec *)h_Tgec;
    t_TgecMemMap       *p_MemMap ;
    uint32_t            tmpReg32, cmdConfCtrl;
    int i;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Tgec->p_MemMap, E_INVALID_HANDLE);

    p_MemMap= (t_TgecMemMap*)(p_Tgec->p_MemMap);

    cmdConfCtrl = GET_UINT32(p_MemMap->cmd_conf_ctrl);

    cmdConfCtrl |= CMD_CFG_STAT_CLR;

    WRITE_UINT32(p_MemMap->cmd_conf_ctrl, cmdConfCtrl);

    for (i=0; i<1000; i++)
    {
        tmpReg32 = GET_UINT32(p_MemMap->cmd_conf_ctrl);
        if (!(tmpReg32 & CMD_CFG_STAT_CLR))
            break;
    }

    cmdConfCtrl &= ~CMD_CFG_STAT_CLR;
    WRITE_UINT32(p_MemMap->cmd_conf_ctrl, cmdConfCtrl);

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecDelExactMatchMacAddress(t_Handle h_Tgec, t_EnetAddr *p_EthAddr)
{
    t_Tgec   *p_Tgec = (t_Tgec *) h_Tgec;
    uint64_t  ethAddr;
    uint8_t   paddrNum;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_Tgec->p_MemMap, E_INVALID_HANDLE);

    ethAddr = ((*(uint64_t *)p_EthAddr) >> 16);

    /* Find used PADDR containing this address */
    for (paddrNum = 0; paddrNum < TGEC_NUM_OF_PADDRS; paddrNum++)
    {
        if ((p_Tgec->indAddrRegUsed[paddrNum]) &&
            (p_Tgec->paddr[paddrNum] == ethAddr))
        {
            /* mark this PADDR as not used */
            p_Tgec->indAddrRegUsed[paddrNum] = FALSE;
            /* clear in hardware */
            HardwareClearAddrInPaddr(p_Tgec, paddrNum);
            p_Tgec->numOfIndAddrInRegs--;

            return E_OK;
        }
    }

    RETURN_ERROR(MAJOR, E_NOT_FOUND, NO_MSG);
}

/* .............................................................................. */

static t_Error TgecAddExactMatchMacAddress(t_Handle h_Tgec, t_EnetAddr *p_EthAddr)
{
    t_Tgec   *p_Tgec = (t_Tgec *) h_Tgec;
    uint64_t  ethAddr;
    uint8_t   paddrNum;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);

    ethAddr = ((*(uint64_t *)p_EthAddr) >> 16);

    if (ethAddr & GROUP_ADDRESS)
        /* Multicast address has no effect in PADDR */
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Multicast address"));

    /* Make sure no PADDR contains this address */
    for (paddrNum = 0; paddrNum < TGEC_NUM_OF_PADDRS; paddrNum++)
    {
        if (p_Tgec->indAddrRegUsed[paddrNum])
        {
            if (p_Tgec->paddr[paddrNum] == ethAddr)
            {
                RETURN_ERROR(MAJOR, E_ALREADY_EXISTS, NO_MSG);
            }
        }
    }

    /* Find first unused PADDR */
    for (paddrNum = 0; paddrNum < TGEC_NUM_OF_PADDRS; paddrNum++)
    {
        if (!(p_Tgec->indAddrRegUsed[paddrNum]))
        {
            /* mark this PADDR as used */
            p_Tgec->indAddrRegUsed[paddrNum] = TRUE;
            /* store address */
            p_Tgec->paddr[paddrNum] = ethAddr;

            /* put in hardware */
            HardwareAddAddrInPaddr(p_Tgec, &ethAddr, paddrNum);
            p_Tgec->numOfIndAddrInRegs++;

            return E_OK;
        }
    }

    /* No free PADDR */
    RETURN_ERROR(MAJOR, E_FULL, NO_MSG);
}

/* .............................................................................. */

static t_Error TgecAddHashMacAddress(t_Handle h_Tgec, t_EnetAddr *p_EthAddr)
{
    t_Tgec          *p_Tgec = (t_Tgec *)h_Tgec;
    t_TgecMemMap    *p_TgecMemMap = p_Tgec->p_MemMap;
    t_EthHashEntry  *p_HashEntry;
    uint32_t        crc;
    uint32_t        hash;
    uint64_t        ethAddr;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_TgecMemMap, E_INVALID_HANDLE);

    ethAddr = ((*(uint64_t *)p_EthAddr) >> 16);

    if (!(ethAddr & GROUP_ADDRESS))
        /* Unicast addresses not supported in hash */
        RETURN_ERROR(MAJOR, E_NOT_SUPPORTED, ("Unicast Address"));

    /* CRC calculation */
    GET_MAC_ADDR_CRC(ethAddr, crc);
    crc = MIRROR_32(crc);

    hash = (crc >> HASH_CTRL_MCAST_SHIFT) & HASH_ADDR_MASK;        /* Take 9 MSB bits */

    /* Create element to be added to the driver hash table */
    p_HashEntry = (t_EthHashEntry *)XX_Malloc(sizeof(t_EthHashEntry));
    p_HashEntry->addr = ethAddr;
    INIT_LIST(&p_HashEntry->node);

    LIST_AddToTail(&(p_HashEntry->node), &(p_Tgec->p_MulticastAddrHash->p_Lsts[hash]));
    WRITE_UINT32(p_TgecMemMap->hashtable_ctrl, (hash | HASH_CTRL_MCAST_EN));

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecDelHashMacAddress(t_Handle h_Tgec, t_EnetAddr *p_EthAddr)
{
    t_Tgec          *p_Tgec = (t_Tgec *)h_Tgec;
    t_TgecMemMap    *p_TgecMemMap = p_Tgec->p_MemMap;
    t_EthHashEntry  *p_HashEntry = NULL;
    t_List          *p_Pos;
    uint32_t        crc;
    uint32_t        hash;
    uint64_t        ethAddr;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_TgecMemMap, E_INVALID_HANDLE);

    ethAddr = ((*(uint64_t *)p_EthAddr) >> 16);

    /* CRC calculation */
    GET_MAC_ADDR_CRC(ethAddr, crc);
    crc = MIRROR_32(crc);

    hash = (crc >> HASH_CTRL_MCAST_SHIFT) & HASH_ADDR_MASK;        /* Take 9 MSB bits */

    LIST_FOR_EACH(p_Pos, &(p_Tgec->p_MulticastAddrHash->p_Lsts[hash]))
    {

        p_HashEntry = ETH_HASH_ENTRY_OBJ(p_Pos);
        if(p_HashEntry->addr == ethAddr)
        {
            LIST_DelAndInit(&p_HashEntry->node);
            XX_Free(p_HashEntry);
            break;
        }
    }
    if(LIST_IsEmpty(&p_Tgec->p_MulticastAddrHash->p_Lsts[hash]))
        WRITE_UINT32(p_TgecMemMap->hashtable_ctrl, (hash & ~HASH_CTRL_MCAST_EN));

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecGetId(t_Handle h_Tgec, uint32_t *macId)
{
    t_Tgec              *p_Tgec = (t_Tgec *)h_Tgec;
    t_TgecMemMap        *p_TgecMemMap = p_Tgec->p_MemMap;
UNUSED(macId);

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_Tgec->p_TgecDriverParam, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_TgecMemMap, E_INVALID_HANDLE);

    RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("TgecGetId Not Supported"));

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecGetVersion(t_Handle h_Tgec, uint32_t *macVersion)
{
    t_Tgec              *p_Tgec = (t_Tgec *)h_Tgec;
    t_TgecMemMap        *p_TgecMemMap = p_Tgec->p_MemMap;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_Tgec->p_TgecDriverParam, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_TgecMemMap, E_INVALID_HANDLE);

    *macVersion = GET_UINT32(p_TgecMemMap->tgec_id);

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecSetPromiscuous(t_Handle h_Tgec, bool newVal)
{
    t_Tgec       *p_Tgec = (t_Tgec *)h_Tgec;
    t_TgecMemMap *p_TgecMemMap = p_Tgec->p_MemMap;
    uint32_t     tmpReg32;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_Tgec->p_TgecDriverParam, E_INVALID_HANDLE);

    tmpReg32 = GET_UINT32(p_TgecMemMap->cmd_conf_ctrl);

    if (newVal)
        tmpReg32 |= CMD_CFG_PROMIS_EN;
    else
        tmpReg32 &= ~CMD_CFG_PROMIS_EN;

    WRITE_UINT32(p_TgecMemMap->cmd_conf_ctrl, tmpReg32);

    return E_OK;
}

/* .............................................................................. */

static t_Error TgecAdjustLink(t_Handle h_Tgec, e_EnetSpeed speed, bool fullDuplex)
{
    t_Tgec       *p_Tgec = (t_Tgec *)h_Tgec;
    /*t_TgecMemMap *p_TgecMemMap = p_Tgec->p_MemMap;
    uint32_t     tmpReg32;*/

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_Tgec->p_TgecDriverParam, E_INVALID_HANDLE);

    if (!fullDuplex)
        RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("half-duplex"));

RETURN_ERROR(MINOR, E_NOT_SUPPORTED, NO_MSG);
    return E_OK;
}

/* .............................................................................. */

static t_Error TgecSetExcpetions(t_Handle h_Tgec, e_FmMacExceptions ex)
{
    t_Tgec              *p_Tgec = (t_Tgec *)h_Tgec;

    SANITY_CHECK_RETURN_ERROR(p_Tgec, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_Tgec->p_TgecDriverParam, E_INVALID_HANDLE);
UNUSED(ex);

    RETURN_ERROR(MINOR, E_NOT_SUPPORTED, NO_MSG);
}

/* .............................................................................. */

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
static t_Error TgecDumpRegs(t_Handle h_Tgec)
{
    t_Tgec    *p_Tgec = (t_Tgec *)h_Tgec;

    DECLARE_DUMP;

    if (p_Tgec->p_MemMap)
    {
        DUMP_TITLE(p_Tgec->p_MemMap, ("10G MAC %d: ", p_Tgec->macId));
    }

    return E_OK;
}
#endif /* (defined(DEBUG_ERRORS) && ... */

/*****************************************************************************/
/*                      Tgec Config  Main Entry                             */
/*****************************************************************************/

static void InitFmMacControllerDriver(t_FmMacControllerDriver *p_FmMacControllerDriver)
{
    p_FmMacControllerDriver->f_FM_MAC_Init                      = TgecInit;
    p_FmMacControllerDriver->f_FM_MAC_Free                      = TgecFree;

    p_FmMacControllerDriver->f_FM_MAC_ConfigStatistics          = TgecConfigStatistics;
    p_FmMacControllerDriver->f_FM_MAC_ConfigLoopback            = TgecConfigLoopback;
    p_FmMacControllerDriver->f_FM_MAC_ConfigMaxFrameLength      = TgecConfigMaxFrameLength;

    p_FmMacControllerDriver->f_FM_MAC_ConfigWan                 = TgecConfigWan;

    p_FmMacControllerDriver->f_FM_MAC_ConfigPadAndCrc           = TgecConfigPadAndCrc;
    p_FmMacControllerDriver->f_FM_MAC_ConfigHalfDuplex          = NULL; /* half-duplex is not supported in xgec */
    p_FmMacControllerDriver->f_FM_MAC_ConfigHugeFrames          = TgecConfigHugeFrames;

    p_FmMacControllerDriver->f_FM_MAC_SetExceptions             = TgecSetExcpetions;

    p_FmMacControllerDriver->f_FM_MAC_SetPromiscuous            = TgecSetPromiscuous;
    p_FmMacControllerDriver->f_FM_MAC_AdjustLink                = TgecAdjustLink;

    p_FmMacControllerDriver->f_FM_MAC_Enable                    = TgecEnable;
    p_FmMacControllerDriver->f_FM_MAC_Disable                   = TgecDisable;
    p_FmMacControllerDriver->f_FM_MAC_Restart                   = NULL; /* TgecRestart; Not Implemented */

    p_FmMacControllerDriver->f_FM_MAC_TxMacPause                = TgecTxMacPause;

    p_FmMacControllerDriver->f_FM_MAC_ResetCounters             = TgecResetCounters;
    p_FmMacControllerDriver->f_FM_MAC_GetStatistics             = TgecGetStatistics;

    p_FmMacControllerDriver->f_FM_MAC_ModifyMacAddr             = TgecModifyMacAddress;
    p_FmMacControllerDriver->f_FM_MAC_AddHashMacAddr            = TgecAddHashMacAddress;
    p_FmMacControllerDriver->f_FM_MAC_RemoveHashMacAddr         = TgecDelHashMacAddress;
    p_FmMacControllerDriver->f_FM_MAC_AddExactMatchMacAddr      = TgecAddExactMatchMacAddress;
    p_FmMacControllerDriver->f_FM_MAC_RemovelExactMatchMacAddr  = TgecDelExactMatchMacAddress;
    p_FmMacControllerDriver->f_FM_MAC_GetId                     = TgecGetId;
    p_FmMacControllerDriver->f_FM_MAC_GetVersion                = TgecGetVersion;

    p_FmMacControllerDriver->f_FM_MAC_MII_WritePhyReg           = TGEC_MII_WritePhyReg;
    p_FmMacControllerDriver->f_FM_MAC_MII_ReadPhyReg            = TGEC_MII_ReadPhyReg;

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
    p_FmMacControllerDriver->f_FM_MAC_DumpRegs                  = TgecDumpRegs;
#endif /* (defined(DEBUG_ERRORS) && ... */
}

t_Handle  TGEC_Config(t_FmMacParams *p_FmMacParam)
{
    t_Tgec                  *p_Tgec;
    t_TgecDriverParam       *p_TgecDriverParam;
    uint64_t                baseAddr = p_FmMacParam->baseAddr;

    SANITY_CHECK_RETURN_VALUE(p_FmMacParam, E_NULL_POINTER, NULL);

    /* allocate memory for the UCC GETH data structure. */
    p_Tgec = (t_Tgec *) XX_Malloc(sizeof(t_Tgec));
    if (!p_Tgec)
    {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("10G MAC driver structure"));
        return NULL;
    }
    /* Zero out * p_Tgec */
    memset( p_Tgec, 0, sizeof(t_Tgec));
    InitFmMacControllerDriver(&p_Tgec->fmMacControllerDriver);

    /* allocate memory for the 10G MAC driver parameters data structure. */
    p_TgecDriverParam = (t_TgecDriverParam *) XX_Malloc(sizeof(t_TgecDriverParam));
    if (!p_TgecDriverParam)
    {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("10G MAC driver parameters"));
        TgecFree(p_Tgec);
        return NULL;
    }
    /* Zero out */
    memset(p_TgecDriverParam, 0, sizeof(t_TgecDriverParam));

    /* Plant parameter structure pointer */
    p_Tgec->p_TgecDriverParam = p_TgecDriverParam;

    SetDefaultParam(p_TgecDriverParam);

    p_Tgec->h_App    = p_FmMacParam->h_App ;
    p_Tgec->addr  = ((*(uint64_t *)p_FmMacParam->addr) >> 16);
    p_Tgec->p_MemMap = CAST_UINT64_TO_POINTER_TYPE(t_TgecMemMap, baseAddr);
    p_Tgec->p_MiiMemMap  = CAST_UINT64_TO_POINTER_TYPE(t_TgecMiiAccessMemMap, (baseAddr + TGEC_TO_MII_OFFSET));
    p_Tgec->enetMode = p_FmMacParam->enetMode;
    p_Tgec->macId    = p_FmMacParam->macId;

    return p_Tgec;
}
