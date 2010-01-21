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
 @File          fm_cc.c

 @Description   FM CC ...
*//***************************************************************************/
#include "std_ext.h"
#include "error_ext.h"
#include "string_ext.h"
#include "debug_ext.h"
#include "fm_pcd_ext.h"
#include "fm_muram_ext.h"

#include "fm_hc.h"
#include "fm_pcd.h"


t_Error FmPcdCcTreeTryLock(t_Handle h_FmPcdCcTree)
{
    TRY_LOCK_RET_ERR(((t_FmPcdCcTree *)h_FmPcdCcTree)->lock);
    return E_OK;
}
void FmPcdCcTreeReleaseLock(t_Handle h_FmPcdCcTree)
{
    RELEASE_LOCK(((t_FmPcdCcTree *)h_FmPcdCcTree)->lock);
}

static void EnqueueAdditionalInfoToRelevantLst(t_List *p_CcNode, t_CcNodeInfo *p_CcInfo)
{
    unsigned long   intFlags;

    intFlags = XX_DisableAllIntr();
    LIST_AddToTail(&p_CcInfo->h_Node, p_CcNode);
    XX_RestoreAllIntr(intFlags);
}

static void CreateNodeInfo(t_List *p_List, uint32_t info)
{
        t_CcNodeInfo *p_CcInfo;

        p_CcInfo = (t_CcNodeInfo *)XX_Malloc(sizeof(t_CcNodeInfo));
        if(!p_CcInfo)
            REPORT_ERROR(MAJOR, E_NO_MEMORY, ("No memory"));
        memset(p_CcInfo, 0, sizeof(t_CcNodeInfo));
        INIT_LIST(&p_CcInfo->h_Node);
        p_CcInfo->nextCcNodeInfo = (uint32_t)info;
        EnqueueAdditionalInfoToRelevantLst(p_List, p_CcInfo);
}

static t_CcNodeInfo * FindNodeInfoAccIndex(t_List *p_List, uint16_t indx)
{
    t_CcNodeInfo   *p_CcNodeInfo = NULL;
    t_List      *p_Pos;

    LIST_FOR_EACH(p_Pos, p_List)
    {
        p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos);
        if((p_CcNodeInfo->nextCcNodeInfo >> 16) == indx)
            return   p_CcNodeInfo;
    }
    return NULL;
}

static t_CcNodeInfo * FindNodeInfoAccId(t_List *p_List, uint16_t indx)
{
    t_CcNodeInfo   *p_CcNodeInfo = NULL;
    t_List      *p_Pos;

    LIST_FOR_EACH(p_Pos, p_List)
    {
        p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos);
        if(((uint16_t)p_CcNodeInfo->nextCcNodeInfo) == indx)
            return   p_CcNodeInfo;
    }
    return NULL;
}

static t_CcNodeInfo * DequeueAdditionalInfoFromRelevantLst(t_List *p_List)
{
    t_CcNodeInfo   *p_CcNodeInfo = NULL;
    unsigned long        intFlags;

    intFlags = XX_DisableAllIntr();
    if (!LIST_IsEmpty(p_List))
    {
        p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_List->p_Next);
        LIST_DelAndInit(&p_CcNodeInfo->h_Node);
    }
    XX_RestoreAllIntr(intFlags);
    return p_CcNodeInfo;
}

static void ReleaseLst(t_List *p_List)
{
    t_CcNodeInfo   *p_CcNodeInfo = NULL;

    if(!LIST_IsEmpty(p_List))
    {
        p_CcNodeInfo = DequeueAdditionalInfoFromRelevantLst(p_List);
        while (p_CcNodeInfo)
        {
            XX_Free(p_CcNodeInfo);
            p_CcNodeInfo = DequeueAdditionalInfoFromRelevantLst(p_List);
        }
    }
    LIST_DelAndInit(p_List);
}

static void ReleaseNodeHandler(t_FmPcdCcNode *p_FmPcdCcNode, t_FmPcdCc *p_FmPcdCc)
{

    if(p_FmPcdCcNode)
    {
            if(p_FmPcdCcNode->p_GlblMask)
            {
                XX_Free(p_FmPcdCcNode->p_GlblMask);
                p_FmPcdCcNode->p_GlblMask = NULL;
            }
            if(p_FmPcdCcNode->h_KeysMatchTable)
            {
                FM_MURAM_FreeMem(p_FmPcdCc->h_FmMuram,p_FmPcdCcNode->h_KeysMatchTable);
                p_FmPcdCcNode->h_KeysMatchTable = NULL;
            }
            if(p_FmPcdCcNode->h_AdTable)
            {
                FM_MURAM_FreeMem(p_FmPcdCc->h_FmMuram,p_FmPcdCcNode->h_AdTable);
                p_FmPcdCcNode->h_AdTable = NULL;
            }

            ReleaseLst(&p_FmPcdCcNode->ccNextNodesLst);
            ReleaseLst(&p_FmPcdCcNode->ccPrevNodesLst);
            ReleaseLst(&p_FmPcdCcNode->ccTreeIdLst);
            ReleaseLst(&p_FmPcdCcNode->ccTreesLst);


            XX_Free(p_FmPcdCcNode);
    }
}

static t_CcNodeInfo * FindNodeInfoAccIdAndAddToRetLst(t_List *p_List, uint16_t nodeId,  t_List *p_ReturnList)
{
    t_CcNodeInfo   *p_CcNodeInfo = NULL;
    t_List      *p_Pos;

    LIST_FOR_EACH(p_Pos, p_List)
    {
        p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos);
        if(((uint16_t)p_CcNodeInfo->nextCcNodeInfo) == nodeId)
            CreateNodeInfo(p_ReturnList, p_CcNodeInfo->nextCcNodeInfo);
    }
    return NULL;
}

static void  UpdateNodeOwner(t_FmPcd  *p_FmPcd, uint16_t nodeId, bool add)
{
    ASSERT_COND(nodeId < FM_PCD_MAX_NUM_OF_CC_NODES);
    if(add)
        p_FmPcd->p_FmPcdCc->ccNodeArrayEntry[nodeId].owners++;
    else
    {
        ASSERT_COND(p_FmPcd->p_FmPcdCc->ccNodeArrayEntry[nodeId].owners);
        p_FmPcd->p_FmPcdCc->ccNodeArrayEntry[nodeId].owners--;
    }
}

static t_Handle GetNodeHandler(t_FmPcd *p_FmPcd, uint16_t nodeId)
{
    ASSERT_COND(nodeId < FM_PCD_MAX_NUM_OF_CC_NODES);
    return p_FmPcd->p_FmPcdCc->ccNodeArrayEntry[nodeId].p_FmPcdCcNode;

}

static void SetNodeHandler(t_Handle h_FmPcdCc, uint16_t nodeId, t_Handle p_FmPcdCcNode)
{
    t_FmPcdCc *p_FmPcdCc = (t_FmPcdCc*)h_FmPcdCc;

    ASSERT_COND(!p_FmPcdCc->ccNodeArrayEntry[nodeId].p_FmPcdCcNode);
    p_FmPcdCc->ccNodeArrayEntry[nodeId].p_FmPcdCcNode = p_FmPcdCcNode;
}

static t_Handle FmPcdCcGetTreeHandler(t_Handle h_FmPcd, uint8_t treeId)
{

    ASSERT_COND(treeId < FM_PCD_MAX_NUM_OF_CC_TREES);
    return ((t_FmPcd*)h_FmPcd)->p_FmPcdCc->ccTreeArrayEntry[treeId].p_FmPcdCcTree;
}

static void SetTreeHandler(t_Handle h_FmPcdCc, uint8_t treeId, t_Handle p_FmPcdCcTree)
{
    t_FmPcdCc *p_FmPcdCc = (t_FmPcdCc*)h_FmPcdCc;

    ASSERT_COND(!p_FmPcdCc->ccTreeArrayEntry[treeId].p_FmPcdCcTree);
    p_FmPcdCc->ccTreeArrayEntry[treeId].p_FmPcdCcTree = p_FmPcdCcTree;
}


static uint8_t GetTreeOwners(t_FmPcd *p_FmPcd, uint8_t treeId)
{
    ASSERT_COND(treeId < FM_PCD_MAX_NUM_OF_CC_TREES);
    return p_FmPcd->p_FmPcdCc->ccTreeArrayEntry[treeId].owners;
}

static uint8_t GetNodeOwners(t_FmPcd *p_FmPcd, uint16_t nodeId)
{
    ASSERT_COND(nodeId < FM_PCD_MAX_NUM_OF_CC_NODES);
    return p_FmPcd->p_FmPcdCc->ccNodeArrayEntry[nodeId].owners;
}


static t_Error OccupyNodeId(t_FmPcd *p_FmPcd, uint16_t *nodeId)
{
    uint16_t i = 0;

    TRY_LOCK_RET_ERR(p_FmPcd->lock);
    for(i = 0; i < FM_PCD_MAX_NUM_OF_CC_NODES; i++)
    {
        if(!p_FmPcd->p_FmPcdCc->ccNodeArrayEntry[i].occupied && !p_FmPcd->p_FmPcdCc->ccNodeArrayEntry[i].p_FmPcdCcNode)
        {
            *nodeId = i;
            p_FmPcd->p_FmPcdCc->ccNodeArrayEntry[i].occupied = TRUE;
            RELEASE_LOCK(p_FmPcd->lock)
           return E_OK;
        }

    }
    RELEASE_LOCK(p_FmPcd->lock)
    RETURN_ERROR(MAJOR, E_NOT_AVAILABLE, ("asked for more nodes in CC than MAX"))  ;
}

static t_Error OccupyTreeId(t_FmPcd *p_FmPcd, uint8_t *treeId)
{
    uint16_t i = 0;

    TRY_LOCK_RET_ERR(p_FmPcd->lock);
    for(i = 0; i < FM_PCD_MAX_NUM_OF_CC_NODES; i++)
    {
        if(!p_FmPcd->p_FmPcdCc->ccTreeArrayEntry[i].occupied && !p_FmPcd->p_FmPcdCc->ccTreeArrayEntry[i].p_FmPcdCcTree)
        {
            *treeId = (uint8_t)i;
            p_FmPcd->p_FmPcdCc->ccTreeArrayEntry[i].occupied = TRUE;
            RELEASE_LOCK(p_FmPcd->lock)
           return E_OK;
        }

    }
    RELEASE_LOCK(p_FmPcd->lock)
    RETURN_ERROR(MAJOR, E_NOT_AVAILABLE, ("asked for more nodes in CC than MAX"))  ;
}



static bool CcNodeIsValid(t_FmPcd *p_FmPcd, uint16_t nodeId)
{
    t_FmPcdCc *p_FmPcdCc = p_FmPcd->p_FmPcdCc;

    ASSERT_COND(nodeId < FM_PCD_MAX_NUM_OF_CC_NODES);

    if(!p_FmPcdCc->ccNodeArrayEntry[nodeId].occupied ||
        !p_FmPcdCc->ccNodeArrayEntry[nodeId].p_FmPcdCcNode)
        return FALSE;
    else
        return TRUE;
}

static void  GetCcExtractKeySize(uint8_t parseCodeRealSize, uint8_t *parseCodeCcSize)
{
    if((parseCodeRealSize > 0) && (parseCodeRealSize < 2))
        *parseCodeCcSize = 1;
    else if(parseCodeRealSize == 2)
        *parseCodeCcSize = 2;
    else if((parseCodeRealSize > 2)    && (parseCodeRealSize <= 4))
        *parseCodeCcSize = 4;
    else if((parseCodeRealSize > 4)    && (parseCodeRealSize <= 8))
        *parseCodeCcSize = 8;
    else if((parseCodeRealSize > 8)    && (parseCodeRealSize <= 16))
        *parseCodeCcSize = 16;
    else if((parseCodeRealSize  > 16)  && (parseCodeRealSize <= 24))
        *parseCodeCcSize = 24;
    else if((parseCodeRealSize  > 24)  && (parseCodeRealSize <= 32))
        *parseCodeCcSize = 32;
    else if((parseCodeRealSize  > 32)  && (parseCodeRealSize <= 40))
        *parseCodeCcSize = 40;
    else if((parseCodeRealSize  > 40)  && (parseCodeRealSize <= 48))
        *parseCodeCcSize = 48;
    else if((parseCodeRealSize  > 48)  && (parseCodeRealSize <= 56))
        *parseCodeCcSize = 56;
    else
        *parseCodeCcSize = 0;
}

static void  GetSizeHeaderField(e_NetHeaderType hdr,t_FmPcdFields field,uint8_t *parseCodeRealSize)
{
    switch(hdr)
    {
        case (HEADER_TYPE_ETH):
            switch(field.eth)
            {
                case(NET_HEADER_FIELD_ETH_DA):
                    *parseCodeRealSize = 6;
                    break;
                case(NET_HEADER_FIELD_ETH_SA):
                    *parseCodeRealSize = 6;
                    break;
                case(NET_HEADER_FIELD_ETH_TYPE):
                    *parseCodeRealSize = 2;
                    break;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported1"));
                    *parseCodeRealSize = CC_SIZE_ILLEGAL;
                    break;
            }
        break;
        case(HEADER_TYPE_PPPoE):
            switch(field.pppoe)
            {
                case(NET_HEADER_FIELD_PPPoE_PID):
                    *parseCodeRealSize = 2;
                    break;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported1"));
                    *parseCodeRealSize = CC_SIZE_ILLEGAL;
                    break;
            }
        break;
        case (HEADER_TYPE_VLAN):
            switch(field.vlan)
            {
               case(NET_HEADER_FIELD_VLAN_TCI):
                    *parseCodeRealSize = 2;
                    break;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported2"));
                    *parseCodeRealSize = CC_SIZE_ILLEGAL;
                    break;
            }
        break;
        case (HEADER_TYPE_MPLS):
            switch(field.mpls)
            {
                case(NET_HEADER_FIELD_MPLS_LABEL_STACK):
                    *parseCodeRealSize = 4;
                    break;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported3"));
                    *parseCodeRealSize = CC_SIZE_ILLEGAL;
                    break;
            }
        break;
        case (HEADER_TYPE_IPv4):
            switch(field.ipv4)
            {
                case(NET_HEADER_FIELD_IPv4_DST_IP):
                case(NET_HEADER_FIELD_IPv4_SRC_IP):
                    *parseCodeRealSize = 4;
                    break;
                case(NET_HEADER_FIELD_IPv4_TOS):
                case(NET_HEADER_FIELD_IPv4_PROTO):
                    *parseCodeRealSize = 1;
                    break;
                case(NET_HEADER_FIELD_IPv4_DST_IP | NET_HEADER_FIELD_IPv4_SRC_IP):
                    *parseCodeRealSize = 8;
                    break;
                case(NET_HEADER_FIELD_IPv4_TTL):
                    *parseCodeRealSize = 1;
                    break;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported4"));
                    *parseCodeRealSize = CC_SIZE_ILLEGAL;
                    break;
            }
        break;
        case (HEADER_TYPE_IPv6):
            switch(field.ipv6)
            {
                case(NET_HEADER_FIELD_IPv6_VER | NET_HEADER_FIELD_IPv6_FL | NET_HEADER_FIELD_IPv6_TC):
                   *parseCodeRealSize = 4;
                    break;
                case(NET_HEADER_FIELD_IPv6_NEXT_HDR):
                case(NET_HEADER_FIELD_IPv6_HOP_LIMIT):
                   *parseCodeRealSize = 1;
                    break;
                case(NET_HEADER_FIELD_IPv6_DST_IP):
                case(NET_HEADER_FIELD_IPv6_SRC_IP):
                   *parseCodeRealSize = 16;
                    break;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported5"));
                    *parseCodeRealSize = CC_SIZE_ILLEGAL;
                    break;
            }
        break;
        case (HEADER_TYPE_GRE):
            switch(field.gre)
            {
                case(NET_HEADER_FIELD_GRE_TYPE):
                   *parseCodeRealSize = 2;
                    break;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported6"));
                    *parseCodeRealSize = CC_SIZE_ILLEGAL;
                    break;
            }
        break;
        case (HEADER_TYPE_MINENCAP):
            switch(field.minencap)
            {
                case(NET_HEADER_FIELD_MINENCAP_TYPE):
                   *parseCodeRealSize = 1;
                    break;
                case(NET_HEADER_FIELD_MINENCAP_DST_IP):
                 case(NET_HEADER_FIELD_MINENCAP_SRC_IP):
                  *parseCodeRealSize = 4;
                    break;
                 case(NET_HEADER_FIELD_MINENCAP_SRC_IP | NET_HEADER_FIELD_MINENCAP_DST_IP):
                  *parseCodeRealSize = 8;
                    break;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported7"));
                    *parseCodeRealSize = CC_SIZE_ILLEGAL;
                    break;
            }
        break;
        case (HEADER_TYPE_TCP):
            switch(field.tcp)
            {
                case(NET_HEADER_FIELD_TCP_PORT_SRC):
                case(NET_HEADER_FIELD_TCP_PORT_DST):
                   *parseCodeRealSize = 2;
                    break;
                 case(NET_HEADER_FIELD_TCP_PORT_SRC | NET_HEADER_FIELD_TCP_PORT_DST):
                  *parseCodeRealSize = 4;
                    break;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported8"));
                    *parseCodeRealSize = CC_SIZE_ILLEGAL;
                    break;
            }
        break;
        case (HEADER_TYPE_UDP):
            switch(field.udp)
            {
                case(NET_HEADER_FIELD_UDP_PORT_SRC):
                case(NET_HEADER_FIELD_UDP_PORT_DST):
                   *parseCodeRealSize = 2;
                    break;
                 case(NET_HEADER_FIELD_UDP_PORT_SRC | NET_HEADER_FIELD_UDP_PORT_DST):
                  *parseCodeRealSize = 4;
                    break;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported9"));
                    *parseCodeRealSize = CC_SIZE_ILLEGAL;
                    break;
            }
            break;
       default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported10"));
                    *parseCodeRealSize = CC_SIZE_ILLEGAL;
                    break;
    }
}



static void ReleaseNode(t_FmPcdCc *p_FmPcdCc, uint16_t nodeId)
{
    t_FmPcdCcNode *p_FmPcdCcNode;

    if(p_FmPcdCc->ccNodeArrayEntry[nodeId].p_FmPcdCcNode)
    {
            p_FmPcdCcNode = (t_FmPcdCcNode *)p_FmPcdCc->ccNodeArrayEntry[nodeId].p_FmPcdCcNode;
            ReleaseNodeHandler(p_FmPcdCcNode,p_FmPcdCc);
            p_FmPcdCc->ccNodeArrayEntry[nodeId].p_FmPcdCcNode = NULL;
    }
    p_FmPcdCc->ccNodeArrayEntry[nodeId].occupied = FALSE;
}

static void ReleaseTreeHandler(t_FmPcdCcTree *p_FmPcdTreeNode, t_FmPcdCc *p_FmPcdCc)
{

    if(p_FmPcdTreeNode)
    {
        if(p_FmPcdTreeNode->ccTreeBaseAddr)
        {
            FM_MURAM_FreeMem(p_FmPcdCc->h_FmMuram, CAST_UINT64_TO_POINTER(p_FmPcdTreeNode->ccTreeBaseAddr));
            p_FmPcdTreeNode->ccTreeBaseAddr = 0;
        }


        ReleaseLst(&p_FmPcdTreeNode->ccNextNodesLst);
        ReleaseLst(&p_FmPcdTreeNode->fmPortsLst);

        XX_Free(p_FmPcdTreeNode);
        }
}

static void ReleaseTree(t_FmPcdCc *p_FmPcdCc, uint8_t treeId)
{
    t_FmPcdCcTree *p_FmPcdCcTree;

    if(p_FmPcdCc->ccTreeArrayEntry[treeId].p_FmPcdCcTree)
    {
            p_FmPcdCcTree = p_FmPcdCc->ccTreeArrayEntry[treeId].p_FmPcdCcTree;
            ReleaseTreeHandler(p_FmPcdCcTree,p_FmPcdCc);
            p_FmPcdCc->ccTreeArrayEntry[treeId].p_FmPcdCcTree = NULL;
    }
    p_FmPcdCc->ccTreeArrayEntry[treeId].occupied = FALSE;
}


static t_Error ValidateNextEngineParams(t_Handle h_FmPcd, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams, t_NextEngineParamsInfo *p_NextEngineParamsInfo)
{
    uint16_t                    absoluteProfileId;
    t_Error                     err = E_OK;
    uint8_t                     relativeSchemeId;

    p_NextEngineParamsInfo->fmPcdEngine = e_FM_PCD_DONE;

     switch(p_FmPcdCcNextEngineParams->nextEngine)
    {
        case(e_FM_PCD_DONE):
            if(p_FmPcdCcNextEngineParams->params.enqueueParams.ctrlFlow &&
               !p_FmPcdCcNextEngineParams->params.enqueueParams.fqidForCtrlFlow)
                RETURN_ERROR(MAJOR, E_INVALID_STATE, ("not defined fqid for control flow for BMI next engine "));
            if(p_FmPcdCcNextEngineParams->params.enqueueParams.fqidForCtrlFlow & ~0x00FFFFFF)
                RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("fqidForCtrlFlow must be between 1 and 2^24-1"));
            p_NextEngineParamsInfo->fmPcdEngine = e_FM_PCD_DONE;
            break;
        case(e_FM_PCD_KG):
            relativeSchemeId = FmPcdKgGetRelativeSchemeId(h_FmPcd, (uint8_t)(CAST_POINTER_TO_UINT32(p_FmPcdCcNextEngineParams->params.kgParams.h_DirectScheme)-1));
            if(relativeSchemeId == FM_PCD_KG_NUM_OF_SCHEMES)
                RETURN_ERROR(MAJOR, E_NOT_IN_RANGE, NO_MSG);

            if(!FmPcdKgIsSchemeValidSw(h_FmPcd, relativeSchemeId))
                RETURN_ERROR(MAJOR, E_INVALID_STATE, ("not valid schemeIndex in KG next engine param"));
            if(!KgIsSchemeAlwaysDirect(h_FmPcd, relativeSchemeId))
                RETURN_ERROR(MAJOR, E_INVALID_STATE, ("CC Node may point only to a scheme that is always direct."));
            p_NextEngineParamsInfo->fmPcdEngine = e_FM_PCD_KG;
            break;
        case(e_FM_PCD_PLCR):
            if(p_FmPcdCcNextEngineParams->params.plcrParams.ctrlFlow)
            {
                /* if private policer profile, it may be uninitialized yet, therefor no checks are done at this stage */
                if(p_FmPcdCcNextEngineParams->params.plcrParams.sharedProfile)
                {
                    err = FmPcdPlcrGetAbsoluteProfileId(h_FmPcd,e_FM_PCD_PLCR_SHARED,NULL,p_FmPcdCcNextEngineParams->params.plcrParams.relativeProfileIdForCtrlFlow, &absoluteProfileId);
                    if(err)
                        RETURN_ERROR(MAJOR, err, ("Shared profile offset is out of range"));
                    if(!FmPcdPlcrIsProfileValid(h_FmPcd, absoluteProfileId))
                        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Invalid profile"));
                    if(p_FmPcdCcNextEngineParams->params.plcrParams.fqidEnqForCtrlFlow &&  (!p_FmPcdCcNextEngineParams->params.plcrParams.fqidForCtrlFlowForEnqueueAfterPlcr ||
                            p_FmPcdCcNextEngineParams->params.plcrParams.fqidForCtrlFlowForEnqueueAfterPlcr & ~0x00FFFFFF))
                        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("fqidForCtrlFlowForEnqueueAfterPlcr  must be between 1 and 2^24-1"));
                }
                else
                {
                    p_NextEngineParamsInfo->fmPcdEngine = e_FM_PCD_PLCR;
                    p_NextEngineParamsInfo->additionalInfo = p_FmPcdCcNextEngineParams->params.plcrParams.relativeProfileIdForCtrlFlow;
                }
            }
            break;
        case(e_FM_PCD_CC):
            if(!p_FmPcdCcNextEngineParams->params.ccParams.h_CcNode)
                RETURN_ERROR(MAJOR, E_NULL_POINTER, ("handler to next Node is NULL"));
            if (!CcNodeIsValid((t_FmPcd*)h_FmPcd,
                               (uint16_t)((t_FmPcdCcNode*)(p_FmPcdCcNextEngineParams->params.ccParams.h_CcNode))->nodeId))
                RETURN_ERROR(MAJOR, E_INVALID_SELECTION, ("not valid nodeId in CC next engine param" ));
            p_NextEngineParamsInfo->fmPcdEngine = e_FM_PCD_CC;
            p_NextEngineParamsInfo->additionalInfo  =
                (uint16_t)((t_FmPcdCcNode*)(p_FmPcdCcNextEngineParams->params.ccParams.h_CcNode))->nodeId;
            break;
        default:
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Next engine is not correct"));
    }

    return E_OK;
}





static uint8_t GetGenParseCode(e_FmPcdExtractFrom src, uint32_t offset, bool glblMask, uint8_t *parseArrayOffset)
{
    switch(src)
    {
        case(e_FM_PCD_EXTRACT_FROM_FRAME_START):
            if(glblMask)
                return CC_PC_GENERIC_WITH_MASK ;
            else
              return CC_PC_GENERIC_WITHOUT_MASK;
            break;
        case(e_FM_PCD_KG_EXTRACT_FROM_CURR_END_OF_PARSE):
            *parseArrayOffset = CC_PC_PR_NEXT_HEADER_OFFSET;
            if(offset)
                return CC_PR_OFFSET;
            else
                return CC_PR_WITHOUT_OFFSET;
        break;
        case(e_FM_PCD_EXTRACT_FROM_IC_KEY) :
            *parseArrayOffset = 0x50;
            return CC_PC_GENERIC_IC_GMASK;
            break;
        case(e_FM_PCD_EXTRACT_FROM_IC_HASH_EXACT_MATCH) :
            *parseArrayOffset = 0x48;
            return CC_PC_GENERIC_IC_GMASK;
            break;
        case(e_FM_PCD_EXTRACT_FROM_IC_HASH_INDEXED_MATCH) :
            return CC_PC_GENERIC_IC_HASH_INDEXED;
             break;
        default:
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("Illegal 'extract from' src"));
            return CC_PC_ILLEGAL;
    }
}


static uint8_t GetFullFieldParseCode(e_NetHeaderType hdr, e_FmPcdHdrIndex index, t_FmPcdFields field)
{

      switch(hdr)
        {
            case(HEADER_TYPE_NONE):
                ASSERT_COND(FALSE);
                return CC_PC_ILLEGAL;
            break;

       case(HEADER_TYPE_ETH):
                switch(field.eth)
                {
                    case(NET_HEADER_FIELD_ETH_DA):
                        return CC_PC_FF_MACDST;
                    case(NET_HEADER_FIELD_ETH_SA):
                         return CC_PC_FF_MACSRC;
                    case(NET_HEADER_FIELD_ETH_TYPE):
                         return CC_PC_FF_ETYPE;
                    default:
                        REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                        return CC_PC_ILLEGAL;
                }
                break;

         case(HEADER_TYPE_VLAN):
            switch(field.vlan)
            {
                case(NET_HEADER_FIELD_VLAN_TCI):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return CC_PC_FF_TCI1;
                    if(index == e_FM_PCD_HDR_INDEX_LAST)
                        return CC_PC_FF_TCI2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return CC_PC_ILLEGAL;
                default:
                        REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                        return CC_PC_ILLEGAL;
            }
            break;

        case(HEADER_TYPE_MPLS):
            switch(field.mpls)
            {
                case(NET_HEADER_FIELD_MPLS_LABEL_STACK):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return CC_PC_FF_MPLS1;
                    if(index == e_FM_PCD_HDR_INDEX_LAST)
                        return CC_PC_FF_MPLS_LAST;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal MPLS index"));
                    return CC_PC_ILLEGAL;
               default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return CC_PC_ILLEGAL;
             }
            break;

        case(HEADER_TYPE_IPv4):
            switch(field.ipv4)
            {
                case(NET_HEADER_FIELD_IPv4_DST_IP):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return CC_PC_FF_IPV4DST1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return CC_PC_FF_IPV4DST2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv4 index"));
                    return CC_PC_ILLEGAL;
                case(NET_HEADER_FIELD_IPv4_TOS):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return CC_PC_FF_IPV4IPTOS_TC1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return CC_PC_FF_IPV4IPTOS_TC2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv4 index"));
                    return CC_PC_ILLEGAL;
                case(NET_HEADER_FIELD_IPv4_PROTO):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return CC_PC_FF_IPV4PTYPE1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return CC_PC_FF_IPV4PTYPE2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv4 index"));
                    return CC_PC_ILLEGAL;
                case(NET_HEADER_FIELD_IPv4_SRC_IP):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return CC_PC_FF_IPV4SRC1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return CC_PC_FF_IPV4SRC2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv4 index"));
                    return CC_PC_ILLEGAL;
                case(NET_HEADER_FIELD_IPv4_SRC_IP | NET_HEADER_FIELD_IPv4_DST_IP):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return CC_PC_FF_IPV4SRC1_IPV4DST1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return CC_PC_FF_IPV4SRC2_IPV4DST2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv4 index"));
                    return CC_PC_ILLEGAL;
                case(NET_HEADER_FIELD_IPv4_TTL):
                    return CC_PC_FF_IPV4TTL;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return CC_PC_ILLEGAL;
            }
            break;
        case(HEADER_TYPE_IPv6):
             switch(field.ipv6)
            {
                case(NET_HEADER_FIELD_IPv6_VER | NET_HEADER_FIELD_IPv6_FL | NET_HEADER_FIELD_IPv6_TC):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return CC_PC_FF_IPTOS_IPV6TC1_IPV6FLOW1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return CC_PC_FF_IPTOS_IPV6TC2_IPV6FLOW2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv6 index"));
                    return CC_PC_ILLEGAL;
                case(NET_HEADER_FIELD_IPv6_NEXT_HDR):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return CC_PC_FF_IPV6PTYPE1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return CC_PC_FF_IPV6PTYPE2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv6 index"));
                    return CC_PC_ILLEGAL;
                case(NET_HEADER_FIELD_IPv6_DST_IP):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return CC_PC_FF_IPV6DST1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return CC_PC_FF_IPV6DST2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv6 index"));
                    return CC_PC_ILLEGAL;
                case(NET_HEADER_FIELD_IPv6_SRC_IP):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return CC_PC_FF_IPV6SRC1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return CC_PC_FF_IPV6SRC2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv6 index"));
                    return CC_PC_ILLEGAL;
                case(NET_HEADER_FIELD_IPv6_HOP_LIMIT):
                    return CC_PC_FF_IPV6HOP_LIMIT;
                 default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return CC_PC_ILLEGAL;
            }
            break;

        case(HEADER_TYPE_GRE):
            switch(field.gre)
            {
                case(NET_HEADER_FIELD_GRE_TYPE):
                    return CC_PC_FF_GREPTYPE;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return CC_PC_ILLEGAL;
           }
        case(HEADER_TYPE_MINENCAP):
            switch(field.minencap)
            {
                case(NET_HEADER_FIELD_MINENCAP_TYPE):
                    return CC_PC_FF_MINENCAP_PTYPE;
                case(NET_HEADER_FIELD_MINENCAP_DST_IP):
                    return CC_PC_FF_MINENCAP_IPDST;
                case(NET_HEADER_FIELD_MINENCAP_SRC_IP):
                    return CC_PC_FF_MINENCAP_IPSRC;
                case(NET_HEADER_FIELD_MINENCAP_SRC_IP | NET_HEADER_FIELD_MINENCAP_DST_IP):
                    return CC_PC_FF_MINENCAP_IPSRC_IPDST;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return CC_PC_ILLEGAL;
           }
           break;
        case(HEADER_TYPE_TCP):
            switch(field.tcp)
            {
                case(NET_HEADER_FIELD_TCP_PORT_SRC):
                    return CC_PC_FF_L4PSRC;
                case(NET_HEADER_FIELD_TCP_PORT_DST):
                    return CC_PC_FF_L4PDST;
                case(NET_HEADER_FIELD_TCP_PORT_DST | NET_HEADER_FIELD_TCP_PORT_SRC):
                    return CC_PC_FF_L4PSRC_L4PDST;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return CC_PC_ILLEGAL;
            }
            break;
        case(HEADER_TYPE_PPPoE):
            switch(field.pppoe)
            {
                case(NET_HEADER_FIELD_PPPoE_PID):
                    return CC_PC_FF_PPPPID;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return CC_PC_ILLEGAL;
            }
            break;
        case(HEADER_TYPE_UDP):
            switch(field.udp)
            {
                case(NET_HEADER_FIELD_UDP_PORT_SRC):
                    return CC_PC_FF_L4PSRC;
                case(NET_HEADER_FIELD_UDP_PORT_DST):
                    return CC_PC_FF_L4PDST;
                case(NET_HEADER_FIELD_UDP_PORT_DST | NET_HEADER_FIELD_UDP_PORT_SRC):
                    return CC_PC_FF_L4PSRC_L4PDST;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return CC_PC_ILLEGAL;
            }
            break;
         default:
            REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
            return CC_PC_ILLEGAL;

    }
}

static uint8_t GetPrParseCode(e_NetHeaderType hdr, e_FmPcdHdrIndex hdrIndex, uint32_t offset, bool glblMask, uint8_t *parseArrayOffset)
{

 bool offsetRelevant = FALSE;

    if(offset)
        offsetRelevant = TRUE;

    switch(hdr){
        case(HEADER_TYPE_NONE):
            ASSERT_COND(FALSE);
            return CC_PC_ILLEGAL;
        case(HEADER_TYPE_ETH):
            *parseArrayOffset = (uint8_t)CC_PC_PR_ETH_OFFSET;
            break;
        case(HEADER_TYPE_USER_DEFINED_SHIM1):
            if(offset || glblMask)
                *parseArrayOffset = (uint8_t)CC_PC_PR_USER_DEFINED_SHIM1_OFFSET;
            else
                return CC_PC_PR_SHIM1;
            break;
        case(HEADER_TYPE_USER_DEFINED_SHIM2):
            if(offset || glblMask)
                *parseArrayOffset = (uint8_t)CC_PC_PR_USER_DEFINED_SHIM2_OFFSET;
            else
                return CC_PC_PR_SHIM2;
            break;
        case(HEADER_TYPE_USER_DEFINED_SHIM3):
            if(offset || glblMask)
                *parseArrayOffset = (uint8_t)CC_PC_PR_USER_DEFINED_SHIM3_OFFSET;
            else
                return CC_PC_PR_SHIM3;
            break;
              break;
      case(HEADER_TYPE_LLC_SNAP):
            *parseArrayOffset = CC_PC_PR_USER_LLC_SNAP_OFFSET;
            break;
        case(HEADER_TYPE_PPPoE):
            *parseArrayOffset = CC_PC_PR_PPPOE_OFFSET;
            break;
            case(HEADER_TYPE_MPLS):
                 if((hdrIndex == e_FM_PCD_HDR_INDEX_NONE) || (hdrIndex == e_FM_PCD_HDR_INDEX_1))
                        *parseArrayOffset = CC_PC_PR_MPLS1_OFFSET;
                else if(hdrIndex == e_FM_PCD_HDR_INDEX_LAST)
                        *parseArrayOffset = CC_PC_PR_MPLS_LAST_OFFSET;
                else
                {
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal MPLS header index"));
                    return CC_PC_ILLEGAL;
                }
                break;
            case(HEADER_TYPE_IPv4):
            case(HEADER_TYPE_IPv6):
              if((hdrIndex == e_FM_PCD_HDR_INDEX_NONE) || (hdrIndex == e_FM_PCD_HDR_INDEX_1))
                    *parseArrayOffset = CC_PC_PR_IP1_OFFSET;
              else if(hdrIndex == e_FM_PCD_HDR_INDEX_2)
                    *parseArrayOffset = CC_PC_PR_IP_LAST_OFFSET;
              else
              {
                REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IP header index"));
                return CC_PC_ILLEGAL;

              }
                break;
            case(HEADER_TYPE_MINENCAP):
                *parseArrayOffset = CC_PC_PR_MINENC_OFFSET;
                break;
            case(HEADER_TYPE_GRE):
                *parseArrayOffset = CC_PC_PR_GRE_OFFSET;
                break;
            case(HEADER_TYPE_TCP):
            case(HEADER_TYPE_UDP):
            case(HEADER_TYPE_IPSEC_AH):
            case(HEADER_TYPE_IPSEC_ESP):
            case(HEADER_TYPE_DCCP):
            case(HEADER_TYPE_SCTP):
                *parseArrayOffset = CC_PC_PR_L4_OFFSET;
                break;

            default:
                REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IP header for this type of operation"));
                return CC_PC_ILLEGAL;
     }

        if(offsetRelevant)
            return CC_PR_OFFSET;
        else
            return CC_PR_WITHOUT_OFFSET;

}

static uint8_t GetFieldParseCode(e_NetHeaderType hdr, t_FmPcdFields field, uint32_t offset, uint8_t *parseArrayOffset, e_FmPcdHdrIndex hdrIndex)
{

 bool offsetRelevant = FALSE;

    if(offset)
        offsetRelevant = TRUE;

    switch(hdr)
    {
        case(HEADER_TYPE_NONE):
                ASSERT_COND(FALSE);
        case(HEADER_TYPE_ETH):
            switch(field.eth)
            {
                case(NET_HEADER_FIELD_ETH_TYPE):
                    *parseArrayOffset = CC_PC_PR_ETYPE_LAST_OFFSET;
                    break;
                REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                return CC_PC_ILLEGAL;
            }
            break;
        case(HEADER_TYPE_VLAN):
            switch(field.vlan)
            {
                case(NET_HEADER_FIELD_VLAN_TCI) :
                    if((hdrIndex == e_FM_PCD_HDR_INDEX_NONE) || (hdrIndex == e_FM_PCD_HDR_INDEX_1))
                        *parseArrayOffset = CC_PC_PR_VLAN1_OFFSET;
                    else if(hdrIndex == e_FM_PCD_HDR_INDEX_LAST)
                        *parseArrayOffset = CC_PC_PR_VLAN2_OFFSET;
                    break;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return CC_PC_ILLEGAL;
            }
        break;
        default:
            REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal header "));
            return CC_PC_ILLEGAL;
    }
    if(offsetRelevant)
        return CC_PR_OFFSET;
    else
        return CC_PR_WITHOUT_OFFSET;

}

static void FillAdOfTypeResult(t_Handle p_Ad, t_FmPcd *p_FmPcd, t_FmPcdCcNextEngineParams *p_CcNextEngineParams)
{
    t_AdOfTypeResult                *p_AdResult = (t_AdOfTypeResult*)p_Ad;
    uint32_t                        tmp = 0, tmpNia = 0;
    uint16_t                        profileId;

    switch(p_CcNextEngineParams->nextEngine)
    {
        case(e_FM_PCD_DONE):
            if(p_CcNextEngineParams->params.enqueueParams.ctrlFlow)
            {
               tmp = FM_PCD_AD_RESULT_CONTRL_FLOW_TYPE;
               tmp |= p_CcNextEngineParams->params.enqueueParams.fqidForCtrlFlow;
            }
            else
            {
               tmp = FM_PCD_AD_RESULT_DATA_FLOW_TYPE;
               tmp |= FM_PCD_AD_RESULT_PLCR_DIS;
            }
            tmpNia |= NIA_ENG_BMI |NIA_BMI_AC_ENQ_FRAME;
            break;
        case(e_FM_PCD_KG):
            if(p_CcNextEngineParams->params.kgParams.ctrlFlow)
                tmp = FM_PCD_AD_RESULT_CONTRL_FLOW_TYPE;
            else
            {
                tmp = FM_PCD_AD_RESULT_DATA_FLOW_TYPE;
                tmp |= FM_PCD_AD_RESULT_PLCR_DIS;
            }
            tmpNia = NIA_KG_DIRECT;
            tmpNia |= NIA_ENG_KG;
            tmpNia |= (uint8_t)(CAST_POINTER_TO_UINT32(p_CcNextEngineParams->params.kgParams.h_DirectScheme)-1);
        break;
        case(e_FM_PCD_PLCR):
            tmp = 0;
            if(p_CcNextEngineParams->params.plcrParams.ctrlFlow)
            {
                tmp = FM_PCD_AD_RESULT_CONTRL_FLOW_TYPE;

                /* if private policer profile, it may be uninitialized yet, therefor no checks are done at this stage */
                if(p_CcNextEngineParams->params.plcrParams.sharedProfile)
                {
                    tmpNia |= NIA_PLCR_ABSOLUTE;
                    FmPcdPlcrGetAbsoluteProfileId((t_Handle)p_FmPcd,e_FM_PCD_PLCR_SHARED,NULL,p_CcNextEngineParams->params.plcrParams.relativeProfileIdForCtrlFlow, &profileId);
                }
                else
                    profileId = p_CcNextEngineParams->params.plcrParams.relativeProfileIdForCtrlFlow;

                if(p_CcNextEngineParams->params.plcrParams.fqidEnqForCtrlFlow)
                    tmp |= p_CcNextEngineParams->params.plcrParams.fqidForCtrlFlowForEnqueueAfterPlcr;
                WRITE_UINT32(p_AdResult->plcrProfile,(uint32_t)((uint32_t)profileId << FM_PCD_AD_PROFILEID_FOR_CNTRL_SHIFT));
            }
            else
               tmp = FM_PCD_AD_RESULT_DATA_FLOW_TYPE;
            tmpNia |= NIA_ENG_PLCR | p_CcNextEngineParams->params.plcrParams.relativeProfileIdForCtrlFlow;
           break;
        default:
            return;
    }

    WRITE_UINT32(p_AdResult->fqid, tmp);
    WRITE_UINT32(p_AdResult->nia, tmpNia);
}

static void FillAdOfTypeContLookup(t_Handle p_Ad,  t_Handle h_FmPcd, t_Handle p_FmPcdCcNode)
{
    t_FmPcdCcNode           *p_Node = (t_FmPcdCcNode *)p_FmPcdCcNode;
    t_AdOfTypeContLookup    *p_AdContLookup = (t_AdOfTypeContLookup *)p_Ad;
    uint32_t                tmpReg32;

    tmpReg32 = 0;
    tmpReg32 |= FM_PCD_AD_CONT_LOOKUP_TYPE;
    tmpReg32 |= p_Node->sizeOfExtraction ? ((p_Node->sizeOfExtraction - 1) << 24) : 0;
    tmpReg32 |= (uint32_t)((uint64_t)(XX_VirtToPhys(p_Node->h_AdTable)) - (((t_FmPcd*)h_FmPcd)->p_FmPcdCc)->physicalMuramBase);
    WRITE_UINT32(p_AdContLookup->ccAdBase, tmpReg32);

    tmpReg32 = 0;
    tmpReg32 |= p_Node->numOfKeys << 24;
    tmpReg32 |= (p_Node->lclMask ? FM_PCD_AD_CONT_LOOKUP_LCL_MASK : 0);
    tmpReg32 |= p_Node->h_KeysMatchTable ?
                    (uint32_t)((uint64_t)(XX_VirtToPhys(p_Node->h_KeysMatchTable)) - (((t_FmPcd*)h_FmPcd)->p_FmPcdCc)->physicalMuramBase) : 0;
    WRITE_UINT32(p_AdContLookup->matchTblPtr, tmpReg32);

    tmpReg32 = 0;
    tmpReg32 |= p_Node->prsArrayOffset << 24;
    tmpReg32 |= p_Node->offset << 16;
    tmpReg32 |= p_Node->parseCode;
    WRITE_UINT32(p_AdContLookup->pcAndOffsets, tmpReg32);

    COPY_BLOCK((void*)&p_AdContLookup->gmask, p_Node->p_GlblMask, p_Node->glblMaskSize);
}

static void NextStepAd(t_Handle p_Ad, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams, t_FmPcd *p_FmPcd)
{
    switch(p_FmPcdCcNextEngineParams->nextEngine)
    {
        case(e_FM_PCD_KG):
        case(e_FM_PCD_PLCR):
        case(e_FM_PCD_DONE):
            FillAdOfTypeResult(p_Ad, p_FmPcd, p_FmPcdCcNextEngineParams);
            break;
        case(e_FM_PCD_CC):
            FillAdOfTypeContLookup( p_Ad,
                                   p_FmPcd,
                                    p_FmPcdCcNextEngineParams->params.ccParams.h_CcNode);
            UpdateNodeOwner(p_FmPcd,
                            (uint16_t)(((t_FmPcdCcNode *)(p_FmPcdCcNextEngineParams->params.ccParams.h_CcNode))->nodeId),
                            TRUE);
        break;
        default:
            return;
    }
}

static t_Error ModifyCcCommon1(t_Handle h_FmPcd, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams,t_Handle *h_OldPointer, t_Handle *h_NewPointer)
{
    t_FmPcdModifyCcAdditionalParams *p_CcOldModifyAdditionalParams;
    t_FmPcdModifyCcAdditionalParams *p_CcNewModifyAdditionalParams;
    t_Error                         err = E_OK;
    t_FmPcd                         *p_FmPcd = (t_FmPcd *)h_FmPcd;
    t_NextEngineParamsInfo          nextEngineParamsInfo;

    SANITY_CHECK_RETURN_ERROR(h_FmPcd,E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc,E_INVALID_STATE);

    err = ValidateNextEngineParams(h_FmPcd, p_FmPcdCcNextEngineParams, &nextEngineParamsInfo);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    p_CcOldModifyAdditionalParams =(t_FmPcdModifyCcAdditionalParams *)XX_Malloc(sizeof(t_FmPcdModifyCcAdditionalParams));
    if(!p_CcOldModifyAdditionalParams)
        RETURN_ERROR(MAJOR, E_NO_MEMORY, ("Memory allocation FAILED"));
    memset(p_CcOldModifyAdditionalParams, 0, sizeof(t_FmPcdModifyCcAdditionalParams));

    p_CcNewModifyAdditionalParams =(t_FmPcdModifyCcAdditionalParams *)XX_Malloc(sizeof(t_FmPcdModifyCcAdditionalParams));
    if(!p_CcNewModifyAdditionalParams)
    {
        XX_Free(p_CcOldModifyAdditionalParams);
        RETURN_ERROR(MAJOR, E_NO_MEMORY, ("Memory allocation FAILED"));
    }
    memset(p_CcNewModifyAdditionalParams, 0, sizeof(t_FmPcdModifyCcAdditionalParams));

    p_CcNewModifyAdditionalParams->p_Ad = (t_Handle)FM_MURAM_AllocMem(p_FmPcd->p_FmPcdCc->h_FmMuram,
                                         FM_PCD_CC_AD_ENTRY_SIZE,
                                         FM_PCD_CC_AD_TABLE_ALIGN);

    if(!p_CcNewModifyAdditionalParams->p_Ad)
    {
        XX_Free(p_CcOldModifyAdditionalParams);
        XX_Free(p_CcNewModifyAdditionalParams);
        RETURN_ERROR(MAJOR, E_NO_MEMORY, ("Memory allocation in MURAM FAILED"));

    }
    WRITE_BLOCK((uint8_t *)p_CcNewModifyAdditionalParams->p_Ad, 0,  FM_PCD_CC_AD_ENTRY_SIZE);

    if(p_FmPcdCcNextEngineParams)
        NextStepAd((t_Handle)p_CcNewModifyAdditionalParams->p_Ad,p_FmPcdCcNextEngineParams, p_FmPcd);

    p_CcNewModifyAdditionalParams->fmPcdEngine      = nextEngineParamsInfo.fmPcdEngine;
    if(p_CcNewModifyAdditionalParams->fmPcdEngine == e_FM_PCD_CC)
        p_CcNewModifyAdditionalParams->myInfo = nextEngineParamsInfo.additionalInfo;
    else
        p_CcNewModifyAdditionalParams->myInfo = 0xffffffff;

    *h_OldPointer = p_CcOldModifyAdditionalParams;
    *h_NewPointer = p_CcNewModifyAdditionalParams;

    return E_OK;
}

static void ReleaseCommonModifyKey(t_Handle h_FmMuram, t_FmPcdModifyCcAdditionalParams *p_CcModifyAdditionalParams)
{
   if(p_CcModifyAdditionalParams->adAllocated)
        FM_MURAM_FreeMem(h_FmMuram, p_CcModifyAdditionalParams->p_Ad);
   XX_Free(p_CcModifyAdditionalParams);
}

static t_Error ModifyCcKeyCommon(t_FmPcd *p_FmPcd, t_Handle *h_Pointer,bool allocateAd)
{
    t_FmPcdModifyCcAdditionalParams *p_ModifyAdditionalParams;

    p_ModifyAdditionalParams =(t_FmPcdModifyCcAdditionalParams *)XX_Malloc(sizeof(t_FmPcdModifyCcAdditionalParams));
    if(!p_ModifyAdditionalParams)
        RETURN_ERROR(MAJOR, E_NO_MEMORY, ("Memory allocation FAILED"));
    memset(p_ModifyAdditionalParams, 0, sizeof(t_FmPcdModifyCcAdditionalParams));
    if(allocateAd)
    {
        p_ModifyAdditionalParams->p_Ad = (t_Handle)FM_MURAM_AllocMem(p_FmPcd->p_FmPcdCc->h_FmMuram,
                                             FM_PCD_CC_AD_ENTRY_SIZE,
                                             FM_PCD_CC_AD_TABLE_ALIGN);

        if(!p_ModifyAdditionalParams->p_Ad)
        {
            XX_Free(p_ModifyAdditionalParams);
            XX_Free(p_ModifyAdditionalParams);
            RETURN_ERROR(MAJOR, E_NO_MEMORY, ("Memory allocation in MURAM FAILED"));

        }
        WRITE_BLOCK((uint8_t *)p_ModifyAdditionalParams->p_Ad, 0,  FM_PCD_CC_AD_ENTRY_SIZE);
        p_ModifyAdditionalParams->adAllocated = TRUE;
    }
    *h_Pointer = p_ModifyAdditionalParams;

    return E_OK;
}


static void ModifyCcCommon2(t_FmPcdModifyCcAdditionalParams *p_CcOldModifyAdditionalParams,
                            t_FmPcdModifyCcAdditionalParams *p_CcNewModifyAdditionalParams,
                            uint16_t indx)
{

    uint32_t    tmpReg;

    if(p_CcNewModifyAdditionalParams->fmPcdEngine == e_FM_PCD_CC)
        p_CcNewModifyAdditionalParams->myInfo |= (uint32_t)indx <<16;

    tmpReg = GET_UINT32(*((uint32_t*)p_CcOldModifyAdditionalParams->p_Ad));
    if((tmpReg & FM_PCD_AD_TYPE_MASK) == FM_PCD_AD_CONT_LOOKUP_TYPE)
    {
        p_CcOldModifyAdditionalParams->fmPcdEngine = e_FM_PCD_CC;
        p_CcOldModifyAdditionalParams->myInfo =(uint32_t)indx ;
    }
    else
        p_CcOldModifyAdditionalParams->myInfo = 0xffffffff;
}

static void ReleaseNewNodeCommonPart(t_FmPcdCc *p_FmPcdCc, t_FmPcdModifyCcKeyAdditionalParams *p_AdditionalInfo)
{
    if(p_AdditionalInfo->p_AdTableNew)
        FM_MURAM_FreeMem(p_FmPcdCc->h_FmMuram, p_AdditionalInfo->p_AdTableNew);
    if(p_AdditionalInfo->p_KeysMatchTableNew)
        FM_MURAM_FreeMem(p_FmPcdCc->h_FmMuram, p_AdditionalInfo->p_KeysMatchTableNew);
}

static t_Error BuildNewNodeCommonPart(t_Handle *h_FmPcd, t_FmPcdCcNode *p_FmPcdCcNode, int *size, bool mask, t_FmPcdModifyCcKeyAdditionalParams *p_AdditionalInfo)
{
    t_FmPcd                *p_FmPcd = (t_FmPcd*)h_FmPcd;
    t_FmPcdCc               *p_FmPcdCc = p_FmPcd->p_FmPcdCc;

    p_AdditionalInfo->p_AdTableNew = (t_Handle)FM_MURAM_AllocMem(p_FmPcdCc->h_FmMuram,
                                     (uint32_t)( (p_AdditionalInfo->numOfKeys+1) * FM_PCD_CC_AD_ENTRY_SIZE),
                                     FM_PCD_CC_AD_TABLE_ALIGN);
    if(!p_AdditionalInfo->p_AdTableNew)
        RETURN_ERROR(MAJOR, E_NO_MEMORY, ("No memory in MURAM ffor AD table "));

    WRITE_BLOCK((uint8_t *)p_AdditionalInfo->p_AdTableNew, 0, (uint32_t)((p_AdditionalInfo->numOfKeys+1) * FM_PCD_CC_AD_ENTRY_SIZE));

    if(p_FmPcdCcNode->lclMask || mask)
    {
        p_AdditionalInfo->lclMask = TRUE;
        *size = 2 * p_FmPcdCcNode->ccKeySizeAccExtraction;
    }
    else
    {
        p_AdditionalInfo->lclMask = p_FmPcdCcNode->lclMask;
        *size = p_FmPcdCcNode->ccKeySizeAccExtraction;
    }

    p_AdditionalInfo->p_KeysMatchTableNew =(t_Handle)FM_MURAM_AllocMem(p_FmPcdCc->h_FmMuram,
                                         (uint32_t)(*size * sizeof(uint8_t) * (p_AdditionalInfo->numOfKeys + 1)),
                                         FM_PCD_CC_KEYS_MATCH_TABLE_ALIGN);
    if(!p_AdditionalInfo->p_KeysMatchTableNew)
    {
        FM_MURAM_FreeMem(p_FmPcdCc->h_FmMuram, p_AdditionalInfo->p_KeysMatchTableNew);
        RETURN_ERROR(MAJOR, E_NO_MEMORY, ("No memory in MURAM for KEY MATCH table"));
    }
    WRITE_BLOCK((uint8_t *)p_AdditionalInfo->p_KeysMatchTableNew, 0, *size * sizeof(uint8_t) * (p_AdditionalInfo->numOfKeys + 1));


    p_AdditionalInfo->p_AdTableOld          = p_FmPcdCcNode->h_AdTable;
    p_AdditionalInfo->p_KeysMatchTableOld   =p_FmPcdCcNode->h_KeysMatchTable;

    return E_OK;
}

static t_Error BuildNewNodeAddKey(t_Handle h_FmPcd ,t_FmPcdCcNode *p_FmPcdCcNode, uint8_t keyIndex, t_FmPcdCcKeyParams  *p_KeyParams,t_FmPcdModifyCcKeyAdditionalParams *p_AdditionalInfo)
{
    t_Error                 err = E_OK;
    t_NextEngineParamsInfo  nextEngineParamsInfo;
    t_Handle                p_AdTableNewTmp, p_KeysMatchTableNewTmp;
    t_Handle                p_KeysMatchTableOldTmp, p_AdTableOldTmp;
    int                     size;
    int                     i = 0, j = 0;
    t_FmPcd                 *p_FmPcd = (t_FmPcd*)h_FmPcd;

    err = ValidateNextEngineParams(h_FmPcd, &p_KeyParams->ccNextEngineParams, &nextEngineParamsInfo);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    p_AdditionalInfo->numOfKeys = (uint8_t)(p_FmPcdCcNode->numOfKeys + 1);

    err = BuildNewNodeCommonPart(h_FmPcd, p_FmPcdCcNode, &size, (bool)(p_KeyParams->p_Mask ? TRUE : FALSE), p_AdditionalInfo);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    i = 0;
    for(j = 0; j < p_AdditionalInfo->numOfKeys; j++)
    {
        p_AdTableNewTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableNew + j*FM_PCD_CC_AD_ENTRY_SIZE);
        if(j == keyIndex)
         {
            NextStepAd(p_AdTableNewTmp,&p_KeyParams->ccNextEngineParams, p_FmPcd);
            p_KeysMatchTableNewTmp = (t_Handle)(((uint32_t)p_AdditionalInfo->p_KeysMatchTableNew) + j*size * sizeof(uint8_t));
            COPY_BLOCK((void*)p_KeysMatchTableNewTmp, p_KeyParams->p_Key, p_FmPcdCcNode->ccKeySizeAccExtraction);
            if(p_AdditionalInfo->lclMask)
            {
                if(p_KeyParams->p_Mask)
                    IO2IOCpy32((t_Handle)(((uint32_t)p_KeysMatchTableNewTmp) + p_FmPcdCcNode->ccKeySizeAccExtraction), p_KeyParams->p_Mask, p_FmPcdCcNode->ccKeySizeAccExtraction);
                else
                    WRITE_BLOCK((void*)(((uint32_t)p_KeysMatchTableNewTmp) + p_FmPcdCcNode->ccKeySizeAccExtraction),0xff, p_FmPcdCcNode->ccKeySizeAccExtraction);
            }
         }
         else
         {
            p_AdTableOldTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableOld + i*FM_PCD_CC_AD_ENTRY_SIZE);
            IO2IOCpy32(p_AdTableNewTmp, p_AdTableOldTmp,  FM_PCD_CC_AD_ENTRY_SIZE);
            p_KeysMatchTableNewTmp = (t_Handle)(((uint32_t)p_AdditionalInfo->p_KeysMatchTableNew) + j*size * sizeof(uint8_t));
            p_KeysMatchTableOldTmp  = (t_Handle)((uint32_t)p_AdditionalInfo->p_KeysMatchTableOld + i*size*sizeof(uint8_t));
            if(p_AdditionalInfo->lclMask)
            {
                if(p_FmPcdCcNode->lclMask)
                {
                    IO2IOCpy32((t_Handle)((uint32_t)p_KeysMatchTableNewTmp + p_FmPcdCcNode->ccKeySizeAccExtraction), (t_Handle)((uint32_t)p_KeysMatchTableOldTmp+p_FmPcdCcNode->ccKeySizeAccExtraction),p_FmPcdCcNode->ccKeySizeAccExtraction);
                }
                else
                {
                    p_KeysMatchTableOldTmp  = (t_Handle)((uint32_t)p_FmPcdCcNode->h_KeysMatchTable + i*p_FmPcdCcNode->ccKeySizeAccExtraction*sizeof(uint8_t));
                    WRITE_BLOCK((void*)(((uint32_t)p_KeysMatchTableNewTmp) + p_FmPcdCcNode->ccKeySizeAccExtraction),0xff, p_FmPcdCcNode->ccKeySizeAccExtraction);
                }
            }
            IO2IOCpy32((void*)p_KeysMatchTableNewTmp, p_KeysMatchTableOldTmp, p_FmPcdCcNode->ccKeySizeAccExtraction);
           i++;
         }
    }

    p_AdTableNewTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableNew + j*FM_PCD_CC_AD_ENTRY_SIZE);
    p_AdTableOldTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableOld + i*FM_PCD_CC_AD_ENTRY_SIZE);
    IO2IOCpy32((void*)p_AdTableNewTmp, p_AdTableOldTmp, FM_PCD_CC_AD_ENTRY_SIZE);

    p_AdditionalInfo->keyIndexForRemove = 0xffff;
    if(nextEngineParamsInfo.fmPcdEngine == e_FM_PCD_CC)
        p_AdditionalInfo->nodeIdForAdd = (uint16_t)nextEngineParamsInfo.additionalInfo;
    else
        p_AdditionalInfo->nodeIdForAdd = 0xffff;
    return E_OK;
}

static t_Error BuildNewNodeRemoveKey(t_Handle h_FmPcd  ,t_FmPcdCcNode *p_FmPcdCcNode, uint8_t keyIndex, t_FmPcdModifyCcKeyAdditionalParams *p_AdditionalInfo)
{
    int         i = 0, j = 0;
    t_Handle    p_AdTableNewTmp,p_KeysMatchTableNewTmp;
    t_Handle    p_KeysMatchTableOldTmp, p_AdTableOldTmp;
    int         size;
    t_Error     err = E_OK;

    p_AdditionalInfo->numOfKeys = (uint16_t)(p_FmPcdCcNode->numOfKeys - 1);

    err = BuildNewNodeCommonPart(h_FmPcd, p_FmPcdCcNode, &size, (bool)(p_FmPcdCcNode->lclMask ? TRUE : FALSE), p_AdditionalInfo);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    for(i = 0, j = 0; j < p_FmPcdCcNode->numOfKeys; i++, j++)
    {
        if(j == keyIndex)
        {
            p_AdTableOldTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableOld + j*FM_PCD_CC_AD_ENTRY_SIZE);
            if((GET_UINT32(*(uint32_t*)p_AdTableOldTmp) & FM_PCD_AD_TYPE_MASK) == FM_PCD_AD_CONT_LOOKUP_TYPE)
                p_AdditionalInfo->keyIndexForRemove = keyIndex;
            else
                p_AdditionalInfo->keyIndexForRemove = 0xffff;
            j++;
        }
        if(j == p_FmPcdCcNode->numOfKeys)
            break;
         p_AdTableNewTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableNew + i*FM_PCD_CC_AD_ENTRY_SIZE);
         p_AdTableOldTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableOld + j*FM_PCD_CC_AD_ENTRY_SIZE);
         IO2IOCpy32(p_AdTableNewTmp,p_AdTableOldTmp,  FM_PCD_CC_AD_ENTRY_SIZE);
         p_KeysMatchTableOldTmp  = (t_Handle)((uint32_t)p_AdditionalInfo->p_KeysMatchTableOld + j*size*sizeof(uint8_t));
         p_KeysMatchTableNewTmp  = (t_Handle)((uint32_t)p_AdditionalInfo->p_KeysMatchTableNew+ i*size*sizeof(uint8_t));
         IO2IOCpy32(p_KeysMatchTableNewTmp,p_KeysMatchTableOldTmp,  size * sizeof(uint8_t));
    }

    p_AdTableNewTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableNew + i*FM_PCD_CC_AD_ENTRY_SIZE);
    p_AdTableOldTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableOld + j*FM_PCD_CC_AD_ENTRY_SIZE);
    IO2IOCpy32(p_AdTableNewTmp,p_AdTableOldTmp,  FM_PCD_CC_AD_ENTRY_SIZE);

    p_AdditionalInfo->nodeIdForAdd = 0xffff;

   return E_OK;
}


static t_Error BuildNewNodeModifyKey(t_Handle h_FmPcd ,t_FmPcdCcNode *p_FmPcdCcNode, uint8_t keyIndex, uint8_t  *p_Key, uint8_t *p_Mask,t_FmPcdModifyCcKeyAdditionalParams *p_AdditionalInfo)
{
    t_Error                 err = E_OK;
    t_Handle                p_AdTableNewTmp, p_KeysMatchTableNewTmp;
    t_Handle                p_KeysMatchTableOldTmp, p_AdTableOldTmp;
    int                     size;
    int                     i = 0, j = 0;

    p_AdditionalInfo->numOfKeys =  p_FmPcdCcNode->numOfKeys;

    err = BuildNewNodeCommonPart(h_FmPcd, p_FmPcdCcNode, &size, (bool)(p_Mask ? TRUE : FALSE), p_AdditionalInfo);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    for(j = 0, i = 0; j < p_AdditionalInfo->numOfKeys; j++, i++)
    {
        p_AdTableNewTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableNew + j*FM_PCD_CC_AD_ENTRY_SIZE);
        p_AdTableOldTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableOld + i*FM_PCD_CC_AD_ENTRY_SIZE);
        IO2IOCpy32(p_AdTableNewTmp, p_AdTableOldTmp,  FM_PCD_CC_AD_ENTRY_SIZE);
        if(j == keyIndex)
        {
            p_KeysMatchTableNewTmp = (t_Handle)(((uint32_t)p_AdditionalInfo->p_KeysMatchTableNew) + j*size * sizeof(uint8_t));
            COPY_BLOCK((void*)p_KeysMatchTableNewTmp, p_Key, p_FmPcdCcNode->ccKeySizeAccExtraction);
            if(p_AdditionalInfo->lclMask)
            {
                if(p_Mask)
                    IO2IOCpy32((t_Handle)(((uint32_t)p_KeysMatchTableNewTmp) + p_FmPcdCcNode->ccKeySizeAccExtraction), p_Mask, p_FmPcdCcNode->ccKeySizeAccExtraction);
                else
                    WRITE_BLOCK((void*)(((uint32_t)p_KeysMatchTableNewTmp) + p_FmPcdCcNode->ccKeySizeAccExtraction),0xff, p_FmPcdCcNode->ccKeySizeAccExtraction);
            }
        }
        else
        {
            p_KeysMatchTableNewTmp = (t_Handle)(((uint32_t)p_AdditionalInfo->p_KeysMatchTableNew) + j*size * sizeof(uint8_t));
            p_KeysMatchTableOldTmp  = (t_Handle)((uint32_t)p_FmPcdCcNode->h_KeysMatchTable + i*size*sizeof(uint8_t));
            if(p_AdditionalInfo->lclMask)
            {
                if(p_FmPcdCcNode->lclMask)
                {
                    IO2IOCpy32((t_Handle)((uint32_t)p_KeysMatchTableNewTmp + p_FmPcdCcNode->ccKeySizeAccExtraction), (t_Handle)((uint32_t)p_KeysMatchTableOldTmp+p_FmPcdCcNode->ccKeySizeAccExtraction),p_FmPcdCcNode->ccKeySizeAccExtraction);
                }
                else
                {
                    p_KeysMatchTableOldTmp  = (t_Handle)((uint32_t)p_FmPcdCcNode->h_KeysMatchTable + i*p_FmPcdCcNode->ccKeySizeAccExtraction*sizeof(uint8_t));
                    WRITE_BLOCK((void*)(((uint32_t)p_KeysMatchTableNewTmp) + p_FmPcdCcNode->ccKeySizeAccExtraction),0xff, p_FmPcdCcNode->ccKeySizeAccExtraction);
                }
            }
            IO2IOCpy32((void*)p_KeysMatchTableNewTmp, p_KeysMatchTableOldTmp, p_FmPcdCcNode->ccKeySizeAccExtraction);
        }
    }

    p_AdTableNewTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableNew + j*FM_PCD_CC_AD_ENTRY_SIZE);
    p_AdTableOldTmp = (t_Handle)((uint32_t)p_FmPcdCcNode->h_AdTable + i*FM_PCD_CC_AD_ENTRY_SIZE);
    IO2IOCpy32((void*)p_AdTableNewTmp, p_AdTableOldTmp, FM_PCD_CC_AD_ENTRY_SIZE);

    p_AdditionalInfo->nodeIdForAdd = 0xffff;
    p_AdditionalInfo->keyIndexForRemove = 0xffff;

    return E_OK;
}

static t_Error BuildNewNodeModifyKeyAndNextEngine(t_Handle h_FmPcd ,t_FmPcdCcNode *p_FmPcdCcNode, uint8_t keyIndex, t_FmPcdCcKeyParams  *p_KeyParams,t_FmPcdModifyCcKeyAdditionalParams *p_AdditionalInfo)
{
    t_Error                 err = E_OK;
    t_NextEngineParamsInfo nextEngineParamsInfo;
    t_FmPcd                 *p_FmPcd = (t_FmPcd *)h_FmPcd;
    t_Handle                p_AdTableNewTmp,p_KeysMatchTableNewTmp;
    t_Handle                p_KeysMatchTableOldTmp, p_AdTableOldTmp;
    int                     size;
    int                     i = 0, j = 0;

    err = ValidateNextEngineParams(h_FmPcd, &p_KeyParams->ccNextEngineParams, &nextEngineParamsInfo);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    p_AdditionalInfo->numOfKeys = p_FmPcdCcNode->numOfKeys;

    err = BuildNewNodeCommonPart(h_FmPcd, p_FmPcdCcNode, &size, (bool)(p_KeyParams->p_Mask ? TRUE : FALSE), p_AdditionalInfo);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    for(j = 0, i = 0; j < p_AdditionalInfo->numOfKeys; j++, i++)
    {
        p_AdTableNewTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableNew + j*FM_PCD_CC_AD_ENTRY_SIZE);
        if(j == keyIndex)
         {
            p_AdTableOldTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableOld + i*FM_PCD_CC_AD_ENTRY_SIZE);
            if((GET_UINT32(*(uint32_t*)p_AdTableOldTmp) & FM_PCD_AD_TYPE_MASK) == FM_PCD_AD_CONT_LOOKUP_TYPE)
                p_AdditionalInfo->keyIndexForRemove = keyIndex;
            else
                p_AdditionalInfo->keyIndexForRemove = 0xffff;
            NextStepAd(p_AdTableNewTmp,&p_KeyParams->ccNextEngineParams, p_FmPcd);
            p_KeysMatchTableNewTmp = (t_Handle)(((uint32_t)p_AdditionalInfo->p_KeysMatchTableNew) + j*size * sizeof(uint8_t));
            COPY_BLOCK((void*)p_KeysMatchTableNewTmp, p_KeyParams->p_Key, p_FmPcdCcNode->ccKeySizeAccExtraction);
            if(p_FmPcdCcNode->lclMask)
            {
                if(p_KeyParams->p_Mask)
                    IO2IOCpy32((void*)(((uint32_t)p_KeysMatchTableNewTmp) + p_FmPcdCcNode->ccKeySizeAccExtraction), p_KeyParams->p_Mask, p_FmPcdCcNode->ccKeySizeAccExtraction);
                else
                    WRITE_BLOCK((void*)(((uint32_t)p_KeysMatchTableNewTmp) + p_FmPcdCcNode->ccKeySizeAccExtraction),0xff, p_FmPcdCcNode->ccKeySizeAccExtraction);
            }
         }
         else
         {
            p_AdTableOldTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableOld + i*FM_PCD_CC_AD_ENTRY_SIZE);
            IO2IOCpy32(p_AdTableNewTmp, p_AdTableOldTmp,  FM_PCD_CC_AD_ENTRY_SIZE);
            p_KeysMatchTableNewTmp = (t_Handle)(((uint32_t)p_AdditionalInfo->p_KeysMatchTableNew) + j*size * sizeof(uint8_t));
            p_KeysMatchTableOldTmp  = (t_Handle)((uint32_t)p_AdditionalInfo->p_KeysMatchTableOld + i*size*sizeof(uint8_t));
            if(p_AdditionalInfo->lclMask)
            {
                if(p_FmPcdCcNode->lclMask)
                    IO2IOCpy32((t_Handle)((uint32_t)p_KeysMatchTableNewTmp + p_FmPcdCcNode->ccKeySizeAccExtraction), (t_Handle)((uint32_t)p_KeysMatchTableOldTmp+p_FmPcdCcNode->ccKeySizeAccExtraction),p_FmPcdCcNode->ccKeySizeAccExtraction);
                else
                {
                    p_KeysMatchTableOldTmp  = (t_Handle)((uint32_t)p_AdditionalInfo->p_KeysMatchTableOld + i*p_FmPcdCcNode->ccKeySizeAccExtraction*sizeof(uint8_t));
                    WRITE_BLOCK((void*)(((uint32_t)p_KeysMatchTableNewTmp) + p_FmPcdCcNode->ccKeySizeAccExtraction),0xff, p_FmPcdCcNode->ccKeySizeAccExtraction);
                }
            }
             IO2IOCpy32((void*)p_KeysMatchTableNewTmp, p_KeysMatchTableOldTmp, p_FmPcdCcNode->ccKeySizeAccExtraction);
         }
    }

    p_AdTableNewTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableNew + j*FM_PCD_CC_AD_ENTRY_SIZE);
    p_AdTableOldTmp = (t_Handle)((uint32_t)p_AdditionalInfo->p_AdTableOld + i*FM_PCD_CC_AD_ENTRY_SIZE);
    IO2IOCpy32((void*)p_AdTableNewTmp, p_AdTableOldTmp, FM_PCD_CC_AD_ENTRY_SIZE);

    if(nextEngineParamsInfo.fmPcdEngine == e_FM_PCD_CC)
        p_AdditionalInfo->nodeIdForAdd = (uint16_t)nextEngineParamsInfo.additionalInfo;
    else
        p_AdditionalInfo->nodeIdForAdd = 0xffff;
    return E_OK;
}


static void FillNodeWithParams(t_FmPcdCcNode *p_FmPcdCcNodeTo, t_FmPcdCcNode *p_FmPcdCcNodeFrom, t_FmPcdModifyCcKeyAdditionalParams *p_FmPcdModifyCcKeyAdditionalParams)
{
    p_FmPcdCcNodeTo->h_AdTable       = p_FmPcdModifyCcKeyAdditionalParams->p_AdTableNew;
    p_FmPcdCcNodeTo->numOfKeys       = p_FmPcdModifyCcKeyAdditionalParams->numOfKeys;
    p_FmPcdCcNodeTo->lclMask         = p_FmPcdModifyCcKeyAdditionalParams->lclMask;
    p_FmPcdCcNodeTo->h_KeysMatchTable= p_FmPcdModifyCcKeyAdditionalParams->p_KeysMatchTableNew;

    p_FmPcdCcNodeTo->sizeOfExtraction = p_FmPcdCcNodeFrom->sizeOfExtraction;
    p_FmPcdCcNodeTo->prsArrayOffset  = p_FmPcdCcNodeFrom->prsArrayOffset;
    p_FmPcdCcNodeTo->offset          = p_FmPcdCcNodeFrom->offset;
    p_FmPcdCcNodeTo->parseCode       = p_FmPcdCcNodeFrom->parseCode;
    p_FmPcdCcNodeTo->p_GlblMask      = p_FmPcdCcNodeFrom->p_GlblMask;
    p_FmPcdCcNodeTo->glblMaskSize    = p_FmPcdCcNodeFrom->glblMaskSize;
}

static t_Error ModifyWithNodeDataStructure(t_FmPcd *p_FmPcd,uint16_t nodeId, t_FmPcdCcNode *p_FmPcdCcNode ,t_List  *h_OldLst)
{
    t_List                          *p_Pos,*p_Pos1;
    t_CcNodeInfo                    *p_CcNodeInfo;
    uint16_t                        nodeIdPrev;
    t_FmPcdCcNode                   *p_FmPcdCcNodePrev;
    t_List                          p_List;


    INIT_LIST(&p_List);

    LIST_FOR_EACH(p_Pos, &p_FmPcdCcNode->ccPrevNodesLst)
    {
        p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos);
        nodeIdPrev = (uint16_t)p_CcNodeInfo->nextCcNodeInfo;
        p_FmPcdCcNodePrev = (t_FmPcdCcNode *)GetNodeHandler(p_FmPcd, nodeIdPrev);
        if(!p_FmPcdCcNodePrev)
            RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("the node with this nodeId wasn't initialized"));
        FindNodeInfoAccIdAndAddToRetLst(&p_FmPcdCcNodePrev->ccNextNodesLst,nodeId, &p_List);
        if(LIST_IsEmpty(&p_List))
            RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("numOfPrevNodes has to be 1"));
        LIST_FOR_EACH(p_Pos1, &p_List)
        {
            p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos1);
            CreateNodeInfo(h_OldLst, (uint32_t)((uint32_t)(p_FmPcdCcNodePrev->h_AdTable) + FM_PCD_CC_AD_ENTRY_SIZE * (p_CcNodeInfo->nextCcNodeInfo >> 16)));
        }
    }

    ReleaseLst(&p_List);
    return E_OK;
}
static t_Error ModifyWithTreeDataStructure(t_FmPcd *p_FmPcd,uint16_t nodeId, t_FmPcdCcNode *p_FmPcdCcNode, t_List  *h_OldLst)
{
    t_List                          *p_Pos,*p_Pos1;
    t_CcNodeInfo                    *p_CcNodeInfo;
    uint16_t                        treeIdPrev;
    t_List                          p_List;
    t_FmPcdCcTree                   *p_FmPcdCcTreePrev;


    INIT_LIST(&p_List);

    LIST_FOR_EACH(p_Pos, &p_FmPcdCcNode->ccTreeIdLst)
    {
        p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos);
        treeIdPrev = (uint16_t)p_CcNodeInfo->nextCcNodeInfo;
        p_FmPcdCcTreePrev = (t_FmPcdCcTree *)FmPcdCcGetTreeHandler(p_FmPcd, (uint8_t)treeIdPrev);
        if(!p_FmPcdCcTreePrev)
            RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("the node with this nodeId wasn't initialized"));
        FindNodeInfoAccIdAndAddToRetLst(&p_FmPcdCcTreePrev->ccNextNodesLst,nodeId , &p_List);
        if(LIST_IsEmpty(&p_List))
            RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("numOfPrevNodes has to be 1"));
        LIST_FOR_EACH(p_Pos1, &p_List)
        {
            p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos1);
            CreateNodeInfo(h_OldLst, (uint32_t)(p_FmPcdCcTreePrev->ccTreeBaseAddr + FM_PCD_CC_AD_ENTRY_SIZE * (p_CcNodeInfo->nextCcNodeInfo >> 16)));
        }
    }
    ReleaseLst(&p_List);
    return E_OK;
}

static t_Error ModifyKeyCommonPart1(t_FmPcdCcNode *p_FmPcdCcNode,  uint8_t keyIndex, t_FmPcdModifyCcKeyAdditionalParams **p_FmPcdModifyCcKeyAdditionalParams)
{
    t_FmPcdModifyCcKeyAdditionalParams *p_FmPcdModifyCcKeyAdditionalParamsTmp;

    SANITY_CHECK_RETURN_ERROR(p_FmPcdCcNode,E_INVALID_HANDLE);

    if(p_FmPcdCcNode->parseCode == CC_PC_FF_IPV4TTL ||
       p_FmPcdCcNode->parseCode == CC_PC_FF_IPV6HOP_LIMIT)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("nodeId of CC_PC_FF_IPV4TTL or CC_PC_FF_IPV6HOP_LIMIT can not be used for addKey, removeKey, modifyKey"));

    if (!LIST_NumOfObjs(&p_FmPcdCcNode->ccPrevNodesLst) &&
        !LIST_NumOfObjs(&p_FmPcdCcNode->ccTreeIdLst))
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("this node without connection"));

    p_FmPcdModifyCcKeyAdditionalParamsTmp =  (t_FmPcdModifyCcKeyAdditionalParams *)XX_Malloc(sizeof(t_FmPcdModifyCcKeyAdditionalParams));
    if(!p_FmPcdModifyCcKeyAdditionalParamsTmp)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("Allocation of internal data structure FAILED"));
    memset(p_FmPcdModifyCcKeyAdditionalParamsTmp, 0, sizeof(t_FmPcdModifyCcKeyAdditionalParams));

    p_FmPcdModifyCcKeyAdditionalParamsTmp->h_CurrentNode = (t_Handle)p_FmPcdCcNode;
    p_FmPcdModifyCcKeyAdditionalParamsTmp->keyIndexForRemove = keyIndex;
    p_FmPcdModifyCcKeyAdditionalParamsTmp->keyIndexForAdd = keyIndex;

    *p_FmPcdModifyCcKeyAdditionalParams = p_FmPcdModifyCcKeyAdditionalParamsTmp;

    return E_OK;
}

t_Error FmPcdCcModifyNextEngineParamTree(t_Handle h_FmPcd, t_Handle h_FmPcdCcTree, uint8_t grpId, uint8_t index, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams, t_Handle *h_OldPointer, t_Handle *h_NewPointer)
{
    t_FmPcdCcTree                   *p_FmPcdCcTree = (t_FmPcdCcTree *)h_FmPcdCcTree;
    t_Error                         err = E_OK;
    t_FmPcdModifyCcAdditionalParams *p_CcOldModifyAdditionalParams;
    t_FmPcdModifyCcAdditionalParams *p_CcNewModifyAdditionalParams;

    SANITY_CHECK_RETURN_ERROR((grpId <= 7),E_INVALID_VALUE);
    SANITY_CHECK_RETURN_ERROR(h_FmPcdCcTree,E_INVALID_VALUE);


    if(grpId >= p_FmPcdCcTree->numOfGrps)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("grpId you asked > numOfGroup of relevant tree"));

    if(index >= p_FmPcdCcTree->fmPcdGroupParam[grpId].numOfEntriesInGroup)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("index > numOfEntriesInGroup"));

    err = ModifyCcCommon1(h_FmPcd, p_FmPcdCcNextEngineParams, h_OldPointer, h_NewPointer);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    p_CcOldModifyAdditionalParams = *h_OldPointer;
    p_CcOldModifyAdditionalParams->p_Ad =
        CAST_UINT64_TO_POINTER(p_FmPcdCcTree->ccTreeBaseAddr + FM_PCD_CC_AD_ENTRY_SIZE* (p_FmPcdCcTree->fmPcdGroupParam[grpId].baseGroupEntry + index));
    p_CcOldModifyAdditionalParams->isTree = TRUE;
    p_CcOldModifyAdditionalParams->h_Node = p_FmPcdCcTree;
    p_CcNewModifyAdditionalParams  = *h_NewPointer;
    p_CcNewModifyAdditionalParams->isTree = TRUE;
    p_CcNewModifyAdditionalParams->h_Node = p_FmPcdCcTree;

    ModifyCcCommon2(p_CcOldModifyAdditionalParams, p_CcNewModifyAdditionalParams, (uint8_t)(p_FmPcdCcTree->fmPcdGroupParam[grpId].baseGroupEntry + index));

    return E_OK;
}


static t_Error ModifyKeyCommonPart2(t_FmPcd *p_FmPcd, t_FmPcdCcNode *p_FmPcdCcNode, t_FmPcdModifyCcKeyAdditionalParams *p_FmPcdModifyCcKeyAdditionalParams ,t_List *h_OldLst, t_Handle *h_NewPointer)
{

    t_Error         err = E_OK;
    t_FmPcdCcNode   fmPcdCcNode;

    if(!LIST_IsEmpty(&p_FmPcdCcNode->ccPrevNodesLst))
    {
        err =  ModifyWithNodeDataStructure(p_FmPcd,p_FmPcdCcNode->nodeId, p_FmPcdCcNode,h_OldLst);
        if(err)
            RETURN_ERROR(MAJOR, err, NO_MSG);
    }
    if(!LIST_IsEmpty(&p_FmPcdCcNode->ccTreeIdLst))
    {
        err =  ModifyWithTreeDataStructure(p_FmPcd,p_FmPcdCcNode->nodeId, p_FmPcdCcNode,h_OldLst);
        if(err)
        {
            ReleaseLst(h_OldLst);
            RETURN_ERROR(MAJOR, err, NO_MSG);
        }

    }

    memset(&fmPcdCcNode, 0, sizeof(t_FmPcdCcNode));
    FillNodeWithParams(&fmPcdCcNode, p_FmPcdCcNode, p_FmPcdModifyCcKeyAdditionalParams);
    err = ModifyCcKeyCommon(p_FmPcd,h_NewPointer, TRUE);
    if(err)
    {
        ReleaseLst(h_OldLst);
        ReleaseCommonModifyKey((p_FmPcd->p_FmPcdCc)->h_FmMuram,(t_FmPcdModifyCcAdditionalParams *)*h_NewPointer);
    }
    FillAdOfTypeContLookup(((t_FmPcdModifyCcAdditionalParams *)*h_NewPointer)->p_Ad, p_FmPcd,&fmPcdCcNode);

    ((t_FmPcdModifyCcAdditionalParams *)*h_NewPointer)->h_AdditionalInfo = p_FmPcdModifyCcKeyAdditionalParams;

    return E_OK;
}


t_Error FmPcdCcAddKey(t_Handle h_FmPcd, t_Handle h_FmPcdCcNode, uint8_t keyIndex, t_FmPcdCcKeyParams *p_FmPcdCcKeyParams, t_List *h_OldLst, t_Handle *h_NewPointer)
{
    t_FmPcdCcNode                   *p_FmPcdCcNode = (t_FmPcdCcNode *)h_FmPcdCcNode;
    t_FmPcd                         *p_FmPcd = (t_FmPcd*)h_FmPcd;
    t_Error                         err = E_OK;
    t_FmPcdModifyCcKeyAdditionalParams *p_FmPcdModifyCcKeyAdditionalParams;

    err =  ModifyKeyCommonPart1(p_FmPcdCcNode, keyIndex, &p_FmPcdModifyCcKeyAdditionalParams);
    if(err)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, NO_MSG);

    if(keyIndex > p_FmPcdCcNode->numOfKeys)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("keyIndex > previousely cleared last index + 1"));

    if((p_FmPcdCcNode->numOfKeys + 1) > FM_PCD_MAX_NUM_OF_CC_NODES)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("numOfKeys with new key can not be larger than 255"));

    err = BuildNewNodeAddKey (h_FmPcd, p_FmPcdCcNode, keyIndex, p_FmPcdCcKeyParams, p_FmPcdModifyCcKeyAdditionalParams);
    if(err)
    {
        XX_Free(p_FmPcdModifyCcKeyAdditionalParams);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    err = ModifyKeyCommonPart2(p_FmPcd, p_FmPcdCcNode, p_FmPcdModifyCcKeyAdditionalParams, h_OldLst, h_NewPointer);
    if(err)
    {
            ReleaseNewNodeCommonPart(p_FmPcd->p_FmPcdCc, p_FmPcdModifyCcKeyAdditionalParams);
            XX_Free(p_FmPcdModifyCcKeyAdditionalParams);
            RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    return E_OK;
}

t_Error FmPcdCcRemoveKey(t_Handle h_FmPcd, t_Handle h_FmPcdCcNode, uint8_t keyIndex,t_List *h_OldLst, t_Handle *h_NewPointer)
{

    t_FmPcdCcNode                   *p_FmPcdCcNode = (t_FmPcdCcNode *) h_FmPcdCcNode;
    t_FmPcd                         *p_FmPcd = (t_FmPcd*)h_FmPcd;
    t_Error                         err = E_OK;
    t_FmPcdModifyCcKeyAdditionalParams *p_FmPcdModifyCcKeyAdditionalParams;


    err = ModifyKeyCommonPart1(p_FmPcdCcNode, keyIndex, &p_FmPcdModifyCcKeyAdditionalParams);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    if(keyIndex >= p_FmPcdCcNode->numOfKeys)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("impossible to remove key from numOfKeys = 0"));

    if(!p_FmPcdCcNode->numOfKeys)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("keyIndex you asked > numOfKeys of relevant node that was initialized"));

    err = BuildNewNodeRemoveKey (h_FmPcd, p_FmPcdCcNode, keyIndex, p_FmPcdModifyCcKeyAdditionalParams);
    if(err)
    {
        XX_Free(p_FmPcdModifyCcKeyAdditionalParams);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    err = ModifyKeyCommonPart2(p_FmPcd, p_FmPcdCcNode, p_FmPcdModifyCcKeyAdditionalParams, h_OldLst, h_NewPointer);
    if(err)
    {
        ReleaseNewNodeCommonPart(p_FmPcd->p_FmPcdCc, p_FmPcdModifyCcKeyAdditionalParams);
        XX_Free(p_FmPcdModifyCcKeyAdditionalParams);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

 return E_OK;
}

t_Error FmPcdCcModifyKey(t_Handle h_FmPcd, t_Handle h_FmPcdCcNode, uint8_t keyIndex, uint8_t *p_Key, uint8_t *p_Mask, t_List *h_OldLst, t_Handle *h_NewPointer)
{
    t_FmPcdCcNode                   *p_FmPcdCcNode = (t_FmPcdCcNode *)h_FmPcdCcNode;
    t_FmPcd                         *p_FmPcd = (t_FmPcd*)h_FmPcd;
    t_Error                         err = E_OK;
    t_FmPcdModifyCcKeyAdditionalParams *p_FmPcdModifyCcKeyAdditionalParams;

    err = ModifyKeyCommonPart1(p_FmPcdCcNode, keyIndex, &p_FmPcdModifyCcKeyAdditionalParams);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    if(keyIndex >= p_FmPcdCcNode->numOfKeys)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("keyIndex you asked > numOfKeys of relevant node that was initialized"));

    err = BuildNewNodeModifyKey (h_FmPcd, p_FmPcdCcNode, keyIndex, p_Key, p_Mask, p_FmPcdModifyCcKeyAdditionalParams);
    if(err)
    {
        XX_Free(p_FmPcdModifyCcKeyAdditionalParams);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    err = ModifyKeyCommonPart2(p_FmPcd, p_FmPcdCcNode, p_FmPcdModifyCcKeyAdditionalParams, h_OldLst, h_NewPointer);
    if(err)
    {
        ReleaseNewNodeCommonPart(p_FmPcd->p_FmPcdCc, p_FmPcdModifyCcKeyAdditionalParams);
        XX_Free(p_FmPcdModifyCcKeyAdditionalParams);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

 return E_OK;
}

t_Error FmPcdCcModifyKeyAndNextEngine(t_Handle h_FmPcd, t_Handle h_FmPcdNode, uint8_t keyIndex, t_FmPcdCcKeyParams *p_FmPcdCcKeyParams, t_List *h_OldLst, t_Handle *h_NewPointer)
{
    t_FmPcdCcNode                   *p_FmPcdCcNode = (t_FmPcdCcNode *)h_FmPcdNode;
    t_FmPcd                         *p_FmPcd = (t_FmPcd*)h_FmPcd;
    t_Error                         err = E_OK;
    t_FmPcdModifyCcKeyAdditionalParams *p_FmPcdModifyCcKeyAdditionalParams;

    err = ModifyKeyCommonPart1(p_FmPcdCcNode, keyIndex, &p_FmPcdModifyCcKeyAdditionalParams);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    if(keyIndex >= p_FmPcdCcNode->numOfKeys)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("keyIndex you asked > numOfKeys of relevant node that was initialized"));


    err = BuildNewNodeModifyKeyAndNextEngine (h_FmPcd, p_FmPcdCcNode, keyIndex, p_FmPcdCcKeyParams, p_FmPcdModifyCcKeyAdditionalParams);
    if(err)
    {
        XX_Free(p_FmPcdModifyCcKeyAdditionalParams);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    err = ModifyKeyCommonPart2(p_FmPcd, p_FmPcdCcNode, p_FmPcdModifyCcKeyAdditionalParams, h_OldLst, h_NewPointer);
    if(err)
    {
        ReleaseNewNodeCommonPart(p_FmPcd->p_FmPcdCc, p_FmPcdModifyCcKeyAdditionalParams);
        XX_Free(p_FmPcdModifyCcKeyAdditionalParams);
        RETURN_ERROR(MAJOR, err, NO_MSG);
    }

 return E_OK;
}



t_Error FmPcdCcModiyNextEngineParamNode(t_Handle h_FmPcd,t_Handle h_FmPcdCcNode, uint8_t keyIndex,t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams,t_Handle *h_OldPointer, t_Handle *h_NewPointer)
{
    t_FmPcdModifyCcAdditionalParams *p_CcOldModifyAdditionalParams;
    t_FmPcdModifyCcAdditionalParams *p_CcNewModifyAdditionalParams;
    t_FmPcdCcNode                   *p_FmPcdCcNode = (t_FmPcdCcNode *)h_FmPcdCcNode;
    t_Error                         err = E_OK;

    SANITY_CHECK_RETURN_ERROR(h_FmPcd,E_INVALID_VALUE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcdCcNode,E_INVALID_HANDLE);


    if(keyIndex >= p_FmPcdCcNode->numOfKeys)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("keyIndex you asked > numOfKeys of relevant node that was initialized"));

    err = ModifyCcCommon1(h_FmPcd, p_FmPcdCcNextEngineParams, h_OldPointer, h_NewPointer);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);


    p_CcOldModifyAdditionalParams = *h_OldPointer;
    p_CcOldModifyAdditionalParams->p_Ad = (t_Handle)((uint32_t)p_FmPcdCcNode->h_AdTable + FM_PCD_CC_AD_ENTRY_SIZE * keyIndex);
    p_CcOldModifyAdditionalParams->h_Node = (t_Handle)p_FmPcdCcNode;

    p_CcNewModifyAdditionalParams  = *h_NewPointer;
    p_CcNewModifyAdditionalParams->h_Node = (t_Handle)p_FmPcdCcNode;
    ModifyCcCommon2(p_CcOldModifyAdditionalParams,p_CcNewModifyAdditionalParams, keyIndex);

    return E_OK;
}

t_Error FmPcdCcModifyMissNextEngineParamNode(t_Handle h_FmPcd, t_Handle h_FmPcdCcNode, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams,t_Handle *h_OldPointer, t_Handle *h_NewPointer)
{
    t_FmPcdModifyCcAdditionalParams *p_CcOldModifyAdditionalParams;
    t_FmPcdModifyCcAdditionalParams *p_CcNewModifyAdditionalParams;
    t_FmPcdCcNode                   *p_FmPcdCcNode = (t_FmPcdCcNode *)h_FmPcdCcNode;
    t_Error                         err = E_OK;

    SANITY_CHECK_RETURN_ERROR(p_FmPcdCcNode,E_INVALID_VALUE);

    err = ModifyCcCommon1(h_FmPcd, p_FmPcdCcNextEngineParams, h_OldPointer, h_NewPointer);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);


    p_CcOldModifyAdditionalParams = *h_OldPointer;
    p_CcOldModifyAdditionalParams->p_Ad = (t_Handle)((uint32_t)p_FmPcdCcNode->h_AdTable + FM_PCD_CC_AD_ENTRY_SIZE * p_FmPcdCcNode->numOfKeys);
    p_CcOldModifyAdditionalParams->h_Node = (t_Handle)p_FmPcdCcNode;

    p_CcNewModifyAdditionalParams  = *h_NewPointer;
    p_CcOldModifyAdditionalParams->h_Node = (t_Handle)p_FmPcdCcNode;
    ModifyCcCommon2(p_CcOldModifyAdditionalParams,p_CcNewModifyAdditionalParams,p_FmPcdCcNode->numOfKeys);

    return E_OK;
}
static t_Error UpdateNodesWithTree(t_Handle h_FmPcd,t_List *ccNextNodesLst, uint16_t *p_CcArray, uint8_t treeId)
{
    t_List          *p_Pos;
    t_Error         err = E_OK;
    t_FmPcdCcNode   *p_FmPcdCcNode;
    uint32_t        nodeIdTmp;
    if(!LIST_IsEmpty(ccNextNodesLst))
     {
         LIST_FOR_EACH(p_Pos, ccNextNodesLst)
         {
             nodeIdTmp = ((t_CcNodeInfo *)CC_NEXT_NODE_F_OBJECT(p_Pos))->nextCcNodeInfo;
             p_FmPcdCcNode = (t_FmPcdCcNode *)GetNodeHandler((t_FmPcd *)h_FmPcd, (uint16_t)nodeIdTmp);
             if(!p_FmPcdCcNode)
                 RETURN_ERROR(MAJOR, E_INVALID_STATE, NO_MSG);
             if(p_CcArray[(uint16_t)nodeIdTmp] ==0)
             {
                 err = UpdateNodesWithTree(h_FmPcd, &p_FmPcdCcNode->ccNextNodesLst,p_CcArray, treeId);
                 if(err)
                     RETURN_ERROR(MAJOR, E_INVALID_STATE, NO_MSG);
                 CreateNodeInfo(&p_FmPcdCcNode->ccTreesLst, (uint32_t)treeId);
                 p_CcArray[(uint16_t)nodeIdTmp] = 1;
             }
         }
     }
    return E_OK;
}

static t_Error RemoveNodesFromTree(t_Handle h_FmPcd,t_List *ccNextNodesLst, uint8_t treeId)
{
    t_List          *p_Pos;
    t_Error         err = E_OK;
    t_FmPcdCcNode   *p_FmPcdCcNode;
    uint32_t        nodeIdTmp;
    t_CcNodeInfo    *p_CcNodeInfo;
    if(!LIST_IsEmpty(ccNextNodesLst))
    {
        LIST_FOR_EACH(p_Pos, ccNextNodesLst)
        {
            nodeIdTmp = ((t_CcNodeInfo *)CC_NEXT_NODE_F_OBJECT(p_Pos))->nextCcNodeInfo;
            p_FmPcdCcNode = GetNodeHandler((t_FmPcd *)h_FmPcd, (uint16_t)nodeIdTmp);
            if(!p_FmPcdCcNode)
                RETURN_ERROR(MAJOR, E_INVALID_STATE, NO_MSG);

            err = RemoveNodesFromTree(h_FmPcd, &p_FmPcdCcNode->ccNextNodesLst, treeId);
            if(err)
                RETURN_ERROR(MAJOR, E_INVALID_STATE, NO_MSG);
            p_CcNodeInfo = FindNodeInfoAccId(&p_FmPcdCcNode->ccTreesLst, treeId);
            ASSERT_COND(p_CcNodeInfo);
            LIST_DelAndInit(&p_CcNodeInfo->h_Node);
            XX_Free(p_CcNodeInfo);
        }
    }

    return E_OK;
}


t_Error FmPcdCcReleaseModifiedOnlyNextEngine(t_Handle h_FmPcd, t_Handle h_FmPcdOldPointer, t_Handle h_FmPcdNewPointer, bool isAllGood)
{
    t_FmPcdModifyCcAdditionalParams *p_CcOldModifyAdditionalParams = (t_FmPcdModifyCcAdditionalParams *)h_FmPcdOldPointer;
    t_FmPcdModifyCcAdditionalParams *p_CcNewModifyAdditionalParams = (t_FmPcdModifyCcAdditionalParams *)h_FmPcdNewPointer;
    t_FmPcd                         *p_FmPcd = (t_FmPcd *)h_FmPcd;
    t_FmPcdCcTree                   *p_CurrentTree=NULL;
    t_FmPcdCcNode                   *p_CurrentNode=NULL, *p_NodeForAdd, *p_NodeForRemove;
    t_CcNodeInfo                    *p_CcNodeInfo, *p_CcNodeInfo1;
    uint16_t                         numOfReplec;
    t_Error                          err = E_OK;
    t_List                          *p_Pos;
    uint16_t                        ccArray[FM_PCD_MAX_NUM_OF_CC_NODES];

    SANITY_CHECK_RETURN_ERROR(h_FmPcd,E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc,E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(h_FmPcdOldPointer,E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_CcNewModifyAdditionalParams->h_Node,E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_CcOldModifyAdditionalParams->h_Node,E_INVALID_HANDLE);

    if(isAllGood)
    {
            if(p_CcNewModifyAdditionalParams->isTree)
                p_CurrentTree = (t_FmPcdCcTree*)p_CcNewModifyAdditionalParams->h_Node;
            else
                p_CurrentNode = (t_FmPcdCcNode*)p_CcNewModifyAdditionalParams->h_Node;
           if(p_CurrentTree)
            {
                    if(p_CcNewModifyAdditionalParams->myInfo != 0xffffffff)
                    {
                        p_NodeForAdd = GetNodeHandler(p_FmPcd, (uint16_t)p_CcNewModifyAdditionalParams->myInfo);
                        ASSERT_COND(p_NodeForAdd);
                        CreateNodeInfo(&p_CurrentTree->ccNextNodesLst, p_CcNewModifyAdditionalParams->myInfo);
                        p_CcNodeInfo = FindNodeInfoAccId(&p_NodeForAdd->ccTreeIdLst, p_CurrentTree->treeId);
                        if(p_CcNodeInfo)
                        {
                            numOfReplec = (uint16_t)(p_CcNodeInfo->nextCcNodeInfo >> 16);
                            p_CcNodeInfo->nextCcNodeInfo = ((p_CcNodeInfo->nextCcNodeInfo & 0x0000ffff) | ((uint32_t)(numOfReplec+1) <<16));
                        }
                        else
                            CreateNodeInfo(&p_NodeForAdd->ccTreeIdLst, (uint32_t)((uint32_t)p_CurrentTree->treeId | ((uint32_t)1<<16)));
                        memset(ccArray, 0, sizeof(uint16_t) * FM_PCD_MAX_NUM_OF_CC_NODES);
                        err = UpdateNodesWithTree(h_FmPcd, &p_NodeForAdd->ccNextNodesLst, ccArray, p_CurrentTree->treeId);
                        if(err)
                            RETURN_ERROR(MAJOR, err, NO_MSG);
                    }
                    if(p_CcOldModifyAdditionalParams->myInfo != 0xffffffff)
                    {
                        p_CcNodeInfo = FindNodeInfoAccIndex(&p_CurrentTree->ccNextNodesLst, (uint16_t)p_CcOldModifyAdditionalParams->myInfo);
                        ASSERT_COND(p_CcNodeInfo);
                        p_NodeForRemove = GetNodeHandler(p_FmPcd, (uint16_t)p_CcNodeInfo->nextCcNodeInfo);
                        ASSERT_COND(p_NodeForRemove);
                        UpdateNodeOwner(p_FmPcd, p_NodeForRemove->nodeId, FALSE);
                        p_CcNodeInfo1 = FindNodeInfoAccId(&p_NodeForRemove->ccTreeIdLst, p_CurrentTree->treeId);
                        ASSERT_COND(p_CcNodeInfo1);
                        numOfReplec = (uint16_t)(p_CcNodeInfo1->nextCcNodeInfo >> 16);
                        ASSERT_COND(numOfReplec);
                        numOfReplec -=1;
                        if(numOfReplec)
                            p_CcNodeInfo1->nextCcNodeInfo = (uint32_t)((p_CcNodeInfo1->nextCcNodeInfo & 0x0000ffff) | (numOfReplec << 16));
                        else
                        {
                            LIST_DelAndInit(&p_CcNodeInfo1->h_Node);
                            XX_Free(p_CcNodeInfo1);
                        }
                        LIST_DelAndInit(&p_CcNodeInfo->h_Node);
                        XX_Free(p_CcNodeInfo);
                        err = RemoveNodesFromTree(h_FmPcd, &p_NodeForRemove->ccNextNodesLst, p_CurrentTree->treeId);
                        if(err)
                            RETURN_ERROR(MAJOR, err, NO_MSG);
                     }
            }
            else if(p_CurrentNode)
            {
                   if(p_CcNewModifyAdditionalParams->myInfo != 0xffffffff)
                    {
                        p_NodeForAdd = GetNodeHandler(p_FmPcd, (uint16_t)p_CcNewModifyAdditionalParams->myInfo);
                        ASSERT_COND(p_NodeForAdd);
                        CreateNodeInfo(&p_CurrentNode->ccNextNodesLst, p_CcNewModifyAdditionalParams->myInfo);
                        p_CcNodeInfo = FindNodeInfoAccId(&p_NodeForAdd->ccPrevNodesLst, p_CurrentNode->nodeId);
                        if(p_CcNodeInfo)
                        {
                            numOfReplec = (uint16_t)(p_CcNodeInfo->nextCcNodeInfo >> 16);
                            p_CcNodeInfo->nextCcNodeInfo = ((p_CcNodeInfo->nextCcNodeInfo & 0x0000ffff) | ((uint32_t)(numOfReplec+1) <<16));
                        }
                        else
                            CreateNodeInfo(&p_NodeForAdd->ccPrevNodesLst, (uint32_t)((uint32_t)p_CurrentNode->nodeId | ((uint32_t)1<<16)));

                        LIST_FOR_EACH(p_Pos, &p_CurrentNode->ccTreesLst)
                        {
                            p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos);
                            memset(ccArray, 0, sizeof(uint16_t)*FM_PCD_MAX_NUM_OF_CC_NODES);
                            err = UpdateNodesWithTree(p_FmPcd, &p_NodeForAdd->ccNextNodesLst, ccArray, (uint8_t)p_CcNodeInfo->nextCcNodeInfo);
                            if(err)
                                RETURN_ERROR(MAJOR, err, NO_MSG);
                        }
                    }
                    if(p_CcOldModifyAdditionalParams->myInfo != 0xffffffff)
                    {
                        p_CcNodeInfo = FindNodeInfoAccIndex(&p_CurrentNode->ccNextNodesLst, (uint16_t)p_CcOldModifyAdditionalParams->myInfo);
                        ASSERT_COND(p_CcNodeInfo);
                        p_NodeForRemove = GetNodeHandler(p_FmPcd, (uint16_t)p_CcNodeInfo->nextCcNodeInfo);
                        ASSERT_COND(p_NodeForRemove);
                        p_CcNodeInfo1 = FindNodeInfoAccId(&p_NodeForRemove->ccPrevNodesLst, p_CurrentNode->nodeId);
                        UpdateNodeOwner(p_FmPcd, p_NodeForRemove->nodeId, FALSE);
                        ASSERT_COND(p_CcNodeInfo1);
                        numOfReplec = (uint16_t)(p_CcNodeInfo1->nextCcNodeInfo >> 16);
                        ASSERT_COND(numOfReplec);
                        numOfReplec -=1;
                        if(numOfReplec)
                            p_CcNodeInfo1->nextCcNodeInfo = (uint32_t)((p_CcNodeInfo1->nextCcNodeInfo & 0x0000ffff) | (numOfReplec << 16));
                        else
                        {
                            LIST_DelAndInit(&p_CcNodeInfo1->h_Node);
                            XX_Free(p_CcNodeInfo1);
                        }
                        LIST_DelAndInit(&p_CcNodeInfo->h_Node);
                        XX_Free(p_CcNodeInfo);
                        LIST_FOR_EACH(p_Pos, &p_CurrentNode->ccTreesLst)
                        {
                            p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos);
                            err = RemoveNodesFromTree(h_FmPcd, &p_NodeForRemove->ccNextNodesLst, (uint8_t)p_CcNodeInfo->nextCcNodeInfo);
                            if(err)
                                RETURN_ERROR(MAJOR, err, NO_MSG);
                        }
                  }
            }
       }
         else
         {
        if(p_CcNewModifyAdditionalParams->myInfo != 0xffffffff)
            UpdateNodeOwner(p_FmPcd, (uint16_t)p_CcNewModifyAdditionalParams->myInfo, FALSE);
       }

    if(p_CcNewModifyAdditionalParams->p_Ad)
        FM_MURAM_FreeMem(p_FmPcd->p_FmPcdCc->h_FmMuram,p_CcNewModifyAdditionalParams->p_Ad);

    XX_Free(h_FmPcdNewPointer);
    XX_Free(h_FmPcdOldPointer);

    return E_OK;
}
t_Error FmPcdCcReleaseModifiedKey(t_Handle h_FmPcd, t_List *h_FmPcdOldPointersLst, t_Handle h_FmPcdNewPointer, uint16_t numOfGoodChanges)
{
    t_FmPcdModifyCcAdditionalParams *p_CcNewModifyAdditionalParams = (t_FmPcdModifyCcAdditionalParams *)h_FmPcdNewPointer;
    t_FmPcd                         *p_FmPcd = (t_FmPcd *)h_FmPcd;
    t_CcNodeInfo                    *p_CcNodeInfo, *p_CcNodeInfo1;
    t_FmPcdModifyCcKeyAdditionalParams *p_FmPcdCcKeyAdditionalParams;
    t_FmPcdCcNode                  *p_CurrentNode = NULL, *p_NodeForAdd, *p_NodeForRemove;
    uint32_t                        numOfReplec;
    t_List                          *p_Pos;
    t_Error                         err = E_OK;
    uint16_t                        ccArray[FM_PCD_MAX_NUM_OF_CC_NODES];

    UNUSED(numOfGoodChanges);

    SANITY_CHECK_RETURN_ERROR(h_FmPcd,E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc,E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(h_FmPcdOldPointersLst,E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(h_FmPcdNewPointer,E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR((numOfGoodChanges == LIST_NumOfObjs(h_FmPcdOldPointersLst)),E_INVALID_STATE);
    SANITY_CHECK_RETURN_ERROR(p_CcNewModifyAdditionalParams->h_AdditionalInfo,E_INVALID_HANDLE);

            p_FmPcdCcKeyAdditionalParams  = (t_FmPcdModifyCcKeyAdditionalParams *)p_CcNewModifyAdditionalParams->h_AdditionalInfo;
            p_CurrentNode = (t_FmPcdCcNode *)p_FmPcdCcKeyAdditionalParams->h_CurrentNode;
            if(p_FmPcdCcKeyAdditionalParams->nodeIdForAdd != 0xffff)
            {
                p_NodeForAdd = GetNodeHandler(p_FmPcd, p_FmPcdCcKeyAdditionalParams->nodeIdForAdd);
                ASSERT_COND(p_NodeForAdd);
                p_CcNodeInfo = FindNodeInfoAccId(&p_NodeForAdd->ccPrevNodesLst, p_CurrentNode->nodeId);
                if(p_CcNodeInfo)
                {
                    numOfReplec = p_CcNodeInfo->nextCcNodeInfo >> 16;
                    numOfReplec +=1;
                    p_CcNodeInfo->nextCcNodeInfo = (uint32_t)((p_CcNodeInfo->nextCcNodeInfo & 0x0000ffff) | (numOfReplec << 16));
                }
                else
                {
                    numOfReplec = (uint32_t)p_CurrentNode->nodeId | ((uint32_t)1<<16);
                    CreateNodeInfo(&p_NodeForAdd->ccPrevNodesLst, numOfReplec);
                }
                CreateNodeInfo(&p_CurrentNode->ccNextNodesLst, (((uint32_t)p_FmPcdCcKeyAdditionalParams->keyIndexForAdd << 16) |(uint32_t)p_FmPcdCcKeyAdditionalParams->nodeIdForAdd));
                LIST_FOR_EACH(p_Pos, &p_CurrentNode->ccTreesLst)
                {
                    p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos);
                    memset(ccArray, 0, sizeof(uint16_t)*FM_PCD_MAX_NUM_OF_CC_NODES);
                    err = UpdateNodesWithTree(p_FmPcd, &p_NodeForAdd->ccNextNodesLst, ccArray, (uint8_t)p_CcNodeInfo->nextCcNodeInfo);
                    if(err)
                        RETURN_ERROR(MAJOR, err, NO_MSG);
                }
            }
            if(p_FmPcdCcKeyAdditionalParams->keyIndexForRemove != 0xffff)
            {
                p_CcNodeInfo =  FindNodeInfoAccIndex(&p_CurrentNode->ccNextNodesLst, p_FmPcdCcKeyAdditionalParams->keyIndexForRemove);
                ASSERT_COND(p_CcNodeInfo);
                p_NodeForRemove = GetNodeHandler(p_FmPcd, (uint16_t)p_CcNodeInfo->nextCcNodeInfo);
                UpdateNodeOwner(p_FmPcd, (uint16_t)p_CcNodeInfo->nextCcNodeInfo, FALSE);
                p_CcNodeInfo1 = FindNodeInfoAccId(&p_NodeForRemove->ccPrevNodesLst, p_CurrentNode->nodeId);
                numOfReplec = p_CcNodeInfo1->nextCcNodeInfo >> 16;
                ASSERT_COND(numOfReplec);
                numOfReplec -=1;
                if(numOfReplec)
                    p_CcNodeInfo->nextCcNodeInfo = (uint32_t)((p_CcNodeInfo1->nextCcNodeInfo & 0x0000ffff) | (numOfReplec << 16));
                else
                {
                    LIST_DelAndInit(&p_CcNodeInfo1->h_Node);
                    XX_Free(p_CcNodeInfo1);
                }
                LIST_DelAndInit(&p_CcNodeInfo->h_Node);
                XX_Free(p_CcNodeInfo);
                LIST_FOR_EACH(p_Pos, &p_CurrentNode->ccTreesLst)
                {
                    p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos);
                    err = RemoveNodesFromTree(h_FmPcd, &p_NodeForRemove->ccNextNodesLst, (uint8_t)p_CcNodeInfo->nextCcNodeInfo);
                    if(err)
                        RETURN_ERROR(MAJOR, err, NO_MSG);
                 }
              }

            ASSERT_COND(p_FmPcdCcKeyAdditionalParams->p_AdTableOld);
            FM_MURAM_FreeMem(p_FmPcd->p_FmPcdCc->h_FmMuram,p_FmPcdCcKeyAdditionalParams->p_AdTableOld);
            ASSERT_COND(p_FmPcdCcKeyAdditionalParams->p_KeysMatchTableOld);
            FM_MURAM_FreeMem(p_FmPcd->p_FmPcdCc->h_FmMuram,p_FmPcdCcKeyAdditionalParams->p_KeysMatchTableOld);

            p_CurrentNode->h_AdTable    = p_FmPcdCcKeyAdditionalParams->p_AdTableNew;
            p_CurrentNode->numOfKeys    = p_FmPcdCcKeyAdditionalParams->numOfKeys;
            p_CurrentNode->lclMask      = p_FmPcdCcKeyAdditionalParams->lclMask;
            p_CurrentNode->h_KeysMatchTable = p_FmPcdCcKeyAdditionalParams->p_KeysMatchTableNew;

            XX_Free(p_CcNewModifyAdditionalParams->h_AdditionalInfo);
            p_CcNewModifyAdditionalParams->h_AdditionalInfo = NULL;

    ReleaseLst(h_FmPcdOldPointersLst);

    if(p_CcNewModifyAdditionalParams->p_Ad)
        FM_MURAM_FreeMem(p_FmPcd->p_FmPcdCc->h_FmMuram,p_CcNewModifyAdditionalParams->p_Ad);

    XX_Free(h_FmPcdNewPointer);

    return E_OK;
}

uint32_t FmPcdCcGetNodeAddrOffset(t_Handle h_FmPcd, t_Handle h_Pointer)
{
    t_FmPcd                         *p_FmPcd = (t_FmPcd *)h_FmPcd;

    SANITY_CHECK_RETURN_VALUE(h_FmPcd,E_INVALID_HANDLE, (uint32_t)ILLEGAL_BASE);
    SANITY_CHECK_RETURN_VALUE(p_FmPcd->p_FmPcdCc,E_INVALID_HANDLE, (uint32_t)ILLEGAL_BASE);

    return (uint32_t)((uint64_t)(XX_VirtToPhys(((t_FmPcdModifyCcAdditionalParams *)h_Pointer)->p_Ad)) -
                     p_FmPcd->p_FmPcdCc->physicalMuramBase);
}

uint32_t FmPcdCcGetNodeAddrOffsetFromNodeInfo(t_Handle h_FmPcd, t_Handle h_Pointer)
{
    t_FmPcd                         *p_FmPcd = (t_FmPcd *)h_FmPcd;
    t_CcNodeInfo                    *p_CcNodeInfo;

    SANITY_CHECK_RETURN_VALUE(h_FmPcd,E_INVALID_HANDLE, (uint32_t)ILLEGAL_BASE);
    SANITY_CHECK_RETURN_VALUE(p_FmPcd->p_FmPcdCc,E_INVALID_HANDLE, (uint32_t)ILLEGAL_BASE);

    p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(h_Pointer);
    return (uint32_t)((uint64_t)(XX_VirtToPhys(CAST_UINT32_TO_POINTER(p_CcNodeInfo->nextCcNodeInfo))) -
                      p_FmPcd->p_FmPcdCc->physicalMuramBase);
}

static t_Error  FmPcdCcUpdateTreeOwner(t_Handle h_FmPcd, uint8_t treeId, bool add)
{


    t_FmPcd *p_FmPcd = (t_FmPcd*)h_FmPcd;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd,E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc,E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc->ccTreeArrayEntry[treeId].p_FmPcdCcTree,E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR((treeId < FM_PCD_MAX_NUM_OF_CC_TREES), E_INVALID_VALUE);

    if(add)
        p_FmPcd->p_FmPcdCc->ccTreeArrayEntry[treeId].owners++;
    else
    {
        if(!p_FmPcd->p_FmPcdCc->ccTreeArrayEntry[treeId].owners)
            RETURN_ERROR(MAJOR, E_INVALID_SELECTION, ("this tree wasn't assighned before"))  ;
        p_FmPcd->p_FmPcdCc->ccTreeArrayEntry[treeId].owners--;
    }

    return E_OK;
}

t_Error     CcGetGrpParams(t_Handle h_FmPcdCcTree, uint8_t grpId, uint32_t *p_GrpBits, uint8_t *p_GrpBase)
{
    t_FmPcdCcTree *p_FmPcdCcTree = (t_FmPcdCcTree *) h_FmPcdCcTree;

    SANITY_CHECK_RETURN_ERROR(h_FmPcdCcTree, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR((grpId <= 7),E_INVALID_STATE);

    if(grpId >= p_FmPcdCcTree->numOfGrps)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("grpId you asked > numOfGroup of relevant tree"));
    *p_GrpBits = p_FmPcdCcTree->fmPcdGroupParam[grpId].totalBitsMask;
    *p_GrpBase = p_FmPcdCcTree->fmPcdGroupParam[grpId].baseGroupEntry;
    return E_OK;
}

t_Handle CcConfig(t_FmPcd *p_FmPcd, t_FmPcdParams *p_FmPcdParams)
{
    t_FmPcdCc           *p_FmPcdCc;
#ifdef CONFIG_GUEST_PARTITION
    t_FmPcdIcPhysAddr   physicalMuramBase;
    t_Error             err = E_OK;
#else
    t_FmPhysAddr        physicalMuramBase;
#endif /* CONFIG_GUEST_PARTITION */

    UNUSED(p_FmPcd);
    p_FmPcdCc = (t_FmPcdCc *) XX_Malloc(sizeof(t_FmPcdCc));
    if (!p_FmPcdCc)
    {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FM Cc allocation FAILED"));
        return NULL;
    }
    memset(p_FmPcdCc, 0, sizeof(t_FmPcdCc));

    p_FmPcdCc->h_FmMuram = p_FmPcdParams->h_FmMuram;
#ifndef CONFIG_GUEST_PARTITION
    FmGetPhysicalMuramBase(p_FmPcdParams->h_Fm, &physicalMuramBase);
#else
    err = XX_SendMessage(p_FmPcd->fmPcdModuleName, FM_PCD_GET_PHYS_MURAM_BASE, (uint8_t*)&physicalMuramBase, NULL, NULL);
    if(err)
    {
        REPORT_ERROR(MINOR, err, NO_MSG);
    }
#endif /* CONFIG_GUEST_PARTITION */
    p_FmPcdCc->physicalMuramBase = (uint64_t)((uint64_t)(&physicalMuramBase)->low | ((uint64_t)(&physicalMuramBase)->high << 32));

    return p_FmPcdCc;
}

void CcFree(t_FmPcdCc *p_FmPcdCc)
{

    int i = 0;
    for (i = 0; i < FM_PCD_MAX_NUM_OF_CC_NODES; i++)
        ReleaseNode(p_FmPcdCc, (uint16_t)i);

    for(i = 0; i < FM_PCD_MAX_NUM_OF_CC_TREES; i++)
        ReleaseTree(p_FmPcdCc, (uint8_t)i);
}

t_Error  FmPcdCcBindTree(t_Handle h_FmPcd, t_Handle  h_FmPcdCcTree,  uint32_t  *p_Offset)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd*)h_FmPcd;
    t_FmPcdCcTree       *p_FmPcdCcTree = (t_FmPcdCcTree *)h_FmPcdCcTree;
    t_Error             err = E_OK;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd,E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc,E_INVALID_STATE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcdCcTree,E_INVALID_STATE);

    err = FmPcdCcUpdateTreeOwner(h_FmPcd, p_FmPcdCcTree->treeId, TRUE);
    if(err)
        RETURN_ERROR(MINOR, err, NO_MSG);

    *p_Offset = (uint32_t)(XX_VirtToPhys(CAST_UINT64_TO_POINTER(p_FmPcdCcTree->ccTreeBaseAddr)) -
                           ((t_FmPcd *)p_FmPcd)->p_FmPcdCc->physicalMuramBase);

    return E_OK;
}

t_Error FmPcdCcUnbindTree(t_Handle h_FmPcd, t_Handle  h_FmPcdCcTree)
{
    t_FmPcdCcTree       *p_FmPcdCcTree = (t_FmPcdCcTree *)h_FmPcdCcTree;

    SANITY_CHECK_RETURN_ERROR(p_FmPcdCcTree,E_INVALID_HANDLE);

    return  FmPcdCcUpdateTreeOwner(h_FmPcd, p_FmPcdCcTree->treeId, FALSE);
}

t_Handle FM_PCD_CcBuildTree(t_Handle h_FmPcd, t_FmPcdCcTreeParams *p_PcdGroupsParam)
{
    t_FmPcd                     *p_FmPcd = (t_FmPcd *)h_FmPcd;
    t_FmPcdCc                   *p_FmPcdCc;
    t_Error                     err = E_OK;
    uint8_t                     treeId;
    int                         i = 0, j = 0, k = 0;
    t_FmPcdCcTree               *p_FmPcdCcTree;
    uint8_t                     numOfEntries;
    t_Handle                    p_CcTreeTmp;
    t_FmPcdCcGrpParams          *p_FmPcdCcGroupParams;
    t_FmPcdCcNextEngineParams   params[16];
    t_NetEnvParams              netEnvParams;
    uint8_t                     lastOne = 0;
    t_CcNodeInfo                *p_CcNodeInfo;
    t_NextEngineParamsInfo      nextEngineParamsInfo;
    uint16_t                    ccInfo[FM_PCD_MAX_NUM_OF_CC_NODES];
    t_FmPcdCcNode               *p_FmPcdCcNextNode;
    t_List                      *p_Pos;
    t_List                      ccNextDifferentNodesLst;
    uint32_t                    myInfo;
    uint16_t                    ccArray[FM_PCD_MAX_NUM_OF_CC_NODES];

    SANITY_CHECK_RETURN_VALUE(h_FmPcd,E_INVALID_HANDLE, NULL);
    SANITY_CHECK_RETURN_VALUE(p_PcdGroupsParam,E_INVALID_HANDLE, NULL);
    SANITY_CHECK_RETURN_VALUE(p_FmPcd->p_FmPcdCc,E_INVALID_STATE, NULL);

    memset(ccInfo, 0, sizeof(uint8_t) * FM_PCD_MAX_NUM_OF_CC_NODES);
    memset(ccArray, 0, sizeof(uint16_t) * FM_PCD_MAX_NUM_OF_CC_NODES);

    memset(params, 0, 16 * sizeof(t_FmPcdCcNextEngineParams));
    p_FmPcdCc = p_FmPcd->p_FmPcdCc;

    err = OccupyTreeId(p_FmPcd, &treeId);
    if(err)
    {
        REPORT_ERROR(MAJOR, err, NO_MSG);
        return NULL;
    }

    p_FmPcdCcTree = (t_FmPcdCcTree*)XX_Malloc (sizeof(t_FmPcdCcTree));
    if(!p_FmPcdCcTree)
    {
        ReleaseTree(p_FmPcdCc,treeId);
        ReleaseTreeHandler(p_FmPcdCcTree,p_FmPcdCc);
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("No memory"));
        return NULL;
    }
    memset(p_FmPcdCcTree, 0, sizeof(t_FmPcdCcTree)) ;

    INIT_LIST(&p_FmPcdCcTree->ccNextNodesLst);
    INIT_LIST(&p_FmPcdCcTree->fmPortsLst);
    INIT_LIST(&ccNextDifferentNodesLst);

    if(p_PcdGroupsParam->numOfGrps > 8)
    {
        ReleaseTree(p_FmPcdCc,treeId);
        ReleaseTreeHandler(p_FmPcdCcTree,p_FmPcdCc);
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("numOfGrps can not be greater than 8"));
        return NULL;
    }
    numOfEntries = 0;
    p_FmPcdCcTree->netEnvId = (uint8_t)(CAST_POINTER_TO_UINT32(p_PcdGroupsParam->h_NetEnv)-1);
    for(i = 0; i < p_PcdGroupsParam->numOfGrps; i++)
    {
        p_FmPcdCcGroupParams = &p_PcdGroupsParam->ccGrpParams[i];
        p_FmPcdCcTree->fmPcdGroupParam[i].baseGroupEntry = numOfEntries;
        p_FmPcdCcTree->fmPcdGroupParam[i].numOfEntriesInGroup =(uint8_t)( 0x01 << p_FmPcdCcGroupParams->numOfDistinctionUnits);
        numOfEntries += p_FmPcdCcTree->fmPcdGroupParam[i].numOfEntriesInGroup;
        if(numOfEntries > 16)
        {
            ReleaseTree(p_FmPcdCc,treeId);
            ReleaseTreeHandler(p_FmPcdCcTree,p_FmPcdCc);
            REPORT_ERROR(MAJOR, E_NO_MEMORY, ("numOfEntries can not be larger than 16"));
            return NULL;
        }
        if(lastOne)
        {
            if(p_FmPcdCcTree->fmPcdGroupParam[i].numOfEntriesInGroup > lastOne)
            {
                ReleaseTree(p_FmPcdCc,treeId);
                ReleaseTreeHandler(p_FmPcdCcTree,p_FmPcdCc);
                REPORT_ERROR(MAJOR, E_NO_MEMORY, ("numOfEntries per group in Tree has to  be from biggest to lower"));
                return NULL;
            }
        }

        lastOne = p_FmPcdCcTree->fmPcdGroupParam[i].numOfEntriesInGroup;

        netEnvParams.netEnvId = p_FmPcdCcTree->netEnvId;
        netEnvParams.numOfDistinctionUnits = p_FmPcdCcGroupParams->numOfDistinctionUnits;
        memcpy(netEnvParams.unitIds, &p_FmPcdCcGroupParams->unitIds, p_FmPcdCcGroupParams->numOfDistinctionUnits);
        err = PcdGetUnitsVector(p_FmPcd, &netEnvParams);
        if(err)
        {
            ReleaseTree(p_FmPcdCc,treeId);
            ReleaseTreeHandler(p_FmPcdCcTree,p_FmPcdCc);
            REPORT_ERROR(MAJOR, E_INVALID_STATE, NO_MSG);
            return NULL;
        }

        p_FmPcdCcTree->fmPcdGroupParam[i].totalBitsMask = netEnvParams.vector;
        for(j = 0; j < p_FmPcdCcTree->fmPcdGroupParam[i].numOfEntriesInGroup; j++)
        {

            err = ValidateNextEngineParams(h_FmPcd,&p_FmPcdCcGroupParams->p_NextEnginePerEntriesInGrp[j], &nextEngineParamsInfo);
            if(err)
            {
                ReleaseTree(p_FmPcdCc,treeId);
                ReleaseTreeHandler(p_FmPcdCcTree,p_FmPcdCc);
                REPORT_ERROR(MAJOR, err, (NO_MSG));
                return NULL;
            }
            if(nextEngineParamsInfo.fmPcdEngine == e_FM_PCD_CC)
            {
                myInfo = (uint32_t)nextEngineParamsInfo.additionalInfo | (uint32_t)k << 16;
                CreateNodeInfo(&p_FmPcdCcTree->ccNextNodesLst, myInfo);
                if(ccInfo[nextEngineParamsInfo.additionalInfo] == 0)
                    CreateNodeInfo(&ccNextDifferentNodesLst, (uint32_t)nextEngineParamsInfo.additionalInfo);

                ccInfo[nextEngineParamsInfo.additionalInfo] +=1;
            }
           memcpy(&params[k], &p_FmPcdCcGroupParams->p_NextEnginePerEntriesInGrp[j], sizeof(t_FmPcdCcNextEngineParams));
           k++;
        }
    }

    p_FmPcdCcTree->numOfGrps = p_PcdGroupsParam->numOfGrps;
    p_FmPcdCcTree->ccTreeBaseAddr =
        CAST_POINTER_TO_UINT64(FM_MURAM_AllocMem(p_FmPcdCc->h_FmMuram,
                                                 (uint32_t)( k * FM_PCD_CC_AD_ENTRY_SIZE),
                                                 FM_PCD_CC_AD_TABLE_ALIGN));

    if(!p_FmPcdCcTree->ccTreeBaseAddr)
    {
        ReleaseTree(p_FmPcdCc,treeId);
        ReleaseTreeHandler(p_FmPcdCcTree,p_FmPcdCc);
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("No memory"));
        return NULL;
    }
    WRITE_BLOCK(CAST_UINT64_TO_POINTER_TYPE(uint8_t, p_FmPcdCcTree->ccTreeBaseAddr), 0, (uint32_t)(k * FM_PCD_CC_AD_ENTRY_SIZE));

    p_CcTreeTmp  = CAST_UINT64_TO_POINTER(p_FmPcdCcTree->ccTreeBaseAddr);

    j = 0;
    for(i = 0; i < numOfEntries; i++)
    {
        NextStepAd(p_CcTreeTmp,&params[i],p_FmPcd);
        p_CcTreeTmp =   (t_Handle)(((uint32_t)p_CcTreeTmp) + FM_PCD_CC_AD_ENTRY_SIZE);
    }

    if(!LIST_IsEmpty(&ccNextDifferentNodesLst))
    {
        LIST_FOR_EACH(p_Pos, &ccNextDifferentNodesLst)
        {
            p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos);
            p_FmPcdCcNextNode = (t_FmPcdCcNode *)GetNodeHandler(p_FmPcd, (uint16_t)p_CcNodeInfo->nextCcNodeInfo);
            if(!p_FmPcdCcNextNode)
            {
                ReleaseTree(p_FmPcdCc,treeId);
                ReleaseTreeHandler(p_FmPcdCcTree,p_FmPcdCc);
                REPORT_ERROR(MAJOR, E_NO_MEMORY, ("No memory"));
                return NULL;
            }
            myInfo = (uint32_t)treeId | ((uint32_t)ccInfo[p_CcNodeInfo->nextCcNodeInfo] << 16);
            CreateNodeInfo(&p_FmPcdCcNextNode->ccTreeIdLst, myInfo);
        }
    }

    ReleaseLst(&ccNextDifferentNodesLst);
    p_FmPcdCcTree->treeId = treeId;
    SetTreeHandler(p_FmPcdCc, treeId, p_FmPcdCcTree);

    FmPcdIncNetEnvOwners(h_FmPcd, p_FmPcdCcTree->netEnvId);

    err = UpdateNodesWithTree(h_FmPcd, &p_FmPcdCcTree->ccNextNodesLst, ccArray, treeId);
    if(err)
    {
        ReleaseTree(p_FmPcdCc,treeId);
        ReleaseTreeHandler(p_FmPcdCcTree,p_FmPcdCc);
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("No memory"));
        return NULL;
    }
    return p_FmPcdCcTree;
}

t_Error FM_PCD_CcDeleteTree(t_Handle h_FmPcd, t_Handle h_CcTree)
{
    t_FmPcd                     *p_FmPcd = (t_FmPcd *)h_FmPcd;
    t_FmPcdCcTree               *p_CcTree = (t_FmPcdCcTree *)h_CcTree;
    uint32_t                    nodeIdTmp;
    t_List                      *p_Pos;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc,E_INVALID_STATE);
    SANITY_CHECK_RETURN_ERROR(p_CcTree,E_INVALID_STATE);

    FmPcdDecNetEnvOwners(h_FmPcd, p_CcTree->netEnvId);

    if(GetTreeOwners(p_FmPcd, p_CcTree->treeId))
        RETURN_ERROR(MAJOR, E_INVALID_SELECTION, ("the tree with this ID can not be removed because this tree is occupied, first - unbind this tree"));

    LIST_FOR_EACH(p_Pos, &p_CcTree->ccNextNodesLst)
    {
        nodeIdTmp = ((t_CcNodeInfo *)CC_NEXT_NODE_F_OBJECT(p_Pos))->nextCcNodeInfo;
        UpdateNodeOwner(p_FmPcd, (uint16_t)nodeIdTmp, FALSE);
    }

    ReleaseTree(p_FmPcd->p_FmPcdCc, p_CcTree->treeId);

    return E_OK;
}

t_Handle FM_PCD_CcSetNode(t_Handle h_FmPcd, t_FmPcdCcNodeParams *p_CcNodeParam)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd *) h_FmPcd;
    t_FmPcdCcNode       *p_FmPcdCcNode, *p_FmPcdCcNextNode;
    t_FmPcdCc           *p_FmPcdCc;
    t_Error             err = E_OK;
    int                 tmp, size;
    bool                glblMask = FALSE;
    t_FmPcdCcKeyParams  *p_KeyParams;
    t_Handle            p_KeysMatchTblTmp;
    t_Handle            p_AdTableTmp;
    bool                fullField = FALSE;
    uint16_t            nodeId;
    t_NextEngineParamsInfo nextEngineParamsInfo;
    uint16_t            profileInfo[FM_PCD_PLCR_NUM_ENTRIES];
    uint16_t            ccInfo[FM_PCD_MAX_NUM_OF_CC_NODES];
    uint8_t             ccDifferentInfo[FM_PCD_MAX_NUM_OF_CC_NODES];
    t_List              *p_Pos;
    t_CcNodeInfo        *p_CcNodeInfo;
    t_List              ccNextDifferentNodesLst;
    uint32_t            myInfo;

    SANITY_CHECK_RETURN_VALUE(h_FmPcd,E_INVALID_HANDLE,NULL);
    SANITY_CHECK_RETURN_VALUE(p_FmPcd->p_FmPcdCc,E_INVALID_STATE,NULL);

    if (!p_CcNodeParam->keysParams.keySize ||
        !p_CcNodeParam->keysParams.numOfKeys)
    {
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("At least one key of keySize > 0 must be defined."));
        return NULL;
    }

    memset(profileInfo, 0x00, FM_PCD_PLCR_NUM_ENTRIES*sizeof(uint16_t));
    memset(ccInfo, 0x00, FM_PCD_MAX_NUM_OF_CC_NODES*sizeof(uint16_t));
    memset(ccDifferentInfo, 0x00, FM_PCD_MAX_NUM_OF_CC_NODES*sizeof(uint8_t));

    p_FmPcdCc = p_FmPcd->p_FmPcdCc;

    if((p_CcNodeParam->keysParams.keySize > 4 )&& (p_CcNodeParam->keysParams.p_GlblMask))
    {
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("Global Mask is relevant only for keySize less or equal than 4"));
        return NULL;
    }

    err = ValidateNextEngineParams(h_FmPcd, &p_CcNodeParam->keysParams.ccNextEngineParamsForMiss, &nextEngineParamsInfo);
    if(err)
    {
        REPORT_ERROR(MAJOR, err, NO_MSG);
        return NULL;
    }

    err = OccupyNodeId(p_FmPcd, &nodeId);
    if(err)
    {
        REPORT_ERROR(MAJOR, err, NO_MSG);
        return NULL;
    }

    p_FmPcdCcNode = (t_FmPcdCcNode*)XX_Malloc(sizeof(t_FmPcdCcNode));
    if(!p_FmPcdCcNode)
    {
        ReleaseNode(p_FmPcdCc,nodeId);
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("No memory"));
        return NULL;
    }
    memset(p_FmPcdCcNode, 0, sizeof(t_FmPcdCcNode));

    INIT_LIST(&p_FmPcdCcNode->ccNextNodesLst);
    INIT_LIST(&p_FmPcdCcNode->ccPrevNodesLst);
    INIT_LIST(&p_FmPcdCcNode->ccTreeIdLst);
    INIT_LIST(&p_FmPcdCcNode->ccTreesLst);

    INIT_LIST(&ccNextDifferentNodesLst);

    if(nextEngineParamsInfo.fmPcdEngine == e_FM_PCD_CC)
    {
        myInfo = (uint32_t)nextEngineParamsInfo.additionalInfo | (uint32_t)(p_CcNodeParam->keysParams.numOfKeys << 16);
        CreateNodeInfo(&p_FmPcdCcNode->ccNextNodesLst, myInfo);
        if(ccInfo[nextEngineParamsInfo.additionalInfo] == 0)
            CreateNodeInfo(&ccNextDifferentNodesLst, (uint32_t)nextEngineParamsInfo.additionalInfo);
        ccInfo[nextEngineParamsInfo.additionalInfo] +=1;
    }

    p_FmPcdCcNode->numOfKeys = p_CcNodeParam->keysParams.numOfKeys;

    p_FmPcdCcNode->p_GlblMask = (t_Handle)XX_Malloc(p_CcNodeParam->keysParams.keySize * sizeof(uint8_t));
    memset(p_FmPcdCcNode->p_GlblMask, 0, p_CcNodeParam->keysParams.keySize * sizeof(uint8_t));

    if(p_CcNodeParam->keysParams.p_GlblMask)
    {
        memcpy((void*)p_FmPcdCcNode->p_GlblMask,p_CcNodeParam->keysParams.p_GlblMask,p_CcNodeParam->keysParams.keySize);
        glblMask = TRUE;
        p_FmPcdCcNode->glblMaskSize = (uint8_t)p_CcNodeParam->keysParams.keySize;
    }
    else
     {   memset(p_FmPcdCcNode->p_GlblMask, 0xff, CC_GLBL_MASK_SIZE);
         p_FmPcdCcNode->glblMaskSize = CC_GLBL_MASK_SIZE;
     }
    switch(p_CcNodeParam->extractCcParams.type)
    {
        case(e_FM_PCD_EXTRACT_BY_HDR):
            switch(p_CcNodeParam->extractCcParams.extractParams.extractByHdr.type)
            {
                case(e_FM_PCD_EXTRACT_FULL_FIELD):
                    p_FmPcdCcNode->parseCode = GetFullFieldParseCode(p_CcNodeParam->extractCcParams.extractParams.extractByHdr.hdr, p_CcNodeParam->extractCcParams.extractParams.extractByHdr.hdrIndex,
                                                                    p_CcNodeParam->extractCcParams.extractParams.extractByHdr.extractByHdrType.fullField);
                    GetSizeHeaderField(p_CcNodeParam->extractCcParams.extractParams.extractByHdr.hdr, p_CcNodeParam->extractCcParams.extractParams.extractByHdr.extractByHdrType.fullField, &p_FmPcdCcNode->sizeOfExtraction);
                    fullField = TRUE;
                    break;
                case(e_FM_PCD_EXTRACT_FROM_HDR):
                        p_FmPcdCcNode->sizeOfExtraction = p_CcNodeParam->extractCcParams.extractParams.extractByHdr.extractByHdrType.fromHdr.size;
                        p_FmPcdCcNode->offset =  p_CcNodeParam->extractCcParams.extractParams.extractByHdr.extractByHdrType.fromHdr.offset;
                        p_FmPcdCcNode->parseCode = GetPrParseCode(p_CcNodeParam->extractCcParams.extractParams.extractByHdr.hdr, p_CcNodeParam->extractCcParams.extractParams.extractByHdr.hdrIndex,
                                                                p_FmPcdCcNode->offset,glblMask, &p_FmPcdCcNode->prsArrayOffset);
                        break;
                case(e_FM_PCD_EXTRACT_FROM_FIELD):
                        p_FmPcdCcNode->offset = p_CcNodeParam->extractCcParams.extractParams.extractByHdr.extractByHdrType.fromField.offset;
                        p_FmPcdCcNode->sizeOfExtraction = p_CcNodeParam->extractCcParams.extractParams.extractByHdr.extractByHdrType.fromField.size;
                        p_FmPcdCcNode->parseCode = GetFieldParseCode(p_CcNodeParam->extractCcParams.extractParams.extractByHdr.hdr, p_CcNodeParam->extractCcParams.extractParams.extractByHdr.extractByHdrType.fromField.field,
                                                    p_FmPcdCcNode->offset,&p_FmPcdCcNode->prsArrayOffset,
                                                    p_CcNodeParam->extractCcParams.extractParams.extractByHdr.hdrIndex);
                        break;
                default:
                    ReleaseNode(p_FmPcdCc,nodeId);
                    ReleaseNodeHandler(p_FmPcdCcNode,p_FmPcdCc);
                    REPORT_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
                    return NULL;
            }
            break;
        case(e_FM_PCD_EXTRACT_NON_HDR):
            /* get the field code for the generic extract */
            p_FmPcdCcNode->sizeOfExtraction = p_CcNodeParam->extractCcParams.extractParams.extractNonHdr.size;
            p_FmPcdCcNode->offset =  p_CcNodeParam->extractCcParams.extractParams.extractNonHdr.offset;
            p_FmPcdCcNode->parseCode = GetGenParseCode(p_CcNodeParam->extractCcParams.extractParams.extractNonHdr.src, p_FmPcdCcNode->offset, glblMask, &p_FmPcdCcNode->prsArrayOffset);
            if(p_FmPcdCcNode->parseCode == CC_PC_GENERIC_IC_GMASK)
            {
                p_FmPcdCcNode->offset +=  p_FmPcdCcNode->prsArrayOffset;
                p_FmPcdCcNode->prsArrayOffset = 0;
            }
            if(p_FmPcdCcNode->parseCode == CC_PC_GENERIC_IC_HASH_INDEXED)
            {
                 ReleaseNode(p_FmPcdCc,nodeId);
                 REPORT_ERROR(MAJOR, E_INVALID_SELECTION, ("Not implemented yet"));
                 return NULL;
                if(!glblMask)
               {
                   REPORT_ERROR(MAJOR, E_INVALID_SELECTION, ("in the type e_FM_PCD_EXTRACT_FROM_IC_HASH_INDEXED_MATCH glblMask has to be defined"));
                   return NULL;
               }
                if(p_FmPcdCcNode->sizeOfExtraction != 2)
                {
                    REPORT_ERROR(MAJOR, E_INVALID_SELECTION, ("in the type e_FM_PCD_EXTRACT_FROM_IC_HASH_INDEXED_MATCH sizeOfExtraction has to be 2 bytes"));
                    return NULL;
                }
              }
                break;

       default:
            ReleaseNode(p_FmPcdCc,nodeId);
            ReleaseNodeHandler(p_FmPcdCcNode,p_FmPcdCc);
            REPORT_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
            return NULL;
    }

    if(p_FmPcdCcNode->parseCode == CC_PC_ILLEGAL)
    {
        ReleaseNode(p_FmPcdCc,nodeId);
        ReleaseNodeHandler(p_FmPcdCcNode,p_FmPcdCc);
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("illeagl extraction type"));
        return NULL;
    }

    if(((p_FmPcdCcNode->parseCode == CC_PC_FF_IPV4TTL) || (p_FmPcdCcNode->parseCode == CC_PC_FF_IPV6HOP_LIMIT)) && (p_FmPcdCcNode->numOfKeys != 1 ))
    {
        ReleaseNode(p_FmPcdCc,nodeId);
        ReleaseNodeHandler(p_FmPcdCcNode,p_FmPcdCc);
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("for IPV4TTL and IPV6_HOP_LIMIT has to be only 1 key - TTL = 1, otherwise it's Miss"));
        return NULL;
    }

    if((p_FmPcdCcNode->sizeOfExtraction > FM_PCD_MAX_SIZE_OF_KEY) || !p_FmPcdCcNode->sizeOfExtraction)
    {
        ReleaseNode(p_FmPcdCc,nodeId);
        ReleaseNodeHandler(p_FmPcdCcNode,p_FmPcdCc);
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("sizeOfExatrction can not be greater than 56 and not 0"));
        return NULL;
    }

    if(p_CcNodeParam->keysParams.keySize !=p_FmPcdCcNode->sizeOfExtraction)
    {
        ReleaseNode(p_FmPcdCc,nodeId);
        ReleaseNodeHandler(p_FmPcdCcNode,p_FmPcdCc);
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("keySize has to be equal to sizeOfExtraction"));
        return NULL;
    }

    GetCcExtractKeySize(p_FmPcdCcNode->sizeOfExtraction, &p_FmPcdCcNode->ccKeySizeAccExtraction);

    for(tmp = 0 ; tmp < p_FmPcdCcNode->numOfKeys; tmp++)
    {
        p_KeyParams = &p_CcNodeParam->keysParams.keyParams[tmp];
        if((p_FmPcdCcNode->parseCode != CC_PC_FF_IPV4TTL) && (p_FmPcdCcNode->parseCode != CC_PC_FF_IPV6HOP_LIMIT))
        {

                if(!p_KeyParams->p_Key)
                {
                    ReleaseNode(p_FmPcdCc,nodeId);
                    ReleaseNodeHandler(p_FmPcdCcNode,p_FmPcdCc);
                    REPORT_ERROR(MAJOR, E_NO_MEMORY, ("p_Key is not initialized"));
                    return NULL;
                }

                if(p_KeyParams->p_Mask && glblMask)
                {
                    ReleaseNode(p_FmPcdCc,nodeId);
                    ReleaseNodeHandler(p_FmPcdCcNode,p_FmPcdCc);
                    REPORT_ERROR(MAJOR, E_NO_MEMORY, ("Can not be used globalMask and localMask"));
                    return NULL;
                }

        }
        err = ValidateNextEngineParams(h_FmPcd, &p_KeyParams->ccNextEngineParams, &nextEngineParamsInfo);
        if(err)
        {
            ReleaseNode(p_FmPcdCc,nodeId);
            ReleaseNodeHandler(p_FmPcdCcNode,p_FmPcdCc);
            REPORT_ERROR(MAJOR, err, (NO_MSG));
            return NULL;
        }
        if(nextEngineParamsInfo.fmPcdEngine == e_FM_PCD_CC)
        {
            myInfo = (uint32_t)nextEngineParamsInfo.additionalInfo | (uint32_t)tmp << 16;
            CreateNodeInfo(&p_FmPcdCcNode->ccNextNodesLst,myInfo);
            if(ccInfo[nextEngineParamsInfo.additionalInfo] == 0)
                CreateNodeInfo(&ccNextDifferentNodesLst, (uint32_t)nextEngineParamsInfo.additionalInfo);
            ccInfo[nextEngineParamsInfo.additionalInfo] +=1;
        }
        if(p_KeyParams->p_Mask)
            p_FmPcdCcNode->lclMask = TRUE;
    }

    if(p_FmPcdCcNode->lclMask)
        size = 2 * p_FmPcdCcNode->ccKeySizeAccExtraction;
    else
        size = p_FmPcdCcNode->ccKeySizeAccExtraction;

    if((p_FmPcdCcNode->parseCode != CC_PC_FF_IPV4TTL) && (p_FmPcdCcNode->parseCode != CC_PC_FF_IPV6HOP_LIMIT))
    {
        p_FmPcdCcNode->h_KeysMatchTable =(t_Handle)FM_MURAM_AllocMem(p_FmPcdCc->h_FmMuram,
                                         (uint32_t)(size * sizeof(uint8_t) * (p_FmPcdCcNode->numOfKeys + 1)),
                                         FM_PCD_CC_KEYS_MATCH_TABLE_ALIGN);
        if(!p_FmPcdCcNode->h_KeysMatchTable)
        {
            ReleaseNode(p_FmPcdCc,nodeId);
            ReleaseNodeHandler(p_FmPcdCcNode,p_FmPcdCc);
            REPORT_ERROR(MAJOR, E_NO_MEMORY, ("No memory in MURAM for KEY MATCH table"));
            return NULL;
        }
        WRITE_BLOCK((uint8_t *)p_FmPcdCcNode->h_KeysMatchTable, 0, size * sizeof(uint8_t) * (p_FmPcdCcNode->numOfKeys + 1));
    }

    p_FmPcdCcNode->h_AdTable = (t_Handle)FM_MURAM_AllocMem(p_FmPcdCc->h_FmMuram,
                                     (uint32_t)( (p_FmPcdCcNode->numOfKeys+1) * FM_PCD_CC_AD_ENTRY_SIZE),
                                     FM_PCD_CC_AD_TABLE_ALIGN);
    if(!p_FmPcdCcNode->h_AdTable)
    {
        ReleaseNode(p_FmPcdCc,nodeId);
        ReleaseNodeHandler(p_FmPcdCcNode,p_FmPcdCc);
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("No memory in MURAM ffor AD table "));
        return NULL;
    }
    WRITE_BLOCK((uint8_t *)p_FmPcdCcNode->h_AdTable, 0, (uint32_t)((p_FmPcdCcNode->numOfKeys+1) * FM_PCD_CC_AD_ENTRY_SIZE));

    p_KeysMatchTblTmp    = p_FmPcdCcNode->h_KeysMatchTable;
    p_AdTableTmp         = p_FmPcdCcNode->h_AdTable;
    for(tmp = 0 ; tmp < p_FmPcdCcNode->numOfKeys; tmp++)
    {
        p_KeyParams = &p_CcNodeParam->keysParams.keyParams[tmp];

        if(p_KeysMatchTblTmp)
        {

            COPY_BLOCK((void*)p_KeysMatchTblTmp, p_KeyParams->p_Key, p_FmPcdCcNode->sizeOfExtraction);

            if(p_FmPcdCcNode->lclMask && p_KeyParams->p_Mask)
                COPY_BLOCK((void*)(((uint32_t)p_KeysMatchTblTmp) + p_FmPcdCcNode->ccKeySizeAccExtraction), p_KeyParams->p_Mask, p_FmPcdCcNode->sizeOfExtraction);
            else if(p_FmPcdCcNode->lclMask)
                WRITE_BLOCK((void*)(((uint32_t)p_KeysMatchTblTmp) + p_FmPcdCcNode->ccKeySizeAccExtraction),0xff, p_FmPcdCcNode->sizeOfExtraction);
            p_KeysMatchTblTmp = (t_Handle)(((uint32_t)p_KeysMatchTblTmp) + size * sizeof(uint8_t));
        }
        NextStepAd(p_AdTableTmp,&p_KeyParams->ccNextEngineParams, p_FmPcd);

        p_AdTableTmp =   (t_Handle)(((uint32_t)p_AdTableTmp) + FM_PCD_CC_AD_ENTRY_SIZE);

    }
    NextStepAd(p_AdTableTmp,&p_CcNodeParam->keysParams.ccNextEngineParamsForMiss, p_FmPcd);

    if(fullField == TRUE)
        p_FmPcdCcNode->sizeOfExtraction = 0;


    if(!LIST_IsEmpty(&ccNextDifferentNodesLst))
    {
        LIST_FOR_EACH(p_Pos, &ccNextDifferentNodesLst)
        {
            p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos);
            p_FmPcdCcNextNode = (t_FmPcdCcNode *)GetNodeHandler(p_FmPcd, (uint16_t)p_CcNodeInfo->nextCcNodeInfo);
            if(!p_FmPcdCcNextNode)
            {
                ReleaseNode(p_FmPcdCc,nodeId);
                REPORT_ERROR(MAJOR, E_NO_MEMORY, ("No memory"));
                return NULL;
            }
            myInfo = (uint32_t)nodeId | ((uint32_t)ccInfo[(uint16_t)p_CcNodeInfo->nextCcNodeInfo] << 16);
            CreateNodeInfo(&p_FmPcdCcNextNode->ccPrevNodesLst, myInfo);
        }
    }
    ReleaseLst(&ccNextDifferentNodesLst);
    p_FmPcdCcNode->nodeId = nodeId;
    SetNodeHandler(p_FmPcdCc, nodeId, p_FmPcdCcNode);
    return p_FmPcdCcNode;
}

t_Error FM_PCD_CcDeleteNode(t_Handle h_FmPcd, t_Handle h_CcNode)
{
    t_FmPcd                     *p_FmPcd = (t_FmPcd *)h_FmPcd;
    t_FmPcdCcNode               *p_CcNode = (t_FmPcdCcNode *)h_CcNode;
    uint32_t                    nodeIdTmp;
    t_List                      *p_Pos;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc,E_INVALID_STATE);

    if(!p_CcNode)
        RETURN_ERROR(MAJOR, E_INVALID_SELECTION, ("the node with this ID is not initialized"));

    if(GetNodeOwners(p_FmPcd, p_CcNode->nodeId))
        RETURN_ERROR(MAJOR, E_INVALID_SELECTION, ("the node with this ID can not be removed because this node is occupied, first - unbind this node"));

    LIST_FOR_EACH(p_Pos, &p_CcNode->ccNextNodesLst)
    {
        nodeIdTmp = ((t_CcNodeInfo *)CC_NEXT_NODE_F_OBJECT(p_Pos))->nextCcNodeInfo;
        UpdateNodeOwner(p_FmPcd, (uint16_t)nodeIdTmp, FALSE);

    }

    ReleaseNode(p_FmPcd->p_FmPcdCc, p_CcNode->nodeId);
    return E_OK;
}

t_Error FM_PCD_CcTreeModifyNextEngine(t_Handle h_FmPcd, t_Handle h_CcTree, uint8_t grpId, uint8_t index, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams)
{
    t_FmPcd                         *p_FmPcd = (t_FmPcd *)h_FmPcd;

    SANITY_CHECK_RETURN_ERROR(h_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->h_Hc, E_INVALID_HANDLE);

    return FmHcPcdCcModifyTreeNextEngine(p_FmPcd->h_Hc, h_CcTree, grpId, index, p_FmPcdCcNextEngineParams);
}

t_Error FM_PCD_CcNodeModifyNextEngine(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams)
{
    t_FmPcd                         *p_FmPcd = (t_FmPcd *)h_FmPcd;

    SANITY_CHECK_RETURN_ERROR(h_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->h_Hc, E_INVALID_HANDLE);

    return FmHcPcdCcModifyNodeNextEngine(p_FmPcd->h_Hc, h_CcNode, keyIndex, p_FmPcdCcNextEngineParams);
}

t_Error FM_PCD_CcNodeModifyMissNextEngine(t_Handle h_FmPcd, t_Handle h_CcNode, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams)
{
    t_FmPcd                         *p_FmPcd = (t_FmPcd *)h_FmPcd;

    SANITY_CHECK_RETURN_ERROR(h_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->h_Hc, E_INVALID_HANDLE);

    return FmHcPcdCcModifyNodeMissNextEngine(p_FmPcd->h_Hc, h_CcNode, p_FmPcdCcNextEngineParams);
}

t_Error FM_PCD_CcNodeRemoveKey(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex)
{
    t_FmPcd                         *p_FmPcd = (t_FmPcd *)h_FmPcd;

    SANITY_CHECK_RETURN_ERROR(h_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->h_Hc, E_INVALID_HANDLE);

    return FmHcPcdCcRemoveKey(p_FmPcd->h_Hc, h_CcNode, keyIndex);
}

t_Error FM_PCD_CcNodeAddKey(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex, uint8_t keySize, t_FmPcdCcKeyParams  *p_KeyParams)
{
    t_FmPcd                         *p_FmPcd = (t_FmPcd *)h_FmPcd;

    SANITY_CHECK_RETURN_ERROR(h_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->h_Hc, E_INVALID_HANDLE);

    return FmHcPcdCcAddKey(p_FmPcd->h_Hc, h_CcNode, keyIndex, keySize, p_KeyParams);
}

t_Error FM_PCD_CcNodeModifyKeyAndNextEngine(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex, uint8_t keySize, t_FmPcdCcKeyParams  *p_KeyParams)
{
    t_FmPcd                         *p_FmPcd = (t_FmPcd *)h_FmPcd;

    SANITY_CHECK_RETURN_ERROR(h_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->h_Hc, E_INVALID_HANDLE);

    return FmHcPcdCcModifyKeyAndNextEngine(p_FmPcd->h_Hc, h_CcNode, keyIndex, keySize, p_KeyParams);
}

t_Error FM_PCD_CcNodeModifyKey(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex, uint8_t keySize, uint8_t  *p_Key, uint8_t *p_Mask)
{
    t_FmPcd                         *p_FmPcd = (t_FmPcd *)h_FmPcd;

    SANITY_CHECK_RETURN_ERROR(h_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdCc, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->h_Hc, E_INVALID_HANDLE);

    return FmHcPcdCcModifyKey(p_FmPcd->h_Hc, h_CcNode, keyIndex, keySize, p_Key, p_Mask);
}

void FmPcdCcNodeTreeReleaseLock(t_Handle h_FmPcd, t_List *p_List)
{
    t_List          *p_Pos;
    t_CcNodeInfo    *p_CcNodeInfo;
    t_Handle        h_FmPcdCcTree;

    LIST_FOR_EACH(p_Pos, p_List)
    {
        p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos);
        h_FmPcdCcTree = FmPcdCcGetTreeHandler(h_FmPcd, (uint8_t)p_CcNodeInfo->nextCcNodeInfo);
        FmPcdCcTreeReleaseLock(h_FmPcdCcTree);
    }
    ReleaseLst(p_List);
}

t_Error FmPcdCcNodeTreeTryLock(t_Handle h_FmPcd,t_Handle h_FmPcdCcNode, t_List *p_List)
{
    t_FmPcdCcNode   *p_FmPcdCcNode = (t_FmPcdCcNode *)h_FmPcdCcNode;
    t_List          *p_Pos;
    t_CcNodeInfo    *p_CcNodeInfo;
    t_Handle        h_FmPcdCcTree;
    t_Error         err = E_OK;

    if(LIST_IsEmpty(&p_FmPcdCcNode->ccTreesLst))
        RETURN_ERROR(MAJOR, E_NOT_AVAILABLE, ("asked for more nodes in CC than MAX"))  ;
    LIST_FOR_EACH(p_Pos, &p_FmPcdCcNode->ccTreesLst)
    {
        p_CcNodeInfo = CC_NEXT_NODE_F_OBJECT(p_Pos);
        h_FmPcdCcTree = FmPcdCcGetTreeHandler(h_FmPcd, (uint8_t)p_CcNodeInfo->nextCcNodeInfo);
        err = FmPcdCcTreeTryLock(h_FmPcdCcTree);
        if(err == E_OK)
            CreateNodeInfo(p_List, (uint8_t)p_CcNodeInfo->nextCcNodeInfo);
        else
            FmPcdCcNodeTreeReleaseLock(h_FmPcd, p_List);
    }

    return err;
}
