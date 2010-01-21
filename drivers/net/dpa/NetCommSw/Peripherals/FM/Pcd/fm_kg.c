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
 @File          fm_kg.c

 @Description   FM PCD ...
*//***************************************************************************/
#include "std_ext.h"
#include "error_ext.h"
#include "string_ext.h"
#include "debug_ext.h"
#include "net_ext.h"
#include "fm_pcd.h"
#include "fm_port_ext.h"
#include "fm_hc.h"


static e_FmPcdKgExtractDfltSelect GetGenericSwDefault(t_FmPcdKgExtractDflt swDefaults[], uint8_t numOfSwDefaults, uint8_t code)
{
    int i;

    switch(code)
    {
        case( KG_SCH_GEN_PARSE_RESULT):
        case( KG_SCH_GEN_DEFAULT):
        case( KG_SCH_GEN_NEXTHDR):
            for(i=0 ; i<numOfSwDefaults ; i++)
                if(swDefaults[i].type == e_FM_PCD_KG_GENERIC_NOT_FROM_DATA)
                    return swDefaults[i].dfltSelect;
            ASSERT_COND(FALSE);
        case( KG_SCH_GEN_SHIM1):
        case( KG_SCH_GEN_SHIM2):
        case( KG_SCH_GEN_SHIM3):
        case( KG_SCH_GEN_ETH_NO_V):
        case( KG_SCH_GEN_SNAP_NO_V):
        case( KG_SCH_GEN_VLAN1_NO_V):
        case( KG_SCH_GEN_VLAN2_NO_V):
        case( KG_SCH_GEN_ETH_TYPE_NO_V):
        case( KG_SCH_GEN_PPP_NO_V):
        case( KG_SCH_GEN_MPLS1_NO_V):
        case( KG_SCH_GEN_MPLS_LAST_NO_V):
        case( KG_SCH_GEN_L3_NO_V):
        case( KG_SCH_GEN_IP2_NO_V):
        case( KG_SCH_GEN_GRE_NO_V):
        case( KG_SCH_GEN_L4_NO_V):
            for(i=0 ; i<numOfSwDefaults ; i++)
                if(swDefaults[i].type == e_FM_PCD_KG_GENERIC_FROM_DATA_NO_V)
                    return swDefaults[i].dfltSelect;

        case( KG_SCH_GEN_START_OF_FRM):
        case( KG_SCH_GEN_ETH):
        case( KG_SCH_GEN_SNAP):
        case( KG_SCH_GEN_VLAN1):
        case( KG_SCH_GEN_VLAN2):
        case( KG_SCH_GEN_ETH_TYPE):
        case( KG_SCH_GEN_PPP):
        case( KG_SCH_GEN_MPLS1):
        case( KG_SCH_GEN_MPLS2):
        case( KG_SCH_GEN_MPLS3):
        case( KG_SCH_GEN_MPLS_LAST):
        case( KG_SCH_GEN_IPV4):
        case( KG_SCH_GEN_IPV6):
        case( KG_SCH_GEN_IPV4_TUNNELED):
        case( KG_SCH_GEN_IPV6_TUNNELED):
        case( KG_SCH_GEN_MIN_ENCAP):
        case( KG_SCH_GEN_GRE):
        case( KG_SCH_GEN_TCP):
        case( KG_SCH_GEN_UDP):
        case( KG_SCH_GEN_IPSEC_AH):
        case( KG_SCH_GEN_SCTP):
        case( KG_SCH_GEN_DCCP):
        case( KG_SCH_GEN_IPSEC_ESP):
            for(i=0 ; i<numOfSwDefaults ; i++)
                if(swDefaults[i].type == e_FM_PCD_KG_GENERIC_FROM_DATA)
                    return swDefaults[i].dfltSelect;
        default:
            return e_FM_PCD_KG_DFLT_ILLEGAL;
    }
}

static uint8_t GetGenCode(e_FmPcdExtractFrom src)
{
    switch(src)
    {
        case(e_FM_PCD_EXTRACT_FROM_FRAME_START):
            return KG_SCH_GEN_START_OF_FRM;
        case(e_FM_PCD_KG_EXTRACT_FROM_DFLT_VALUE):
            return KG_SCH_GEN_DEFAULT;
        case(e_FM_PCD_KG_EXTRACT_FROM_PARSE_RESULT):
            return KG_SCH_GEN_PARSE_RESULT;
        case(e_FM_PCD_KG_EXTRACT_FROM_CURR_END_OF_PARSE):
            return KG_SCH_GEN_NEXTHDR;
        default:
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("Illegal 'extract from' src"));
            return 0;
    }
}

static uint8_t GetGenHdrCode(e_NetHeaderType hdr, e_FmPcdHdrIndex hdrIndex, bool ignoreProtocolValidation)
{
    if(!ignoreProtocolValidation)
        switch(hdr)
        {
            case(HEADER_TYPE_NONE):
                ASSERT_COND(FALSE);
            case(HEADER_TYPE_ETH):
                return KG_SCH_GEN_ETH;
            case(HEADER_TYPE_LLC_SNAP):
                return KG_SCH_GEN_SNAP;
            case(HEADER_TYPE_PPPoE):
                return KG_SCH_GEN_PPP;
            case(HEADER_TYPE_MPLS):
                if((hdrIndex == e_FM_PCD_HDR_INDEX_NONE) || (hdrIndex == e_FM_PCD_HDR_INDEX_1))
                    return KG_SCH_GEN_MPLS1;
                if(hdrIndex == e_FM_PCD_HDR_INDEX_2)
                    return KG_SCH_GEN_MPLS2;
                if(hdrIndex == e_FM_PCD_HDR_INDEX_3)
                    return KG_SCH_GEN_MPLS3;
                if(hdrIndex == e_FM_PCD_HDR_INDEX_LAST)
                    return KG_SCH_GEN_MPLS_LAST;
                REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal MPLS header index"));
                return 0;
            case(HEADER_TYPE_IPv4):
                if((hdrIndex == e_FM_PCD_HDR_INDEX_NONE) || (hdrIndex == e_FM_PCD_HDR_INDEX_1))
                    return KG_SCH_GEN_IPV4;
                if(hdrIndex == e_FM_PCD_HDR_INDEX_2)
                    return KG_SCH_GEN_IPV4_TUNNELED;
                REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv4 header index"));
                return 0;
            case(HEADER_TYPE_IPv6):
                if((hdrIndex == e_FM_PCD_HDR_INDEX_NONE) || (hdrIndex == e_FM_PCD_HDR_INDEX_1))
                    return KG_SCH_GEN_IPV6;
                if(hdrIndex == e_FM_PCD_HDR_INDEX_2)
                    return KG_SCH_GEN_IPV6_TUNNELED;
                REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv6 header index"));
                return 0;
            case(HEADER_TYPE_GRE):
                return KG_SCH_GEN_GRE;
            case(HEADER_TYPE_TCP):
                return KG_SCH_GEN_TCP;
            case(HEADER_TYPE_UDP):
                return KG_SCH_GEN_UDP;
            case(HEADER_TYPE_IPSEC_AH):
                return KG_SCH_GEN_IPSEC_AH;
            case(HEADER_TYPE_IPSEC_ESP):
                return KG_SCH_GEN_IPSEC_ESP;
            case(HEADER_TYPE_SCTP):
                return KG_SCH_GEN_SCTP;
            case(HEADER_TYPE_DCCP):
                return KG_SCH_GEN_DCCP;
            default:
                REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                return 0;
        }
    else
        switch(hdr)
        {
            case(HEADER_TYPE_NONE):
                ASSERT_COND(FALSE);
            case(HEADER_TYPE_ETH):
                return KG_SCH_GEN_ETH_NO_V;
            case(HEADER_TYPE_LLC_SNAP):
                return KG_SCH_GEN_SNAP_NO_V;
            case(HEADER_TYPE_PPPoE):
                return KG_SCH_GEN_PPP_NO_V;
            case(HEADER_TYPE_MPLS):
                 if((hdrIndex == e_FM_PCD_HDR_INDEX_NONE) || (hdrIndex == e_FM_PCD_HDR_INDEX_1))
                    return KG_SCH_GEN_MPLS1_NO_V;
                if(hdrIndex == e_FM_PCD_HDR_INDEX_LAST)
                    return KG_SCH_GEN_MPLS_LAST_NO_V;
                if((hdrIndex == e_FM_PCD_HDR_INDEX_2) || (hdrIndex == e_FM_PCD_HDR_INDEX_3) )
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Indexed MPLS Extraction not supported"));
                else
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal MPLS header index"));
                return 0;
            case(HEADER_TYPE_IPv4):
            case(HEADER_TYPE_IPv6):
              if((hdrIndex == e_FM_PCD_HDR_INDEX_NONE) || (hdrIndex == e_FM_PCD_HDR_INDEX_1))
                    return KG_SCH_GEN_L3_NO_V;
                if(hdrIndex == e_FM_PCD_HDR_INDEX_2)
                    return KG_SCH_GEN_IP2_NO_V;
                REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IP header index"));
            case(HEADER_TYPE_MINENCAP):
                return KG_SCH_GEN_IP2_NO_V;
            case(HEADER_TYPE_USER_DEFINED_L3):
                return KG_SCH_GEN_L3_NO_V;
            case(HEADER_TYPE_GRE):
                return KG_SCH_GEN_GRE_NO_V;
            case(HEADER_TYPE_TCP):
            case(HEADER_TYPE_UDP):
            case(HEADER_TYPE_IPSEC_AH):
            case(HEADER_TYPE_IPSEC_ESP):
            case(HEADER_TYPE_SCTP):
            case(HEADER_TYPE_DCCP):
                return KG_SCH_GEN_L4_NO_V;
            case(HEADER_TYPE_USER_DEFINED_SHIM1):
                return KG_SCH_GEN_SHIM1;
            case(HEADER_TYPE_USER_DEFINED_SHIM2):
                return KG_SCH_GEN_SHIM2;
            case(HEADER_TYPE_USER_DEFINED_SHIM3):
                return KG_SCH_GEN_SHIM3;
            default:
                REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                return 0;
        }
}

static t_GenericCodes GetGenFieldCode(e_NetHeaderType hdr, t_FmPcdFields field, bool ignoreProtocolValidation, e_FmPcdHdrIndex hdrIndex)
{
    if(!ignoreProtocolValidation)
        switch(hdr)
        {
            case(HEADER_TYPE_NONE):
                ASSERT_COND(FALSE);
            case(HEADER_TYPE_ETH):
                switch(field.eth)
                {
                    case(NET_HEADER_FIELD_ETH_TYPE):
                        return KG_SCH_GEN_ETH_TYPE;
                    default:
                        REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                        return 0;
                }
                break;
            case(HEADER_TYPE_VLAN):
                switch(field.vlan)
                {
                case(NET_HEADER_FIELD_VLAN_TCI) :
                    if((hdrIndex == e_FM_PCD_HDR_INDEX_NONE) || (hdrIndex == e_FM_PCD_HDR_INDEX_1))
                        return KG_SCH_GEN_VLAN1;
                    if(hdrIndex == e_FM_PCD_HDR_INDEX_LAST)
                        return KG_SCH_GEN_VLAN2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal VLAN header index"));
                        return 0;
                }
                break;
            case(HEADER_TYPE_MPLS):
            case(HEADER_TYPE_IPSEC_AH):
            case(HEADER_TYPE_IPSEC_ESP):
            case(HEADER_TYPE_LLC_SNAP):
            case(HEADER_TYPE_PPPoE):
            case(HEADER_TYPE_IPv4):
            case(HEADER_TYPE_IPv6):
            case(HEADER_TYPE_GRE):
            case(HEADER_TYPE_MINENCAP):
            case(HEADER_TYPE_USER_DEFINED_L3):
            case(HEADER_TYPE_TCP):
            case(HEADER_TYPE_UDP):
            case(HEADER_TYPE_SCTP):
            case(HEADER_TYPE_DCCP):
            case(HEADER_TYPE_USER_DEFINED_L4):
                REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
            default:
                REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Header not supported"));
                return 0;
        }
        else
            switch(hdr)
            {
                case(HEADER_TYPE_NONE):
                    ASSERT_COND(FALSE);
                case(HEADER_TYPE_ETH):
                switch(field.eth)
                {
                    case(NET_HEADER_FIELD_ETH_TYPE):
                        return KG_SCH_GEN_ETH_TYPE_NO_V;
                    default:
                        REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                        return 0;
                }
                break;
                case(HEADER_TYPE_VLAN):
                    switch(field.vlan)
                    {
                    case(NET_HEADER_FIELD_VLAN_TCI) :
                        if((hdrIndex == e_FM_PCD_HDR_INDEX_NONE) || (hdrIndex == e_FM_PCD_HDR_INDEX_1))
                            return KG_SCH_GEN_VLAN1_NO_V;
                        if(hdrIndex == e_FM_PCD_HDR_INDEX_LAST)
                            return KG_SCH_GEN_VLAN2_NO_V;
                        REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal VLAN header index"));
                        return 0;
                    }
                    break;
                case(HEADER_TYPE_MPLS):
                case(HEADER_TYPE_LLC_SNAP):
                case(HEADER_TYPE_PPPoE):
                case(HEADER_TYPE_IPv4):
                case(HEADER_TYPE_IPv6):
                case(HEADER_TYPE_GRE):
                case(HEADER_TYPE_MINENCAP):
                case(HEADER_TYPE_USER_DEFINED_L3):
                case(HEADER_TYPE_TCP):
                case(HEADER_TYPE_UDP):
                case(HEADER_TYPE_IPSEC_AH):
                case(HEADER_TYPE_IPSEC_ESP):
                case(HEADER_TYPE_SCTP):
                case(HEADER_TYPE_DCCP):
                case(HEADER_TYPE_USER_DEFINED_L4):
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Header not supported"));
                    return 0;
            }
    return 0;
}

static t_KnownFieldsMasks GetKnownProtMask(e_NetHeaderType hdr, e_FmPcdHdrIndex index, t_FmPcdFields field)
{
    switch(hdr)
    {
        case(HEADER_TYPE_NONE):
            ASSERT_COND(FALSE);
        case(HEADER_TYPE_ETH):
            switch(field.eth)
            {
                case(NET_HEADER_FIELD_ETH_DA):
                    return KG_SCH_KN_MACDST;
                case(NET_HEADER_FIELD_ETH_SA):
                    return KG_SCH_KN_MACSRC;
                case(NET_HEADER_FIELD_ETH_TYPE):
                    return KG_SCH_KN_ETYPE;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
           }
        case(HEADER_TYPE_LLC_SNAP):
            switch(field.llcSnap)
            {
                case(NET_HEADER_FIELD_LLC_SNAP_TYPE):
                    return KG_SCH_KN_ETYPE;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
           }
        case(HEADER_TYPE_VLAN):
            switch(field.vlan)
            {
                case(NET_HEADER_FIELD_VLAN_TCI):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return KG_SCH_KN_TCI1;
                    if(index == e_FM_PCD_HDR_INDEX_LAST)
                        return KG_SCH_KN_TCI2;
                    else
                    {
                        REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                        return 0;
                    }
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
            }
            break;
        case(HEADER_TYPE_MPLS):
            switch(field.mpls)
            {
                case(NET_HEADER_FIELD_MPLS_LABEL_STACK):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return KG_SCH_KN_MPLS1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return KG_SCH_KN_MPLS2;
                    if(index == e_FM_PCD_HDR_INDEX_LAST)
                        return KG_SCH_KN_MPLS_LAST;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal MPLS index"));
                    return 0;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
            }
            break;
        case(HEADER_TYPE_IPv4):
            switch(field.ipv4)
            {
                case(NET_HEADER_FIELD_IPv4_SRC_IP):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return KG_SCH_KN_IPSRC1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return KG_SCH_KN_IPSRC2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv4 index"));
                    return 0;
                case(NET_HEADER_FIELD_IPv4_DST_IP):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return KG_SCH_KN_IPDST1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return KG_SCH_KN_IPDST2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv4 index"));
                    return 0;
                case(NET_HEADER_FIELD_IPv4_PROTO):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return KG_SCH_KN_PTYPE1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return KG_SCH_KN_PTYPE2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv4 index"));
                    return 0;
                case(NET_HEADER_FIELD_IPv4_TOS):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return KG_SCH_KN_IPTOS_TC1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return KG_SCH_KN_IPTOS_TC2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv4 index"));
                    return 0;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
            }
            break;
        case(HEADER_TYPE_IPv6):
             switch(field.ipv6)
            {
                case(NET_HEADER_FIELD_IPv6_SRC_IP):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return KG_SCH_KN_IPSRC1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return KG_SCH_KN_IPSRC2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv6 index"));
                    return 0;
                case(NET_HEADER_FIELD_IPv6_DST_IP):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return KG_SCH_KN_IPDST1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return KG_SCH_KN_IPDST2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv6 index"));
                    return 0;
                case(NET_HEADER_FIELD_IPv6_NEXT_HDR):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return KG_SCH_KN_PTYPE1;
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return KG_SCH_KN_PTYPE2;
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv6 index"));
                    return 0;
                case(NET_HEADER_FIELD_IPv6_VER | NET_HEADER_FIELD_IPv6_FL | NET_HEADER_FIELD_IPv6_TC):
                    if((index == e_FM_PCD_HDR_INDEX_NONE) || (index == e_FM_PCD_HDR_INDEX_1))
                        return (KG_SCH_KN_IPV6FL1 | KG_SCH_KN_IPTOS_TC1);
                    if(index == e_FM_PCD_HDR_INDEX_2)
                        return (KG_SCH_KN_IPV6FL2 | KG_SCH_KN_IPTOS_TC2);
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal IPv6 index"));
                    return 0;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
            }
            break;
        case(HEADER_TYPE_GRE):
            switch(field.gre)
            {
                case(NET_HEADER_FIELD_GRE_TYPE):
                    return KG_SCH_KN_GREPTYPE;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
           }
        case(HEADER_TYPE_MINENCAP):
            switch(field.minencap)
            {
                case(NET_HEADER_FIELD_MINENCAP_SRC_IP):
                    return KG_SCH_KN_IPSRC2;
                case(NET_HEADER_FIELD_MINENCAP_DST_IP):
                    return KG_SCH_KN_IPDST2;
                case(NET_HEADER_FIELD_MINENCAP_TYPE):
                    return KG_SCH_KN_PTYPE2;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
           }
           break;
        case(HEADER_TYPE_TCP):
            switch(field.tcp)
            {
                case(NET_HEADER_FIELD_TCP_PORT_SRC):
                    return KG_SCH_KN_L4PSRC;
                case(NET_HEADER_FIELD_TCP_PORT_DST):
                    return KG_SCH_KN_L4PDST;
                case(NET_HEADER_FIELD_TCP_FLAGS):
                    return KG_SCH_KN_TFLG;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
            }
            break;
        case(HEADER_TYPE_UDP):
            switch(field.udp)
            {
                case(NET_HEADER_FIELD_UDP_PORT_SRC):
                    return KG_SCH_KN_L4PSRC;
                case(NET_HEADER_FIELD_UDP_PORT_DST):
                    return KG_SCH_KN_L4PDST;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
            }
            break;
        case(HEADER_TYPE_IPSEC_AH):
            switch(field.ipsecAh)
            {
                case(NET_HEADER_FIELD_IPSEC_AH_SPI):
                    return KG_SCH_KN_IPSEC_SPI;
                case(NET_HEADER_FIELD_IPSEC_AH_NH):
                    return KG_SCH_KN_IPSEC_NH;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
            }
            break;
        case(HEADER_TYPE_IPSEC_ESP):
            switch(field.ipsecEsp)
            {
                case(NET_HEADER_FIELD_IPSEC_ESP_SPI):
                    return KG_SCH_KN_IPSEC_SPI;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
            }
            break;
        case(HEADER_TYPE_SCTP):
            switch(field.sctp)
            {
                case(NET_HEADER_FIELD_SCTP_PORT_SRC):
                    return KG_SCH_KN_L4PSRC;
                case(NET_HEADER_FIELD_SCTP_PORT_DST):
                    return KG_SCH_KN_L4PDST;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
            }
            break;
        case(HEADER_TYPE_DCCP):
            switch(field.dccp)
            {
                case(NET_HEADER_FIELD_DCCP_PORT_SRC):
                    return KG_SCH_KN_L4PSRC;
                case(NET_HEADER_FIELD_DCCP_PORT_DST):
                    return KG_SCH_KN_L4PDST;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
            }
            break;
        case(HEADER_TYPE_PPPoE):
            switch(field.pppoe)
            {
                case(NET_HEADER_FIELD_PPPoE_PID):
                    return KG_SCH_KN_PPPID;
                case(NET_HEADER_FIELD_PPPoE_SID):
                    return KG_SCH_KN_PPPSID;
                default:
                    REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
                    return 0;
            }
            break;
        default:
            REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
            return 0;
    }
}

static uint8_t GetFieldSize(e_NetHeaderType hdr, t_FmPcdFields field)
{
    UNUSED(field);

    switch(hdr)
    {
        case(HEADER_TYPE_NONE):
            ASSERT_COND(FALSE);
        case(HEADER_TYPE_ETH):
        case(HEADER_TYPE_LLC_SNAP):
        case(HEADER_TYPE_VLAN):
        case(HEADER_TYPE_PPPoE):
        case(HEADER_TYPE_MPLS):
        case(HEADER_TYPE_IPv4):
        case(HEADER_TYPE_IPv6):
        case(HEADER_TYPE_GRE):
        case(HEADER_TYPE_MINENCAP):
        case(HEADER_TYPE_USER_DEFINED_L3):
        case(HEADER_TYPE_TCP):
        case(HEADER_TYPE_UDP):
        case(HEADER_TYPE_IPSEC_AH):
        case(HEADER_TYPE_IPSEC_ESP):
        case(HEADER_TYPE_SCTP):
        case(HEADER_TYPE_DCCP):
        case(HEADER_TYPE_USER_DEFINED_L4):
        default:
            REPORT_ERROR(MAJOR, E_NOT_SUPPORTED, ("Extraction not supported"));
            return 0;
    }
}

static uint8_t GetKnownFieldId(uint32_t bitMask)
{
    uint8_t cnt = 0;

    while (bitMask)
        if(bitMask & 0x80000000)
            break;
        else
        {
            cnt++;
            bitMask <<= 1;
        }
    return cnt;

}

static t_Error AllocClsPlanGrpBlocks(t_FmPcd *p_FmPcd, uint16_t sizeOfGrp, uint8_t *p_BaseEntry)
{
    uint8_t     numOfBlocks, blocksFound=0, first=0;
    uint8_t     i, j;

    numOfBlocks =  (uint8_t)(sizeOfGrp/CLS_PLAN_NUM_PER_GRP);

    TRY_LOCK_RET_ERR(p_FmPcd->lock);

    /* try to find consequent blocks */
    first = 0;
    for(i=p_FmPcd->p_FmPcdKg->clsPlanBase;i<p_FmPcd->p_FmPcdKg->numOfClsPlanEntries/CLS_PLAN_NUM_PER_GRP;)
    {
        if(!p_FmPcd->p_FmPcdKg->clsPlanUsedBlocks[i])
        {
            blocksFound++;
            i++;
            if(blocksFound == numOfBlocks)
                break;
        }
        else
        {
            blocksFound = 0;
            /* advance i to the next aligned address */
            first = i = (uint8_t)(first + numOfBlocks);
        }
    }

    if(blocksFound == numOfBlocks)
    {
        *p_BaseEntry = (uint8_t)(first*CLS_PLAN_NUM_PER_GRP);
        for(j = first; j<first + numOfBlocks; j++)
            p_FmPcd->p_FmPcdKg->clsPlanUsedBlocks[j] = TRUE;
        RELEASE_LOCK(p_FmPcd->lock);
        return E_OK;
    }
    else
    {
        RELEASE_LOCK(p_FmPcd->lock);
        RETURN_ERROR(MINOR, E_FULL, ("No recources for clsPlan"));
    }
}

static void FreeClsPlanGrpBlock(t_FmPcd *p_FmPcd, uint16_t sizeOfGrp, uint8_t baseEntry)
{
    int     i;

    for(i=baseEntry/CLS_PLAN_NUM_PER_GRP;i<(baseEntry/CLS_PLAN_NUM_PER_GRP+sizeOfGrp/CLS_PLAN_NUM_PER_GRP);i++)
    {
        ASSERT_COND( p_FmPcd->p_FmPcdKg->clsPlanUsedBlocks[i]);
        p_FmPcd->p_FmPcdKg->clsPlanUsedBlocks[i] = FALSE;
    }
}

t_Handle FmPcdKgBuildClsPlanGrp(t_Handle h_FmPcd, t_FmPcdKgClsPlanGrpParams *p_Grp, t_FmPcdKgInterModuleClsPlanSet *p_ClsPlanSet)
{
    t_FmPcd                         *p_FmPcd = (t_FmPcd*)h_FmPcd;
    protocolOpt_t                   allOptions = 0, tmpOpt;
    int                             i, j;
    uint8_t                         numOfOptions = 0, grpId;
    uint32_t                        walking1Mask, oredVectors = 0, lcvVector;
    uint8_t                         tmpEntryId;
    t_FmPcdKgClsPlanGrp             *p_ClsPlanGrp;
    t_Error                         err = E_OK;
    struct {
        protocolOpt_t   opt;
        uint32_t        vector;
    }                               tmpOptStruct[FM_PCD_MAX_NUM_OF_OPTIONS];

    SANITY_CHECK_RETURN_VALUE(p_FmPcd, E_INVALID_HANDLE, NULL);
    SANITY_CHECK_RETURN_VALUE(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE, NULL);

    TRY_LOCK_RET_NULL(p_FmPcd->lock);

    /* find a new clsPlan group */
    for(i = 0;i<PCD_MAX_NUM_OF_PORTS;i++)
        if(!p_FmPcd->p_FmPcdKg->clsPlanGrps[i].used)
            break;
    if(i== PCD_MAX_NUM_OF_PORTS)
    {
        REPORT_ERROR(MAJOR, E_FULL,("No classification plan groups available."));
        RELEASE_LOCK(p_FmPcd->lock);
        return NULL;
    }
    p_FmPcd->p_FmPcdKg->clsPlanGrps[i].used = TRUE;

    TRY_LOCK_RET_NULL(p_FmPcd->p_FmPcdKg->clsPlanGrps[i].lock);
    RELEASE_LOCK(p_FmPcd->lock);

    grpId = (uint8_t)i;

    p_ClsPlanGrp = &p_FmPcd->p_FmPcdKg->clsPlanGrps[i];
    p_ClsPlanGrp->netEnvId = (uint8_t)(CAST_POINTER_TO_UINT32(p_Grp->h_NetEnv)-1);

    p_ClsPlanGrp->owners = 0;

    FmPcdIncNetEnvOwners(p_FmPcd, (uint8_t)(CAST_POINTER_TO_UINT32(p_Grp->h_NetEnv)-1));

    /* first count all options */
    for(i=0;i<p_Grp->numOfOptions;i++)
        allOptions |= p_Grp->options[i];

    walking1Mask = 0x80000000;
    while(allOptions)
    {
        if (numOfOptions==8)
        {
            REPORT_ERROR(MAJOR, E_INVALID_VALUE,("Too many (more than 8) classification plan basic options selected."));
            RELEASE_LOCK(p_FmPcd->lock);
            return NULL;
        }

        if(allOptions & walking1Mask)
        {
            allOptions &= ~walking1Mask;
            /* the internal array now represents the single options considered.
            it's order defines the location of each option in the
            classification plan array */
            tmpOptStruct[numOfOptions].opt = walking1Mask;
            err = PcdGetVectorForOpt(p_FmPcd,
                                     (uint8_t)(CAST_POINTER_TO_UINT32(p_Grp->h_NetEnv)-1),
                                     walking1Mask,
                                     &tmpOptStruct[numOfOptions].vector);
            if(err)
            {
                REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
                RELEASE_LOCK(p_FmPcd->p_FmPcdKg->clsPlanGrps[grpId].lock);
                return NULL;
            }
            oredVectors  |= tmpOptStruct[numOfOptions].vector;
            numOfOptions++;
        }
        walking1Mask >>= 1;
    }

    /* allocate 2^numOfOptions entries */
    if(numOfOptions > FM_PCD_MAX_NUM_OF_OPTIONS)
    {
        REPORT_ERROR(MINOR, E_INVALID_VALUE, ("Too many options - no more than %d components allowed.", FM_PCD_MAX_NUM_OF_OPTIONS));
        RELEASE_LOCK(p_FmPcd->p_FmPcdKg->clsPlanGrps[grpId].lock);
        return NULL;
    }
    p_ClsPlanGrp->sizeOfGrp = (uint16_t)(1<<numOfOptions);
    /* a minimal group of 8 is required */
    if(p_ClsPlanGrp->sizeOfGrp < CLS_PLAN_NUM_PER_GRP)
        p_ClsPlanGrp->sizeOfGrp = CLS_PLAN_NUM_PER_GRP;

    err = AllocClsPlanGrpBlocks(p_FmPcd, p_ClsPlanGrp->sizeOfGrp, &p_ClsPlanGrp->baseEntry);
    if(err)
    {
        RELEASE_LOCK(p_FmPcd->p_FmPcdKg->clsPlanGrps[grpId].lock);
        REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
        return NULL;
    }

    p_ClsPlanSet->baseEntry = p_ClsPlanGrp->baseEntry;

    /* set all entries in group to be the non-option vector */
    p_ClsPlanSet->numOfClsPlanEntries = p_ClsPlanGrp->sizeOfGrp;
    for(i=0;i<p_ClsPlanGrp->sizeOfGrp;i++)
        p_ClsPlanSet->vectors[i] = ~oredVectors;

    /* now set the relevant values only for user defined option */
    for(i=0;i<p_Grp->numOfOptions;i++)
    {
        tmpEntryId = 0;
        lcvVector = ~oredVectors;
        j = 0;
        tmpOpt = p_Grp->options[i];
        while(tmpOpt)
        {
            /* find each option in the internal array */
            if((tmpOpt & tmpOptStruct[j].opt) == tmpOptStruct[j].opt)
            {
                /* clear that bit */
                tmpOpt &= ~tmpOptStruct[j].opt;
                /* j is now the internal array interesting entry */
                tmpEntryId += (1 << j);
                lcvVector |= tmpOptStruct[j].vector;
            }
            j++;
        }

        ASSERT_COND(tmpEntryId < p_ClsPlanGrp->sizeOfGrp);
        p_ClsPlanSet->vectors[tmpEntryId] = lcvVector;
    }

    /* save an array of used options - the indexes represent the power of 2 index */
    j=0;
    while(j<numOfOptions)
    {
        p_ClsPlanGrp->optArray[j] = tmpOptStruct[j].opt;
        j++;
    }

    RELEASE_LOCK(p_FmPcd->p_FmPcdKg->clsPlanGrps[grpId].lock);

    return CAST_UINT32_TO_POINTER((uint32_t)grpId+1);;
}

void FmPcdKgDestroyClsPlanGrp(t_Handle h_FmPcd, uint8_t grpId)
{
    t_FmPcd                         *p_FmPcd = (t_FmPcd*)h_FmPcd;

    /* check that no port is bound to this port */
    if(p_FmPcd->p_FmPcdKg->clsPlanGrps[grpId].owners)
    {
       REPORT_ERROR(MINOR, E_INVALID_STATE, ("Trying to delete a clsPlan grp that has ports bound to"));
       return;
    }

    FmPcdDecNetEnvOwners(p_FmPcd, p_FmPcd->p_FmPcdKg->clsPlanGrps[grpId].netEnvId);

    /* free blocks */
    FreeClsPlanGrpBlock(p_FmPcd, p_FmPcd->p_FmPcdKg->clsPlanGrps[grpId].sizeOfGrp, p_FmPcd->p_FmPcdKg->clsPlanGrps[grpId].baseEntry);

    /* clear clsPlan driver structure */
    memset(&p_FmPcd->p_FmPcdKg->clsPlanGrps[grpId], 0, sizeof(t_FmPcdKgClsPlanGrp));
}

t_Error FmPcdKgBuildBindPortToSchemes(t_Handle h_FmPcd , t_FmPcdKgInterModuleBindPortToSchemes *p_BindPort, uint32_t *p_SpReg, bool add)
{
    t_FmPcd                 *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint32_t                j, schemesPerPortVector = 0;
    t_FmPcdKgScheme         *p_Scheme;
    uint8_t                 i, relativeSchemeId;
    uint32_t                tmp, walking1Mask;
    uint16_t                pcdPortId;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE);

    /* for each scheme */
    for(i = 0; i<p_BindPort->numOfSchemes; i++)
    {
        relativeSchemeId = FmPcdKgGetRelativeSchemeId(p_FmPcd, p_BindPort->schemesIds[i]);
        if(relativeSchemeId == FM_PCD_KG_NUM_OF_SCHEMES)
            REPORT_ERROR(MAJOR, E_NOT_IN_RANGE, NO_MSG);

        if(add)
        {
            if (!FmPcdKgIsSchemeValidSw(h_FmPcd, relativeSchemeId))
                RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Requested scheme is invalid."));

            p_Scheme = &p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId];
            /* check netEnvId  of the port against the scheme netEnvId */
            if((p_Scheme->netEnvId != p_BindPort->netEnvId) && (p_Scheme->netEnvId != DRIVER_PRIVATE_NET_ENV_ID))
                RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Port may not be bound to requested scheme - differ in netEnvId"));

            /* if next engine is private port policer profile, we need to check that it is valid */
            SET_FM_PCD_PORTID_FROM_GLOBAL_PORTID(pcdPortId, p_BindPort->hardwarePortId);
            if(p_Scheme->nextRelativePlcrProfile)
            {
                for(j = 0;j<p_Scheme->numOfProfiles;j++)
                {
                    ASSERT_COND(p_FmPcd->p_FmPcdPlcr->portsMapping[pcdPortId].h_FmPort);
                    if(p_Scheme->relativeProfileId+j >= p_FmPcd->p_FmPcdPlcr->portsMapping[pcdPortId].numOfProfiles)
                        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Relative profile not in range"));
                     if(!FmPcdPlcrIsProfileValid(p_FmPcd, (uint16_t)(p_FmPcd->p_FmPcdPlcr->portsMapping[pcdPortId].profilesBase + p_Scheme->relativeProfileId + j)))
                        RETURN_ERROR(MINOR, E_INVALID_STATE, ("Relative profile not valid."));
                }
            }
            if(!p_BindPort->useClsPlan)
            {
                /* if this port does not use clsPlan, it may not be bound to schemes with units that contain
                cls plan options. Schemes that are used only directly, should not be checked.
                it also may not be bound to schemes that go to CC with units that are options  - so we OR
                the match vector and the grpBits (= ccUnits) */
                if ((p_Scheme->matchVector != SCHEME_ALWAYS_DIRECT) || p_Scheme->ccUnits)
                {
                    walking1Mask = 0x80000000;
                    tmp = (p_Scheme->matchVector == SCHEME_ALWAYS_DIRECT)? 0:p_Scheme->matchVector;
                    tmp |= p_Scheme->ccUnits;
                    while (tmp)
                    {
                        if(tmp & walking1Mask)
                        {
                            tmp &= ~walking1Mask;
                            if(!PcdNetEnvIsUnitWithoutOpts(p_FmPcd, p_Scheme->netEnvId, walking1Mask))
                                RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Port (without clsPlan) may not be bound to requested scheme - uses clsPlan options"));
                        }
                        walking1Mask >>= 1;
                    }
                }
            }
        }
        /* build vector */
        schemesPerPortVector |= 1 << (31 - p_BindPort->schemesIds[i]);
    }

    *p_SpReg = schemesPerPortVector;

    return E_OK;
}

void FmPcdKgIncSchemeOwners(t_Handle h_FmPcd , t_FmPcdKgInterModuleBindPortToSchemes *p_BindPort)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd*)h_FmPcd;
    int             i;
    t_FmPcdKgScheme *p_Scheme;

    /* for each scheme - update owners counters */
    for(i = 0; i<p_BindPort->numOfSchemes; i++)
    {
        p_Scheme = &p_FmPcd->p_FmPcdKg->schemes[p_BindPort->schemesIds[i]];

        /* increment owners number */
        p_Scheme->owners++;
    }
}

void FmPcdKgDecSchemeOwners(t_Handle h_FmPcd , t_FmPcdKgInterModuleBindPortToSchemes *p_BindPort)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd*)h_FmPcd;
    int             i;
    t_FmPcdKgScheme *p_Scheme;

    /* for each scheme - update owners counters */
    for(i = 0; i<p_BindPort->numOfSchemes; i++)
    {
        p_Scheme = &p_FmPcd->p_FmPcdKg->schemes[p_BindPort->schemesIds[i]];

        /* increment owners number */
        ASSERT_COND(p_Scheme->owners);
        p_Scheme->owners--;
    }
}

#ifdef FM_MASTER_PARTITION
t_Error  FmPcdKgAllocSchemes(t_Handle h_FmPcd, uint8_t numOfSchemes, uint8_t partitionId, uint8_t *p_SchemesIds)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint8_t             i,j;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE);

    TRY_LOCK_RET_ERR(p_FmPcd->lock);

    for(j=0,i=0;i<FM_PCD_KG_NUM_OF_SCHEMES && j<numOfSchemes;i++)
    {
        if(!p_FmPcd->p_FmPcdKg->schemesMng[i].allocated)
        {
            p_FmPcd->p_FmPcdKg->schemesMng[i].allocated = TRUE;
            p_FmPcd->p_FmPcdKg->schemesMng[i].ownerId = partitionId;
            p_SchemesIds[j] = i;
            j++;
        }
    }

    if (j != numOfSchemes)
    {
        /* roll back */
        for(i = 0;i<j;i++)
        {
            p_SchemesIds[j] = 0;
            p_FmPcd->p_FmPcdKg->schemesMng[p_SchemesIds[i]].allocated = FALSE;
            p_FmPcd->p_FmPcdKg->schemesMng[p_SchemesIds[i]].ownerId = 0;
            p_SchemesIds[i] = 0;
        }
        RELEASE_LOCK(p_FmPcd->lock);
        RETURN_ERROR(MAJOR, E_NOT_AVAILABLE, ("No schemes found"));
    }

    RELEASE_LOCK(p_FmPcd->lock);

    return E_OK;
}

t_Error  FmPcdKgFreeSchemes(t_Handle h_FmPcd, uint8_t numOfSchemes, uint8_t partitionId, uint8_t *p_SchemesIds)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint8_t             i;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE);

    TRY_LOCK_RET_ERR(p_FmPcd->lock);

    for(i=0;i<numOfSchemes;i++)
    {
        if(!p_FmPcd->p_FmPcdKg->schemesMng[p_SchemesIds[i]].allocated)
        {
            RELEASE_LOCK(p_FmPcd->lock);
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Scheme was not previously allocated"));
        }
        if(p_FmPcd->p_FmPcdKg->schemesMng[p_SchemesIds[i]].ownerId != partitionId)
        {
            RELEASE_LOCK(p_FmPcd->lock);
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Scheme is not owned by caller. "));
        }
        p_FmPcd->p_FmPcdKg->schemesMng[p_SchemesIds[i]].allocated = FALSE;
        p_FmPcd->p_FmPcdKg->schemesMng[p_SchemesIds[i]].ownerId = 0;
    }

    RELEASE_LOCK(p_FmPcd->lock);
    return E_OK;
}

t_Error  FmPcdKgAllocClsPlanEntries(t_Handle h_FmPcd, uint16_t numOfClsPlanEntries, uint8_t partitionId, uint8_t *p_First)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint8_t     numOfBlocks, blocksFound=0, first=0;
    uint8_t     i, j;

    TRY_LOCK_RET_ERR(p_FmPcd->lock);

    if(!numOfClsPlanEntries)
    {
        RELEASE_LOCK(p_FmPcd->lock);
        return E_OK;
    }

    if ((numOfClsPlanEntries % CLS_PLAN_NUM_PER_GRP) || (!POWER_OF_2(numOfClsPlanEntries)))
    {
        RELEASE_LOCK(p_FmPcd->lock);
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("numOfClsPlanEntries must be a power of 2 and divisible by 8"));
    }

    numOfBlocks =  (uint8_t)(numOfClsPlanEntries/CLS_PLAN_NUM_PER_GRP);

    /* try to find consequent blocks */
    first = 0;
    for(i=0;i<FM_PCD_MAX_NUM_OF_CLS_PLANS/CLS_PLAN_NUM_PER_GRP;)
    {
        if(!p_FmPcd->p_FmPcdKg->clsPlanBlocksMng[i].allocated)
        {
            blocksFound++;
            i++;
            if(blocksFound == numOfBlocks)
                break;
        }
        else
        {
            blocksFound = 0;
            /* advance i to the next aligned address */
            first = i = (uint8_t)(first + numOfBlocks);
        }
    }

    if(blocksFound == numOfBlocks)
    {
        *p_First = (uint8_t)(first*CLS_PLAN_NUM_PER_GRP);
        for(j = first; j<first + numOfBlocks; j++)
        {
            p_FmPcd->p_FmPcdKg->clsPlanBlocksMng[j].allocated = TRUE;
            p_FmPcd->p_FmPcdKg->clsPlanBlocksMng[j].ownerId = partitionId;
        }
        RELEASE_LOCK(p_FmPcd->lock);

        return E_OK;
    }
    else
    {
        RELEASE_LOCK(p_FmPcd->lock);
        RETURN_ERROR(MINOR, E_FULL, ("No recources for clsPlan"));
    }
}

t_Error  FmPcdKgFreeClsPlanEntries(t_Handle h_FmPcd, uint16_t numOfClsPlanEntries, uint8_t partitionId, uint8_t base)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint8_t     numOfBlocks;
    uint8_t     i;

    UNUSED( partitionId);

    TRY_LOCK_RET_ERR(p_FmPcd->lock);

    numOfBlocks =  (uint8_t)(numOfClsPlanEntries/CLS_PLAN_NUM_PER_GRP);

    for(i=base;i<base+numOfBlocks;i++)
    {
        ASSERT_COND(p_FmPcd->p_FmPcdKg->clsPlanBlocksMng[i].allocated);
        ASSERT_COND(partitionId == p_FmPcd->p_FmPcdKg->clsPlanBlocksMng[i].ownerId);
        p_FmPcd->p_FmPcdKg->clsPlanBlocksMng[i].allocated = FALSE;
        p_FmPcd->p_FmPcdKg->clsPlanBlocksMng[i].ownerId = 0;
    }
    RELEASE_LOCK(p_FmPcd->lock);
    return E_OK;
}
#endif /* FM_MASTER_PARTITION */

#ifndef CONFIG_GUEST_PARTITION /* master or single */
t_Error KgEnable(t_FmPcd *p_FmPcd)
{
    t_FmPcdKgRegs               *p_Regs = p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs;

    WRITE_UINT32(p_Regs->kggcr,GET_UINT32(p_Regs->kggcr) | FM_PCD_KG_KGGCR_EN);

    return E_OK;
}

t_Error KgDisable(t_FmPcd *p_FmPcd)
{
    t_FmPcdKgRegs               *p_Regs = p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs;

    WRITE_UINT32(p_Regs->kggcr,GET_UINT32(p_Regs->kggcr) & ~FM_PCD_KG_KGGCR_EN);

    return E_OK;
}

static t_Error WriteKgarWait(t_FmPcd *p_FmPcd, uint32_t kgar)
{
    WRITE_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgar, kgar);
    /* Wait for GO to be idle and read error */
    while ((kgar = GET_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgar)) & FM_PCD_KG_KGAR_GO) ;
    if (kgar & FM_PCD_KG_KGAR_ERR)
        RETURN_ERROR(MINOR, E_INVALID_STATE, ("Keygen scheme access violation"));
    return E_OK;
}

void KgSetClsPlan(t_Handle h_FmPcd, t_FmPcdKgInterModuleClsPlanSet *p_Set)
{
    t_FmPcd                 *p_FmPcd = (t_FmPcd*)h_FmPcd;
    t_FmPcdKgClsPlanRegs    *p_FmPcdKgPortRegs;
    uint32_t                tmpKgarReg=0;
    uint16_t                i, j;

    SANITY_CHECK_RETURN(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE);

    p_FmPcdKgPortRegs = &p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.clsPlanRegs;

    for(i=p_Set->baseEntry;i<p_Set->baseEntry+p_Set->numOfClsPlanEntries;i+=8)
    {
        tmpKgarReg = FmPcdKgBuildWriteClsPlanBlockActionReg((uint8_t)(i / CLS_PLAN_NUM_PER_GRP));

        for(j=i;j<i+8;j++)
            WRITE_UINT32(p_FmPcdKgPortRegs->kgcpe[j % CLS_PLAN_NUM_PER_GRP],p_Set->vectors[j - p_Set->baseEntry]);

        WriteKgarWait(p_FmPcd, tmpKgarReg);

    }
}

static void PcdKgErrorException(t_Handle h_FmPcd)
{
    t_FmPcd                 *p_FmPcd = (t_FmPcd *)h_FmPcd;
    uint32_t                event, schemeIndexes = 0,index = 0, mask = 0;

    event = GET_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgeer);
    mask = GET_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgeeer);

    schemeIndexes = GET_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgseer);
    schemeIndexes &= GET_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgseeer);

    /* clear the forced events */
    if(GET_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgfeer)& event)
        WRITE_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgfeer, event & ~(event & mask));

    event &= mask;

    WRITE_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgeer, event);
    WRITE_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->kgseer, schemeIndexes);

    if(event & FM_PCD_KG_DOUBLE_ECC)
        p_FmPcd->f_Exception(p_FmPcd->h_App,e_FM_PCD_KG_EXCEPTION_DOUBLE_ECC);
    if(event & FM_PCD_KG_KEYSIZE_OVERFLOW)
    {
        if(schemeIndexes)
        {
            while(schemeIndexes)
            {
                if(schemeIndexes & 0x1)
                    p_FmPcd->f_FmPcdIndexedException(p_FmPcd->h_App,e_FM_PCD_KG_EXCEPTION_KEYSIZE_OVERFLOW, (uint16_t)(31 - index));
                schemeIndexes >>= 1;
                index+=1;
            }
        }
        else /* this should happen only when interrupt is forced. */
            p_FmPcd->f_Exception(p_FmPcd->h_App,e_FM_PCD_KG_EXCEPTION_KEYSIZE_OVERFLOW);
    }
}

static t_Error KgWriteSp(t_FmPcd *p_FmPcd, uint8_t hardwarePortId, uint32_t spReg, bool add)
{
    if (p_FmPcd->h_Hc)
        return FmHcKgWriteSp(p_FmPcd->h_Hc, hardwarePortId, spReg, add);
    else
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("No HC obj. must have HC in guest partition!"));
#else
    {
        t_FmPcdKgPortConfigRegs *p_FmPcdKgPortRegs;
        uint32_t                tmpKgarReg = 0, tmpKgpeSp;
        t_Error                 err = E_OK;

        p_FmPcdKgPortRegs = &p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.portRegs;

        tmpKgarReg = FmPcdKgBuildReadPortSchemeBindActionReg(hardwarePortId);

        err = WriteKgarWait(p_FmPcd, tmpKgarReg);
        if(err)
            RETURN_ERROR(MINOR, err, NO_MSG);

        tmpKgpeSp = GET_UINT32(p_FmPcdKgPortRegs->kgoe_sp);

        if(add)
            tmpKgpeSp |= spReg;
        else
            tmpKgpeSp &= ~spReg;

        WRITE_UINT32(p_FmPcdKgPortRegs->kgoe_sp, tmpKgpeSp);

        tmpKgarReg = FmPcdKgBuildWritePortSchemeBindActionReg(hardwarePortId);

        err = WriteKgarWait(p_FmPcd, tmpKgarReg);
        if(err)
            RETURN_ERROR(MINOR, err, NO_MSG);
    }

    return E_OK;
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
}

static t_Error KgWriteCpp(t_FmPcd *p_FmPcd, uint8_t hardwarePortId, uint32_t cppReg)
{
    if (p_FmPcd->h_Hc)
        return FmHcKgWriteCpp(p_FmPcd->h_Hc, hardwarePortId, cppReg);
    else
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("No HC obj. must have HC in guest partition!"));
#else
    {
        t_FmPcdKgPortConfigRegs *p_FmPcdKgPortRegs;
        uint32_t                tmpKgarReg;

        p_FmPcdKgPortRegs = &p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.portRegs;
        WRITE_UINT32(p_FmPcdKgPortRegs->kgoe_cpp, cppReg);

        tmpKgarReg = FmPcdKgBuildWritePortClsPlanBindActionReg(hardwarePortId);
        return WriteKgarWait(p_FmPcd, tmpKgarReg);
    }
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
}

/****************************************/
/*  Internal and Inter-Module routines  */
/****************************************/
t_Error FmPcdKgBindPortToSchemes(t_Handle h_FmPcd , t_FmPcdKgInterModuleBindPortToSchemes  *p_SchemeBind)
{
    t_FmPcd                 *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint32_t                spReg;
    t_Error                 err = E_OK;

    err = FmPcdKgBuildBindPortToSchemes(h_FmPcd, p_SchemeBind, &spReg, TRUE);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    err = KgWriteSp(p_FmPcd, p_SchemeBind->hardwarePortId, spReg, TRUE);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    FmPcdKgIncSchemeOwners(h_FmPcd, p_SchemeBind);

    return E_OK;
}

t_Error FmPcdKgUnbindPortToSchemes(t_Handle h_FmPcd ,  t_FmPcdKgInterModuleBindPortToSchemes *p_SchemeBind)
{
    t_FmPcd                 *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint32_t                spReg;
    t_Error                 err = E_OK;

    err = FmPcdKgBuildBindPortToSchemes(h_FmPcd, p_SchemeBind, &spReg, FALSE);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    err = KgWriteSp(p_FmPcd, p_SchemeBind->hardwarePortId, spReg, FALSE);
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    FmPcdKgDecSchemeOwners(h_FmPcd, p_SchemeBind);

    return E_OK;
}

t_Error FmPcdKgBindPortToClsPlanGrp(t_Handle h_FmPcd, uint8_t hardwarePortId, uint8_t clsPlanGrpId)
{
    t_FmPcd                 *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint32_t                tmpKgpeCpp = 0;

    tmpKgpeCpp = FmPcdKgBuildCppReg(p_FmPcd, clsPlanGrpId);
    return KgWriteCpp(p_FmPcd, hardwarePortId, tmpKgpeCpp);
}

void FmPcdKgUnbindPortToClsPlanGrp(t_Handle h_FmPcd, uint8_t hardwarePortId)
{
    t_FmPcd                 *p_FmPcd = (t_FmPcd*)h_FmPcd;

    SANITY_CHECK_RETURN(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE);

    KgWriteCpp(p_FmPcd, hardwarePortId, 0);
}

#if 0
bool KgSchemeIsValid(t_Handle h_FmPcd, uint8_t schemeId)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint32_t    tmpReg;
    uint32_t    tmpKgarReg;
    t_Error     err = E_OK;

    SANITY_CHECK_RETURN_VALUE(p_FmPcd, E_INVALID_HANDLE, FALSE);
    SANITY_CHECK_RETURN_VALUE(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE, FALSE);
    SANITY_CHECK_RETURN_VALUE(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE, FALSE);
    SANITY_CHECK_RETURN_VALUE(schemeId<p_FmPcd->p_FmPcdKg->numOfSchemes, E_INVALID_STATE, 0);

    /* read specified scheme into scheme registers */
    tmpKgarReg = FmPcdKgBuildReadSchemeActionReg(p_FmPcd->p_FmPcdKg->schemesIds[schemeId]);
    WriteKgarWait(p_FmPcd, tmpKgarReg);

    tmpReg = GET_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs.kgse_mode);
    if (tmpReg & KG_SCH_MODE_EN)
        return TRUE;
    else
        return FALSE;
}
#endif /* 0 */

bool     FmPcdKgIsSchemeValidSw(t_Handle h_FmPcd, uint8_t schemeId)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;

    return p_FmPcd->p_FmPcdKg->schemes[schemeId].valid;
}

bool     KgIsSchemeAlwaysDirect(t_Handle h_FmPcd, uint8_t schemeId)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;

    if(p_FmPcd->p_FmPcdKg->schemes[schemeId].matchVector == SCHEME_ALWAYS_DIRECT)
        return TRUE;
    else
        return FALSE;
}

t_Handle KgConfig( t_FmPcd *p_FmPcd, t_FmPcdParams *p_FmPcdParams)
{
    t_FmPcdKg   *p_FmPcdKg;
#ifndef CONFIG_MULTI_PARTITION_SUPPORT
    uint8_t     i=0;
#endif /* !CONFIG_MULTI_PARTITION_SUPPORT */

    UNUSED(p_FmPcd);
    p_FmPcdKg = (t_FmPcdKg *) XX_Malloc(sizeof(t_FmPcdKg));
    if (!p_FmPcdKg)
    {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FM Keygen allocation FAILED"));
        return NULL;
    }
    memset(p_FmPcdKg, 0, sizeof(t_FmPcdKg));

#ifndef CONFIG_GUEST_PARTITION
    p_FmPcdKg->p_FmPcdKgRegs  = CAST_UINT64_TO_POINTER_TYPE(t_FmPcdKgRegs, (FmGetPcdKgBaseAddr(p_FmPcdParams->h_Fm)));

    p_FmPcd->exceptions |= DEFAULT_fmPcdKgErrorExceptions;
#endif /* !CONFIG_GUEST_PARTITION */

#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    p_FmPcdKg->numOfSchemes = p_FmPcdParams->numOfSchemes;
    p_FmPcdKg->numOfClsPlanEntries = p_FmPcdParams->numOfClsPlanEntries;
#else
    p_FmPcdKg->numOfSchemes = FM_PCD_KG_NUM_OF_SCHEMES;
    for(i = 0;i<FM_PCD_KG_NUM_OF_SCHEMES;i++)
        p_FmPcdKg->schemesIds[i] = i;
    p_FmPcdKg->numOfClsPlanEntries = (uint16_t)FM_PCD_MAX_NUM_OF_CLS_PLANS;
    p_FmPcdKg->clsPlanBase = 0;
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */

    p_FmPcdKg->isDriverEmptyClsPlanGrp = FALSE;

    return p_FmPcdKg;
}

t_Error KgInit(t_FmPcd *p_FmPcd)
{
    t_Error                     err = E_OK;
    t_FmPcdKgRegs               *p_Regs = p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs;
#ifndef CONFIG_GUEST_PARTITION
    int                         i;
    t_FmPcdKgPortConfigRegs     *p_FmPcdKgPortRegs;
    uint8_t                     hardwarePortId;
    uint32_t                    tmpReg;
#else
    t_FmPcdIpcKgAllocParams     kgAlloc;
#endif /* CONFIG_GUEST_PARTITION */

#ifndef CONFIG_GUEST_PARTITION
    /**********************KGEER******************/
    WRITE_UINT32(p_Regs->kgeer, (FM_PCD_KG_DOUBLE_ECC | FM_PCD_KG_KEYSIZE_OVERFLOW));
    /**********************KGEER******************/
    /**********************KGEEER******************/
    tmpReg = 0;
    if(p_FmPcd->exceptions & FM_PCD_EX_KG_DOUBLE_ECC)
    {
        FmEnableRamsEcc(p_FmPcd->h_Fm);
        tmpReg |= FM_PCD_KG_DOUBLE_ECC;
    }
    if(p_FmPcd->exceptions & FM_PCD_EX_KG_KEYSIZE_OVERFLOW)
        tmpReg |= FM_PCD_KG_KEYSIZE_OVERFLOW;
    WRITE_UINT32(p_Regs->kgeeer,tmpReg);
    /**********************KGEEER******************/

    /**********************KGFDOR******************/
    WRITE_UINT32(p_Regs->kgfdor,0);
    /**********************KGFDOR******************/

    /**********************KGGDV0R******************/
    WRITE_UINT32(p_Regs->kggdv0r,0);
    /**********************KGGDV0R******************/

    /**********************KGGDV1R******************/
    WRITE_UINT32(p_Regs->kggdv1r,0);
    /**********************KGGDV1R******************/

    /**********************KGGCR******************/
    WRITE_UINT32(p_Regs->kggcr, NIA_ENG_BMI | NIA_BMI_AC_ENQ_FRAME);
    /**********************KGGCR******************/

    /* register even if no interrupts enabled, to allow future enablement */
    FmRegisterIntr(p_FmPcd->h_Fm, e_FM_MOD_KG, 0, e_FM_INTR_TYPE_ERR, PcdKgErrorException, p_FmPcd);

    for(i=0;i<FM_PCD_MAX_NUM_OF_CLS_PLANS/CLS_PLAN_NUM_PER_GRP;i++)
        p_FmPcd->p_FmPcdKg->clsPlanUsedBlocks[i] = FALSE;

    /* clear binding between ports to schemes so that all ports are not bound to any schemes */
    p_FmPcdKgPortRegs = &p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.portRegs;
    for (i=0;i<PCD_MAX_NUM_OF_PORTS;i++)
    {

        GET_GLOBAL_PORTID_FROM_PCD_PORTS_TABLE(hardwarePortId, i);

        err = KgWriteSp(p_FmPcd, hardwarePortId, 0xffffffff, FALSE);
        if(err)
            RETURN_ERROR(MINOR, err, NO_MSG);

        err = KgWriteCpp(p_FmPcd, hardwarePortId, 0);
        if(err)
            RETURN_ERROR(MINOR, err, NO_MSG);
    }

    /* enable and enable all scheme interrupts                */
    WRITE_UINT32(p_Regs->kgseer, 0xFFFFFFFF);
    WRITE_UINT32(p_Regs->kgseeer, 0xFFFFFFFF);
#endif /* !CONFIG_GUEST_PARTITION */

    if(!p_FmPcd->p_FmPcdKg->numOfClsPlanEntries)
        /* allocate at least the minimum grp for not using clsPlan */
        p_FmPcd->p_FmPcdKg->numOfClsPlanEntries = CLS_PLAN_NUM_PER_GRP;

    /* In Multi partition, both guest and master should allocate schemes and
       clsPlan entries for future use */
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
#ifdef CONFIG_GUEST_PARTITION
    /* in GUEST_PARTITION, we use the IPC, to also set a private driver group if required */
    kgAlloc.numOfSchemes = p_FmPcd->p_FmPcdKg->numOfSchemes;
    kgAlloc.partitionId = p_FmPcd->partitionId;
    kgAlloc.numOfClsPlanEntries = p_FmPcd->p_FmPcdKg->numOfClsPlanEntries;
    err = XX_SendMessage(p_FmPcd->fmPcdModuleName, FM_PCD_ALLOC_KG_RSRC, (uint8_t*)&kgAlloc, NULL, NULL);
    if(err)
        RETURN_ERROR(MINOR, err, NO_MSG);
    memcpy(p_FmPcd->p_FmPcdKg->schemesIds , kgAlloc.schemesIds, kgAlloc.numOfSchemes);
    p_FmPcd->p_FmPcdKg->clsPlanBase = kgAlloc.clsPlanBase;
#else /* master */
    if(p_FmPcd->p_FmPcdKg->numOfSchemes)
    {
        err = FmPcdKgAllocSchemes(p_FmPcd,
                                    p_FmPcd->p_FmPcdKg->numOfSchemes,
                                    p_FmPcd->partitionId,
                                    p_FmPcd->p_FmPcdKg->schemesIds);
        if(err)
            RETURN_ERROR(MINOR, err, NO_MSG);
    }
    err = FmPcdKgAllocClsPlanEntries(p_FmPcd,
                                p_FmPcd->p_FmPcdKg->numOfClsPlanEntries,
                                p_FmPcd->partitionId,
                                &p_FmPcd->p_FmPcdKg->clsPlanBase);
    if(err)
        RETURN_ERROR(MINOR, err, NO_MSG);

#endif /* CONFIG_GUEST_PARTITION */
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */

    return E_OK;
}

#if 0
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    if(p_FmPcd->p_FmPcdKg->isDriverEmptyClsPlanGrp)
    {
        /* For guest XX_SendMessage will initialize the first 8 entries
           for the driver's empty classification plan. For master it will be set below.
           We now need to internally (sw) allocate this
           block (8 entries) in the partition p_FmPcd clsPlan structure.
           We assume that since this is the first internal allocation, the first 8 entries will be allocated */
        clsPlanGrp.netEnvId = DRIVER_PRIVATE_NET_ENV_ID;
        clsPlanGrp.numOfOptions = 0;
        err = FmPcdKgBuildClsPlanGrp(p_FmPcd, &clsPlanGrp, &clsPlanSet);
        if(err)
            RETURN_ERROR(MAJOR, err, NO_MSG);
        ASSERT_COND(clsPlanGrp.clsPlanGrpId==0);
        ASSERT_COND(clsPlanSet.baseEntry == p_FmPcd->p_FmPcdKg->clsPlanBase);

#ifdef CONFIG_GUEST_PARTITION
    err = XX_SendMessage(p_FmPcd->fmPcdModuleName, FM_PCD_ALLOC_CLS_PLAN_EMPTY_GRP, (uint8_t*)&p_FmPcd->p_FmPcdKg->clsPlanBase, NULL, NULL);
    if(err)
        RETURN_ERROR(MINOR, err, NO_MSG);

#else /* CONFIG_GUEST_PARTITION --> Master */
        memset(clsPlanSet.vectors, 0xFF, CLS_PLAN_NUM_PER_GRP*sizeof(uint32_t));
        clsPlanSet.baseEntry = p_FmPcd->p_FmPcdKg->clsPlanBase;
        clsPlanSet.numOfClsPlanEntries = CLS_PLAN_NUM_PER_GRP;
        KgSetClsPlan(p_FmPcd, &clsPlanSet);
    }
#endif /* CONFIG_GUEST_PARTITION */

    /* in master or single partition, we now allocate a private driver group if required */
#else /* ! CONFIG_MULTI_PARTITION_SUPPORT  --> Single */
        /* prepare a clsPlan group for all ports that are not using the clsPlan mechanism */
        clsPlanGrp.netEnvId = DRIVER_PRIVATE_NET_ENV_ID;
        clsPlanGrp.numOfOptions = 0;
        if(FM_PCD_KgSetClsPlanGrp(p_FmPcd, &clsPlanGrp) !=E_OK)
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Driver internal clsPlan group id should be '0'"));
#endif  /* CONFIG_MULTI_PARTITION_SUPPORT */
#endif /* 0 */

t_Error KgFree(t_FmPcd *p_FmPcd)
{
#ifdef CONFIG_GUEST_PARTITION
    t_FmPcdIpcKgAllocParams             kgAlloc;
#endif /* CONFIG_GUEST_PARTITION */
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    t_Error                             err = E_OK;
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */

    FmUnregisterIntr(p_FmPcd->h_Fm, e_FM_MOD_KG, 0, e_FM_INTR_TYPE_ERR);

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
    return E_OK;
}

t_Error FmPcdKgSwBindPortToClsPlanGrp(t_Handle h_FmPcd, uint8_t netEnvId, uint8_t clsPlanGrpId, protocolOpt_t *p_OptArray)
{
    t_FmPcd                 *p_FmPcd = (t_FmPcd *)h_FmPcd;
    t_FmPcdKgClsPlanGrp     *p_ClsPlanGrp = &p_FmPcd->p_FmPcdKg->clsPlanGrps[clsPlanGrpId];

    /* check that this clsPlan group is valid */
    if(!p_FmPcd->p_FmPcdKg->clsPlanGrps[clsPlanGrpId].used)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Invalid clsPlan group."));

    if(p_ClsPlanGrp->netEnvId != DRIVER_PRIVATE_NET_ENV_ID)
    /* When netEnvId == DRIVER_PRIVATE_NET_ENV_ID this is a special internal driver group.
    it is used by ports that do not use the classification plan mechanism, and it is of
    the minimum size - 8 entries = 1 block. In this case no check is done,
    any port from any netEnv may be bound to this group. */
        if(p_ClsPlanGrp->netEnvId != netEnvId)
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Port may not be bound to requested clsPlan - difer in netEnvId"));
    /* increment owners number */
    p_ClsPlanGrp->owners++;

    /* copy options array for port */
    memcpy(p_OptArray, &p_FmPcd->p_FmPcdKg->clsPlanGrps[clsPlanGrpId].optArray, FM_PCD_MAX_NUM_OF_OPTIONS*sizeof(protocolOpt_t));

    return E_OK;
}

void FmPcdKgSwUnbindPortToClsPlanGrp(t_Handle h_FmPcd, uint8_t clsPlanGrpId)
{
    t_FmPcd                 *p_FmPcd = (t_FmPcd *)h_FmPcd;

    t_FmPcdKgClsPlanGrp     *p_ClsPlanGrp = &p_FmPcd->p_FmPcdKg->clsPlanGrps[clsPlanGrpId];

     /* decrement owners number */
    ASSERT_COND(p_ClsPlanGrp->owners);
    p_ClsPlanGrp->owners--;
}

t_Error FmPcdKgBuildScheme(t_Handle h_FmPcd,  t_FmPcdKgSchemeParams *p_Scheme, t_FmPcdKgInterModuleSchemeRegs *p_SchemeRegs)
{
    t_FmPcd                             *p_FmPcd = (t_FmPcd *)h_FmPcd;
    uint32_t                            grpBits;
    uint8_t                             grpBase;
    bool                                direct=TRUE, absolute=FALSE;
    uint16_t                            profileId, numOfProfiles=0, relativeProfileId;
    t_Error                             err = E_OK;
    int                                 i = 0;
    t_NetEnvParams                      netEnvParams;
    uint32_t                            tmpReg, fqbTmp = 0, ppcTmp = 0, selectTmp, maskTmp, knownTmp, genTmp;
    t_FmPcdKgKeyExtractAndHashParams    *p_KeyAndHash;
    uint8_t                             j, curr;
    uint8_t                             id, shift=0, code=0, offset=0, size=0;
    t_FmPcdExtractEntry                 *p_Extract = NULL;
    t_FmPcdKgExtractedOrForFqid         *p_ExtractOr;
    bool                                generic = FALSE;
    t_KnownFieldsMasks                  bitMask;
    e_FmPcdKgExtractDfltSelect          swDefault = (e_FmPcdKgExtractDfltSelect)0;
    t_FmPcdKgSchemesExtracts            *p_LocalExtractsArray;
    uint8_t                             numOfSwDefaults = 0;
    t_FmPcdKgExtractDflt                swDefaults[NUM_OF_SW_DEFAULTS];
    uint8_t                             currGenId = 0, relativeSchemeId;

    if(!p_Scheme->modify)
        relativeSchemeId = p_Scheme->id.relativeSchemeId;
    else
        relativeSchemeId = FmPcdKgGetRelativeSchemeId(p_FmPcd, (uint8_t)(CAST_POINTER_TO_UINT32(p_Scheme->id.h_Scheme)-1));

    memset(&p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].orderedArray, 0, sizeof(t_FmPcdKgKeyOrder));
    memset(swDefaults, 0, NUM_OF_SW_DEFAULTS*sizeof(t_FmPcdKgExtractDflt));
    memset(p_SchemeRegs, 0, sizeof(t_FmPcdKgInterModuleSchemeRegs));

    /* by netEnv parameters, get match vector */
    if(!p_Scheme->alwaysDirect)
    {
        p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].netEnvId =
            (uint8_t)(CAST_POINTER_TO_UINT32(p_Scheme->netEnvParams.h_NetEnv)-1);
        netEnvParams.netEnvId = p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].netEnvId;
        netEnvParams.numOfDistinctionUnits = p_Scheme->netEnvParams.numOfDistinctionUnits;
        memcpy(netEnvParams.unitIds, p_Scheme->netEnvParams.unitIds, p_Scheme->netEnvParams.numOfDistinctionUnits);
        err = PcdGetUnitsVector(p_FmPcd, &netEnvParams);
        if(err)
            RETURN_ERROR(MAJOR, E_INVALID_STATE, NO_MSG);
        p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].matchVector = netEnvParams.vector;
    }
    else
    {
        p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].matchVector = SCHEME_ALWAYS_DIRECT;
        p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].netEnvId = DRIVER_PRIVATE_NET_ENV_ID;
    }

    if(p_Scheme->nextEngine == e_FM_PCD_PLCR)
    {
        direct = p_Scheme->kgNextEngineParams.plcrProfile.direct;
        absolute = (bool)(p_Scheme->kgNextEngineParams.plcrProfile.sharedProfile ? TRUE : FALSE);
        if(!direct && absolute)
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Indirect policing is not available when profile is shared."));

        if(direct)
        {
            profileId = p_Scheme->kgNextEngineParams.plcrProfile.profileSelect.directRelativeProfileId;
            numOfProfiles = 1;
        }
        else
        {
            profileId = p_Scheme->kgNextEngineParams.plcrProfile.profileSelect.indirectProfile.fqidOffsetRelativeProfileIdBase;
            shift = p_Scheme->kgNextEngineParams.plcrProfile.profileSelect.indirectProfile.fqidOffsetShift;
            numOfProfiles = p_Scheme->kgNextEngineParams.plcrProfile.profileSelect.indirectProfile.numOfProfiles;
        }
    }

    if(p_Scheme->nextEngine == e_FM_PCD_CC)
    {
        err = CcGetGrpParams(p_Scheme->kgNextEngineParams.cc.h_CcTree,
                             p_Scheme->kgNextEngineParams.cc.grpId,
                             &grpBits,
                             &grpBase);
        if(err)
            RETURN_ERROR(MAJOR, err, NO_MSG);
        p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].ccUnits = grpBits;

        if(p_Scheme->kgNextEngineParams.cc.plcrNext)
        {
            if(p_Scheme->kgNextEngineParams.cc.plcrProfile.sharedProfile)
                RETURN_ERROR(MAJOR, E_INVALID_STATE, ("Shared profile may not be used after Coarse classification."));
            absolute = FALSE;
            direct = p_Scheme->kgNextEngineParams.cc.plcrProfile.direct;
            if(direct)
            {
                profileId = p_Scheme->kgNextEngineParams.cc.plcrProfile.profileSelect.directRelativeProfileId;
                numOfProfiles = 1;
            }
            else
            {
                profileId = p_Scheme->kgNextEngineParams.cc.plcrProfile.profileSelect.indirectProfile.fqidOffsetRelativeProfileIdBase;
                shift = p_Scheme->kgNextEngineParams.cc.plcrProfile.profileSelect.indirectProfile.fqidOffsetShift;
                numOfProfiles = p_Scheme->kgNextEngineParams.cc.plcrProfile.profileSelect.indirectProfile.numOfProfiles;
            }
        }
    }

    /* if policer is used directly after KG, or after CC */
    if((p_Scheme->nextEngine == e_FM_PCD_PLCR)  ||
       ((p_Scheme->nextEngine == e_FM_PCD_CC) && (p_Scheme->kgNextEngineParams.cc.plcrNext)))
    {
        /* if private policer profile, it may be uninitialized yet, therefor no checks are done at this stage */
        if(absolute)
        {
            /* for absolute direct policy only, */
            relativeProfileId = profileId;
            err = FmPcdPlcrGetAbsoluteProfileId(h_FmPcd,e_FM_PCD_PLCR_SHARED,NULL, relativeProfileId, &profileId);
            if(err)
                RETURN_ERROR(MAJOR, err, ("Shared profile not valid offset"));
            if(!FmPcdPlcrIsProfileValid(p_FmPcd, profileId))
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("Shared profile not valid."));
        }
        else
        {
            /* save relative profile id's for later check */
            p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].nextRelativePlcrProfile = TRUE;
            p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].relativeProfileId = profileId;
            p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].numOfProfiles = numOfProfiles;
        }
    }

    /* configure all 21 scheme registers */
    tmpReg =  KG_SCH_MODE_EN;
    switch(p_Scheme->nextEngine)
    {
        case(e_FM_PCD_PLCR):
            /* add to mode register - NIA */
            tmpReg |= KG_SCH_MODE_NIA_PLCR;
            tmpReg |= NIA_ENG_PLCR;
            tmpReg |= (uint32_t)(p_Scheme->kgNextEngineParams.plcrProfile.sharedProfile ? NIA_PLCR_ABSOLUTE:0);
            /* initialize policer profile command - */
            /*  configure kgse_ppc  */
            if(direct)
            /* use profileId as base, other fields are 0 */
                p_SchemeRegs->kgse_ppc = (uint32_t)profileId;
            else
            {
                if(shift > MAX_PP_SHIFT)
                    RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("fqidOffsetShift may not be larger than %d", MAX_PP_SHIFT));

                if(!numOfProfiles || !POWER_OF_2(numOfProfiles))
                    RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("numOfProfiles must not be 0 and must be a power of 2"));

                ppcTmp = ((uint32_t)shift << KG_SCH_PP_SHIFT_HIGH_SHIFT) & KG_SCH_PP_SHIFT_HIGH;
                ppcTmp |= ((uint32_t)shift << KG_SCH_PP_SHIFT_LOW_SHIFT) & KG_SCH_PP_SHIFT_LOW;
                ppcTmp |= ((uint32_t)(numOfProfiles-1) << KG_SCH_PP_MASK_SHIFT);
                ppcTmp |= (uint32_t)profileId;

                p_SchemeRegs->kgse_ppc = ppcTmp;
            }
            break;
        case(e_FM_PCD_CC):
            /* mode reg - define NIA */
            tmpReg |= (NIA_ENG_FM_CTL | NIA_FM_CTL_AC_CC);

            p_SchemeRegs->kgse_ccbs = grpBits;
            tmpReg |= (uint32_t)(grpBase << KG_SCH_MODE_CCOBASE_SHIFT);

            if(p_Scheme->kgNextEngineParams.cc.plcrNext)
            {
                /* find out if absolute or relative */
                if(absolute)
                     RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("It is illegal to request a shared profile in a scheme that is in a KG->CC->PLCR flow"));
                if(direct)
                {
                    /* mask = 0, base = directProfileId */
                    p_SchemeRegs->kgse_ppc = (uint32_t)profileId;
                }
                else
                {
                    if(shift > MAX_PP_SHIFT)
                        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("fqidOffsetShift may not be larger than %d", MAX_PP_SHIFT));
                    if(!numOfProfiles || !POWER_OF_2(numOfProfiles))
                        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("numOfProfiles must not be 0 and must be a power of 2"));

                    ppcTmp = ((uint32_t)shift << KG_SCH_PP_SHIFT_HIGH_SHIFT) & KG_SCH_PP_SHIFT_HIGH;
                    ppcTmp |= ((uint32_t)shift << KG_SCH_PP_SHIFT_LOW_SHIFT) & KG_SCH_PP_SHIFT_LOW;
                    ppcTmp |= ((uint32_t)(numOfProfiles-1) << KG_SCH_PP_MASK_SHIFT);
                    ppcTmp |= (uint32_t)profileId;

                    p_SchemeRegs->kgse_ppc = ppcTmp;
                }
            }

            break;
        case(e_FM_PCD_DONE):
            tmpReg |= (NIA_ENG_BMI | NIA_BMI_AC_ENQ_FRAME);
            break;
        default:
             RETURN_ERROR(MAJOR, E_NOT_SUPPORTED, ("Next engine not supported"));
    }
    p_SchemeRegs->kgse_mode = tmpReg;

    p_SchemeRegs->kgse_mv = p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].matchVector;

    if(p_Scheme->useHash)
    {
        p_KeyAndHash = &p_Scheme->keyExtractAndHashParams;

        /*  configure kgse_dv0  */
        p_SchemeRegs->kgse_dv0 = p_KeyAndHash->privateDflt0;

        /*  configure kgse_dv1  */
        p_SchemeRegs->kgse_dv1 = p_KeyAndHash->privateDflt1;


        /*  configure kgse_ekdv  */
        tmpReg = 0;
        for( i=0 ;i<p_KeyAndHash->numOfUsedDflts ; i++)
        {
            switch(p_KeyAndHash->dflts[i].type)
            {
                case(e_FM_PCD_KG_MAC_ADDR):
                    tmpReg |= (p_KeyAndHash->dflts[i].dfltSelect << KG_SCH_DEF_MAC_ADDR_SHIFT);
                    break;
                case(e_FM_PCD_KG_TCI):
                    tmpReg |= (p_KeyAndHash->dflts[i].dfltSelect << KG_SCH_DEF_TCI_SHIFT);
                    break;
                case(e_FM_PCD_KG_ENET_TYPE):
                    tmpReg |= (p_KeyAndHash->dflts[i].dfltSelect << KG_SCH_DEF_ENET_TYPE_SHIFT);
                    break;
                case(e_FM_PCD_KG_PPP_SESSION_ID):
                    tmpReg |= (p_KeyAndHash->dflts[i].dfltSelect << KG_SCH_DEF_PPP_SESSION_ID_SHIFT);
                    break;
                case(e_FM_PCD_KG_PPP_PROTOCOL_ID):
                    tmpReg |= (p_KeyAndHash->dflts[i].dfltSelect << KG_SCH_DEF_PPP_PROTOCOL_ID_SHIFT);
                    break;
                case(e_FM_PCD_KG_MPLS_LABEL):
                    tmpReg |= (p_KeyAndHash->dflts[i].dfltSelect << KG_SCH_DEF_MPLS_LABEL_SHIFT);
                    break;
                case(e_FM_PCD_KG_IP_ADDR):
                    tmpReg |= (p_KeyAndHash->dflts[i].dfltSelect << KG_SCH_DEF_IP_ADDR_SHIFT);
                    break;
                case(e_FM_PCD_KG_PROTOCOL_TYPE):
                    tmpReg |= (p_KeyAndHash->dflts[i].dfltSelect << KG_SCH_DEF_PROTOCOL_TYPE_SHIFT);
                    break;
                case(e_FM_PCD_KG_IP_TOS_TC):
                    tmpReg |= (p_KeyAndHash->dflts[i].dfltSelect << KG_SCH_DEF_IP_TOS_TC_SHIFT);
                    break;
                case(e_FM_PCD_KG_IPV6_FLOW_LABEL):
                    tmpReg |= (p_KeyAndHash->dflts[i].dfltSelect << KG_SCH_DEF_L4_PORT_SHIFT);
                    break;
                case(e_FM_PCD_KG_IPSEC_SPI):
                    tmpReg |= (p_KeyAndHash->dflts[i].dfltSelect << KG_SCH_DEF_IPSEC_SPI_SHIFT);
                    break;
                case(e_FM_PCD_KG_L4_PORT):
                    tmpReg |= (p_KeyAndHash->dflts[i].dfltSelect << KG_SCH_DEF_L4_PORT_SHIFT);
                    break;
                case(e_FM_PCD_KG_TCP_FLAG):
                    tmpReg |= (p_KeyAndHash->dflts[i].dfltSelect << KG_SCH_DEF_TCP_FLAG_SHIFT);
                    break;
                case(e_FM_PCD_KG_GENERIC_FROM_DATA):
                    swDefaults[numOfSwDefaults].type = e_FM_PCD_KG_GENERIC_FROM_DATA;
                    swDefaults[numOfSwDefaults].dfltSelect = p_KeyAndHash->dflts[i].dfltSelect;
                    numOfSwDefaults ++;
                    break;
                case(e_FM_PCD_KG_GENERIC_FROM_DATA_NO_V):
                    swDefaults[numOfSwDefaults].type = e_FM_PCD_KG_GENERIC_FROM_DATA_NO_V;
                    swDefaults[numOfSwDefaults].dfltSelect = p_KeyAndHash->dflts[i].dfltSelect;
                    numOfSwDefaults ++;
                    break;
                case(e_FM_PCD_KG_GENERIC_NOT_FROM_DATA):
                    swDefaults[numOfSwDefaults].type = e_FM_PCD_KG_GENERIC_NOT_FROM_DATA;
                    swDefaults[numOfSwDefaults].dfltSelect = p_KeyAndHash->dflts[i].dfltSelect;
                    numOfSwDefaults ++;
                   break;
                default:
                    RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
            }
        }
        p_SchemeRegs->kgse_ekdv = tmpReg;

        p_LocalExtractsArray = (t_FmPcdKgSchemesExtracts *)XX_Malloc(sizeof(t_FmPcdKgSchemesExtracts));
        if(!p_LocalExtractsArray)
            RETURN_ERROR(MAJOR, E_NO_MEMORY, ("No memory"));
        /*  configure kgse_ekfc and  kgse_gec */
        knownTmp = 0;
        for( i=0 ;i<p_KeyAndHash->numOfUsedExtracts ; i++)
        {
            p_Extract = &p_KeyAndHash->extractArray[i];
            switch(p_Extract->type)
            {
                case(e_FM_PCD_KG_EXTRACT_PORT_PRIVATE_INFO):
                    knownTmp |= KG_SCH_KN_PORT_ID;
                    /* save in driver structure */
                    p_LocalExtractsArray->extractsArray[i].id = GetKnownFieldId(KG_SCH_KN_PORT_ID);
                    p_LocalExtractsArray->extractsArray[i].known = TRUE;
                    break;
                case(e_FM_PCD_EXTRACT_BY_HDR):
                    switch(p_Extract->extractParams.extractByHdr.type)
                    {
                        case(e_FM_PCD_EXTRACT_FROM_HDR):
                            generic = TRUE;
                            /* get the header code for the generic extract */
                            code = GetGenHdrCode(p_Extract->extractParams.extractByHdr.hdr, p_Extract->extractParams.extractByHdr.hdrIndex, p_Extract->extractParams.extractByHdr.ignoreProtocolValidation);
                            /* set generic register fields */
                            offset = p_Extract->extractParams.extractByHdr.extractByHdrType.fromHdr.offset;
                            size = p_Extract->extractParams.extractByHdr.extractByHdrType.fromHdr.size;
                            break;
                        case(e_FM_PCD_EXTRACT_FROM_FIELD):
                            generic = TRUE;
                            /* get the field code for the generic extract */
                            code = GetGenFieldCode(p_Extract->extractParams.extractByHdr.hdr,
                                        p_Extract->extractParams.extractByHdr.extractByHdrType.fromField.field, p_Extract->extractParams.extractByHdr.ignoreProtocolValidation,p_Extract->extractParams.extractByHdr.hdrIndex);
                            offset = p_Extract->extractParams.extractByHdr.extractByHdrType.fromField.offset;
                            size = p_Extract->extractParams.extractByHdr.extractByHdrType.fromField.size;
                            break;
                        case(e_FM_PCD_EXTRACT_FULL_FIELD):
                            if(!p_Extract->extractParams.extractByHdr.ignoreProtocolValidation)
                            {
                                /* if we have a known field for it - use it, otherwise use generic */
                                bitMask = GetKnownProtMask(p_Extract->extractParams.extractByHdr.hdr, p_Extract->extractParams.extractByHdr.hdrIndex,
                                            p_Extract->extractParams.extractByHdr.extractByHdrType.fullField);
                                if(bitMask)
                                {
                                    knownTmp |= bitMask;
                                    /* save in driver structure */
                                    p_LocalExtractsArray->extractsArray[i].id = GetKnownFieldId(bitMask);
                                    p_LocalExtractsArray->extractsArray[i].known = TRUE;
                                }
                                else
                                    generic = TRUE;

                            }
                            else
                                generic = TRUE;
                            if(generic)
                            {
                                /* tmp - till we cover more headers under generic */
                                RETURN_ERROR(MAJOR, E_NOT_SUPPORTED, ("Full header selection not supported"));
                                /* get the field code for the generic extract */
                                code = GetGenFieldCode(p_Extract->extractParams.extractByHdr.hdr,
                                            p_Extract->extractParams.extractByHdr.extractByHdrType.fullField, p_Extract->extractParams.extractByHdr.ignoreProtocolValidation, p_Extract->extractParams.extractByHdr.hdrIndex);
                                offset = 0; //GetFieldOffset
                                size = GetFieldSize(p_Extract->extractParams.extractByHdr.hdr, p_Extract->extractParams.extractByHdr.extractByHdrType.fullField);
                            }
                            break;
                        default:
                            RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
                    }
                    break;
                case(e_FM_PCD_EXTRACT_NON_HDR):
                    /* use generic */
                    generic = TRUE;
                    /* get the field code for the generic extract */
                    code = GetGenCode(p_Extract->extractParams.extractNonHdr.src);
                    offset = p_Extract->extractParams.extractNonHdr.offset;
                    size = p_Extract->extractParams.extractNonHdr.size;
                    break;
                default:
                    RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
            }

            if(generic)
            {
                /* set generic register fields */
                if(currGenId >= FM_PCD_KG_NUM_OF_GENERIC_REGS)
                    RETURN_ERROR(MAJOR, E_FULL, ("Generic registers are fully used"));
                if(!code)
                    RETURN_ERROR(MAJOR, E_NOT_SUPPORTED, NO_MSG);

                genTmp = KG_SCH_GEN_VALID;
                genTmp |= (uint32_t)(code << KG_SCH_GEN_HT_SHIFT);
                genTmp |= offset;
                if((size > MAX_KG_SCH_SIZE) || (size < 1))
                      RETURN_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal extraction (size out of range)"));
                genTmp |= (uint32_t)((size - 1) << KG_SCH_GEN_SIZE_SHIFT);
                swDefault = GetGenericSwDefault(swDefaults, numOfSwDefaults, code);
                if(swDefault == e_FM_PCD_KG_DFLT_ILLEGAL)
                    DBG(WARNING, ("No sw default configured"));

                genTmp |= swDefault << KG_SCH_GEN_DEF_SHIFT;
                genTmp |= KG_SCH_GEN_MASK;
                p_SchemeRegs->kgse_gec[currGenId] = genTmp;
                /* save in driver structure */
                p_LocalExtractsArray->extractsArray[i].id = currGenId++;
                p_LocalExtractsArray->extractsArray[i].known = FALSE;
                generic = FALSE;
            }
        }
        p_SchemeRegs->kgse_ekfc = knownTmp;

        selectTmp = 0;
        maskTmp = 0xFFFFFFFF;
        /*  configure kgse_bmch, kgse_bmcl and kgse_fqb */
        for( i=0 ;i<p_KeyAndHash->numOfUsedMasks ; i++)
        {
            /* Get the relative id of the extract (for known 0-0x1f, for generic 0-7) */
            id = p_LocalExtractsArray->extractsArray[p_KeyAndHash->masks[i].extractArrayIndex].id;
            /* Get the shift of the select field (depending on i) */
            GET_MASK_SEL_SHIFT(shift,i);
            if (p_LocalExtractsArray->extractsArray[p_KeyAndHash->masks[i].extractArrayIndex].known)
                selectTmp |= id << shift;
            else
                selectTmp |= (id + MASK_FOR_GENERIC_BASE_ID) << shift;

            /* Get the shift of the offset field (depending on i) - may
               be in  kgse_bmch or in kgse_fqb (depending on i) */
            GET_MASK_OFFSET_SHIFT(shift,i);
            if (i<=1)
                selectTmp |= p_KeyAndHash->masks[i].offset << shift;
            else
                fqbTmp |= p_KeyAndHash->masks[i].offset << shift;

            /* Get the shift of the mask field (depending on i) */
            GET_MASK_SHIFT(shift,i);
            /* pass all bits */
            maskTmp |= KG_SCH_BITMASK_MASK << shift;
            /* clear bits that need masking */
            maskTmp &= ~(0xFF << shift) ;
            /* set mask bits */
            maskTmp |= (p_KeyAndHash->masks[i].mask << shift) ;
        }
        p_SchemeRegs->kgse_bmch = selectTmp;
        p_SchemeRegs->kgse_bmcl = maskTmp;
        /* kgse_fqb will be written t the end of the routine */

        /*  configure kgse_hc  */
        if(!p_KeyAndHash->hashDistributionNumOfFqids || !POWER_OF_2(p_KeyAndHash->hashDistributionNumOfFqids))
            RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("hashDistributionNumOfFqids must not be 0 and must be a power of 2"));
        if(p_KeyAndHash->hashShift > MAX_HASH_SHIFT)
             RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("hashShift must not be larger than %d", MAX_HASH_SHIFT));
        if(p_KeyAndHash->hashDistributionFqidsShift > MAX_DIST_FQID_SHIFT)
             RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("hashDistributionFqidsShift must not be larger than %d", MAX_DIST_FQID_SHIFT));

        tmpReg = 0;
        tmpReg |= ((p_KeyAndHash->hashDistributionNumOfFqids - 1) << p_KeyAndHash->hashDistributionFqidsShift);
        tmpReg |= p_KeyAndHash->hashShift << KG_SCH_HASH_CONFIG_SHIFT_SHIFT;
        p_SchemeRegs->kgse_hc = tmpReg;

        /* build the return array describing the order of the extractions */

        /* the last currGenId places of the array
           are for generic extracts that are always last.
           We now sort for the calculation of the order of the known
           extractions we sort the known extracts between orderedArray[0] and
           orderedArray[p_KeyAndHash->numOfUsedExtracts - currGenId - 1].
           for the calculation of the order of the generic extractions we use:
           num_of_generic - currGenId
           num_of_known - p_KeyAndHash->numOfUsedExtracts - currGenId
           first_generic_index = num_of_known */
        curr = 0;
        for (i=0;i<p_KeyAndHash->numOfUsedExtracts ; i++)
        {
            if(p_LocalExtractsArray->extractsArray[i].known)
            {
                ASSERT_COND(curr<(p_KeyAndHash->numOfUsedExtracts - currGenId));
                j = curr;
                /* id is the extract id (port id = 0, mac src = 1 etc.). the value in the array is the original
                index in the user's extractions array */
                /* we compare the id of the current extract with the id of the extract in the orderedArray[j-1]
                location */
                while(p_LocalExtractsArray->extractsArray[i].id <
                      p_LocalExtractsArray->extractsArray[p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].orderedArray[j-1]].id && j>0)
                {
                    p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].orderedArray[j] =
                        p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].orderedArray[j-1];
                    j--;
                }
                p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].orderedArray[j] = (uint8_t)i;
                curr++;
            }
            else
                /* index is first_generic_index + generic index (id) */
                p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].orderedArray[p_KeyAndHash->numOfUsedExtracts - currGenId + p_LocalExtractsArray->extractsArray[i].id]= (uint8_t)i;

        }
        XX_Free(p_LocalExtractsArray);
        p_LocalExtractsArray = NULL;

    }
    else
    {
        /* clear all unused registers: */
        p_SchemeRegs->kgse_ekfc = 0;
        p_SchemeRegs->kgse_ekdv = 0;
        p_SchemeRegs->kgse_bmch = 0;
        p_SchemeRegs->kgse_bmcl = 0;
        p_SchemeRegs->kgse_hc = 0;
        p_SchemeRegs->kgse_dv0 = 0;
        p_SchemeRegs->kgse_dv1 = 0;
    }

    /*  configure kgse_spc  */
    if( p_Scheme->schemeCounter.update)
        p_SchemeRegs->kgse_spc = p_Scheme->schemeCounter.value;


    /* check that are enough generic registers */
    if(p_Scheme->numOfUsedFqidMasks + currGenId > FM_PCD_KG_NUM_OF_GENERIC_REGS)
        RETURN_ERROR(MAJOR, E_FULL, ("Generic registers are fully used"));

    /* extracted OR mask on Qid */
    for( i=0 ;i<p_Scheme->numOfUsedFqidMasks ; i++)
    {

        /*  configure kgse_gec[i]  */
        p_ExtractOr = &p_Scheme->fqidMasks[i];
        switch(p_ExtractOr->type)
        {
            case(e_FM_PCD_KG_EXTRACT_PORT_PRIVATE_INFO):
                code = KG_SCH_GEN_PARSE_RESULT;
                offset = 0;
                break;
            case(e_FM_PCD_EXTRACT_BY_HDR):
                /* get the header code for the generic extract */
                code = GetGenHdrCode(p_ExtractOr->extractParams.extractByHdr.hdr, p_ExtractOr->extractParams.extractByHdr.hdrIndex, p_ExtractOr->extractParams.extractByHdr.ignoreProtocolValidation);
                /* set generic register fields */
                offset = p_ExtractOr->extractionOffset;
                break;
            case(e_FM_PCD_EXTRACT_NON_HDR):
                /* get the field code for the generic extract */
                code = GetGenCode(p_ExtractOr->extractParams.src);
                offset = p_ExtractOr->extractionOffset;
                break;
            default:
                RETURN_ERROR(MAJOR, E_INVALID_SELECTION, NO_MSG);
        }

        /* set generic register fields */
        if(!code)
            RETURN_ERROR(MAJOR, E_NOT_SUPPORTED, NO_MSG);
        genTmp = KG_SCH_GEN_EXTRACT_TYPE | KG_SCH_GEN_VALID;
        genTmp |= (uint32_t)(code << KG_SCH_GEN_HT_SHIFT);
        genTmp |= offset;
        if(p_ExtractOr->bitOffsetInFqid > MAX_KG_SCH_BIT_OFFSET )
              RETURN_ERROR(MAJOR, E_NOT_SUPPORTED, ("Illegal extraction (bitOffsetInFqid out of range)"));
        genTmp |= (uint32_t)(p_ExtractOr->bitOffsetInFqid << KG_SCH_GEN_SIZE_SHIFT);
        genTmp |= (uint32_t)(p_ExtractOr->extractionOffset << KG_SCH_GEN_DEF_SHIFT);
        /* pass all bits */
        genTmp |= KG_SCH_GEN_MASK;
        /* clear bits that need masking */
        genTmp &= ~(0xFF << KG_SCH_GEN_MASK_SHIFT) ;
        /* set mask bits */
        genTmp |= (uint32_t)(p_ExtractOr->mask << KG_SCH_GEN_MASK_SHIFT);
        p_SchemeRegs->kgse_gec[currGenId++] = genTmp;

    }
    /* clear all unused GEC registers */
    for( i=currGenId ;i<FM_PCD_KG_NUM_OF_GENERIC_REGS ; i++)
        p_SchemeRegs->kgse_gec[i] = 0;

    /* add base Qid for this scheme */
    /* add configuration for kgse_fqb */
    if(p_Scheme->baseFqid & ~0x00FFFFFF)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("baseFqid must be between 1 and 2^24-1"));

    fqbTmp |= p_Scheme->baseFqid;
    p_SchemeRegs->kgse_fqb = fqbTmp;

    return E_OK;
}

void  FmPcdKgValidateSchemeSw(t_Handle h_FmPcd, uint8_t schemeId)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;

    ASSERT_COND(!p_FmPcd->p_FmPcdKg->schemes[schemeId].valid);

    FmPcdIncNetEnvOwners(p_FmPcd, p_FmPcd->p_FmPcdKg->schemes[schemeId].netEnvId);
    p_FmPcd->p_FmPcdKg->schemes[schemeId].valid = TRUE;
}

void  FmPcdKgInvalidateSchemeSw(t_Handle h_FmPcd, uint8_t schemeId)
{

    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;

    FmPcdDecNetEnvOwners(h_FmPcd, p_FmPcd->p_FmPcdKg->schemes[schemeId].netEnvId);
    p_FmPcd->p_FmPcdKg->schemes[schemeId].valid = FALSE;
}

t_Error FmPcdKgCheckInvalidateSchemeSw(t_Handle h_FmPcd, uint8_t schemeId)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;

   /* check that no port is bound to this scheme */
    if(p_FmPcd->p_FmPcdKg->schemes[schemeId].owners)
       RETURN_ERROR(MINOR, E_INVALID_STATE, ("Trying to delete a scheme that has ports bound to"));
    if(!p_FmPcd->p_FmPcdKg->schemes[schemeId].valid)
       RETURN_ERROR(MINOR, E_INVALID_STATE, ("Trying to delete an invalid scheme"));
    return E_OK;
}

uint32_t FmPcdKgBuildCppReg(t_Handle h_FmPcd, uint8_t clsPlanGrpId)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint32_t    tmpKgpeCpp;

    tmpKgpeCpp = (uint32_t)(p_FmPcd->p_FmPcdKg->clsPlanGrps[clsPlanGrpId].baseEntry / 8);
    tmpKgpeCpp |= (uint32_t)(((p_FmPcd->p_FmPcdKg->clsPlanGrps[clsPlanGrpId].sizeOfGrp / 8) - 1) << FM_PCD_KG_PE_CPP_MASK_SHIFT);
    return tmpKgpeCpp;
}

bool    FmPcdKgHwSchemeIsValid(uint32_t schemeModeReg)
{

    if(schemeModeReg & KG_SCH_MODE_EN)
        return TRUE;
    else
        return FALSE;
}

uint32_t    FmPcdKgBuildWriteSchemeActionReg(uint8_t schemeId, bool updateCounter)
{
    return     (uint32_t)((schemeId << FM_PCD_KG_KGAR_NUM_SHIFT)|
                            FM_PCD_KG_KGAR_GO |
                            FM_PCD_KG_KGAR_WRITE |
                            FM_PCD_KG_KGAR_SEL_SCHEME_ENTRY |
                            DUMMY_PORT_ID |
                            (updateCounter ? FM_PCD_KG_KGAR_SCHEME_WSEL_UPDATE_CNT:0));

}

uint32_t    FmPcdKgBuildReadSchemeActionReg(uint8_t schemeId)
{
    return     (uint32_t)((schemeId << FM_PCD_KG_KGAR_NUM_SHIFT)|
                            FM_PCD_KG_KGAR_GO |
                            FM_PCD_KG_KGAR_READ |
                            FM_PCD_KG_KGAR_SEL_SCHEME_ENTRY |
                            DUMMY_PORT_ID |
                            FM_PCD_KG_KGAR_SCHEME_WSEL_UPDATE_CNT);

}


uint32_t    FmPcdKgBuildWriteClsPlanBlockActionReg(uint8_t grpId)
{
    return (uint32_t)(FM_PCD_KG_KGAR_GO |
                        FM_PCD_KG_KGAR_WRITE |
                        FM_PCD_KG_KGAR_SEL_CLS_PLAN_ENTRY |
                        DUMMY_PORT_ID |
                        (grpId << FM_PCD_KG_KGAR_NUM_SHIFT) |
                        FM_PCD_KG_KGAR_WSEL_MASK);


        /* if we ever want to write 1 by 1, use:
        sel = (uint8_t)(0x01 << (7- (entryId % CLS_PLAN_NUM_PER_GRP)));*/
}

uint32_t    FmPcdKgBuildReadClsPlanBlockActionReg(uint8_t grpId)
{
    return (uint32_t)(FM_PCD_KG_KGAR_GO |
                        FM_PCD_KG_KGAR_READ |
                        FM_PCD_KG_KGAR_SEL_CLS_PLAN_ENTRY |
                        DUMMY_PORT_ID |
                        (grpId << FM_PCD_KG_KGAR_NUM_SHIFT) |
                        FM_PCD_KG_KGAR_WSEL_MASK);


        /* if we ever want to write 1 by 1, use:
        sel = (uint8_t)(0x01 << (7- (entryId % CLS_PLAN_NUM_PER_GRP)));*/
}

uint32_t        FmPcdKgBuildWritePortSchemeBindActionReg(uint8_t hardwarePortId)
{

    return (uint32_t)(FM_PCD_KG_KGAR_GO |
                        FM_PCD_KG_KGAR_WRITE |
                        FM_PCD_KG_KGAR_SEL_PORT_ENTRY |
                        hardwarePortId |
                        FM_PCD_KG_KGAR_SEL_PORT_WSEL_SP);
}

uint32_t        FmPcdKgBuildReadPortSchemeBindActionReg(uint8_t hardwarePortId)
{

    return (uint32_t)(FM_PCD_KG_KGAR_GO |
                        FM_PCD_KG_KGAR_READ |
                        FM_PCD_KG_KGAR_SEL_PORT_ENTRY |
                        hardwarePortId |
                        FM_PCD_KG_KGAR_SEL_PORT_WSEL_SP);
}
uint32_t        FmPcdKgBuildWritePortClsPlanBindActionReg(uint8_t hardwarePortId)
{

    return (uint32_t)(FM_PCD_KG_KGAR_GO |
                        FM_PCD_KG_KGAR_WRITE |
                        FM_PCD_KG_KGAR_SEL_PORT_ENTRY |
                        hardwarePortId |
                        FM_PCD_KG_KGAR_SEL_PORT_WSEL_CPP);
}

uint8_t FmPcdKgGetClsPlanGrpBase(t_Handle h_FmPcd, uint8_t clsPlanGrp)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;

    return p_FmPcd->p_FmPcdKg->clsPlanGrps[clsPlanGrp].baseEntry;
}

uint16_t FmPcdKgGetClsPlanGrpSize(t_Handle h_FmPcd, uint8_t clsPlanGrp)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;

    return p_FmPcd->p_FmPcdKg->clsPlanGrps[clsPlanGrp].sizeOfGrp;
}

uint8_t FmPcdKgGetSchemeSwId(t_Handle h_FmPcd, uint8_t schemeHwId)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint8_t     i;

    for(i=0;i<p_FmPcd->p_FmPcdKg->numOfSchemes;i++)
        if(p_FmPcd->p_FmPcdKg->schemesIds[i] == schemeHwId)
            return i;
    ASSERT_COND(i!=p_FmPcd->p_FmPcdKg->numOfSchemes);
    return FM_PCD_KG_NUM_OF_SCHEMES;
}

bool FmPcdKgIsEmptyClsPlanGrp(t_Handle h_FmPcd)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;

    return p_FmPcd->p_FmPcdKg->isDriverEmptyClsPlanGrp;
}

uint8_t FmPcdKgGetEmptyClsPlanGrpId(t_Handle h_FmPcd)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;

    return p_FmPcd->p_FmPcdKg->emptyClsPlanGrpId;
}

uint8_t FmPcdKgGetNumOfPartitionSchemes(t_Handle h_FmPcd)
{
    return ((t_FmPcd*)h_FmPcd)->p_FmPcdKg->numOfSchemes;
}

uint8_t FmPcdKgGetPhysicalSchemeId(t_Handle h_FmPcd, uint8_t relativeSchemeId)
{
    return ((t_FmPcd*)h_FmPcd)->p_FmPcdKg->schemesIds[relativeSchemeId];
}

uint8_t FmPcdKgGetRelativeSchemeId(t_Handle h_FmPcd, uint8_t schemeId)
{
    t_FmPcd     *p_FmPcd = (t_FmPcd*)h_FmPcd;
    uint8_t     i;

    for(i = 0;i<p_FmPcd->p_FmPcdKg->numOfSchemes;i++)
        if(p_FmPcd->p_FmPcdKg->schemesIds[i] == schemeId)
            return i;

    if(i == p_FmPcd->p_FmPcdKg->numOfSchemes)
        REPORT_ERROR(MAJOR, E_NOT_IN_RANGE, ("Scheme is out of partition range"));

    return FM_PCD_KG_NUM_OF_SCHEMES;
}

t_Error FmPcdKgSchemeTryLock(t_Handle h_FmPcd, uint8_t schemeId)
{
    TRY_LOCK_RET_ERR(((t_FmPcd*)h_FmPcd)->p_FmPcdKg->schemes[schemeId].lock);

    return E_OK;
}

void FmPcdKgReleaseSchemeLock(t_Handle h_FmPcd, uint8_t schemeId)
{
    RELEASE_LOCK(((t_FmPcd*)h_FmPcd)->p_FmPcdKg->schemes[schemeId].lock);
}

/****************************************/
/*  API routines                        */
/****************************************/
t_Error FM_PCD_KgSetAdditionalDataAfterParsing(t_Handle h_FmPcd, uint8_t payloadOffset)
{
   t_FmPcd              *p_FmPcd = (t_FmPcd*)h_FmPcd;
   t_FmPcdKgRegs        *p_Regs = p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE);

    if(payloadOffset >  256)
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("data exatraction offset from parseing end can not be more than 256"));

    WRITE_UINT32(p_Regs->kgfdor,payloadOffset);

    return E_OK;
}

t_Error FM_PCD_KgSetDfltValue(t_Handle h_FmPcd, uint8_t valueId, uint32_t value)
{
   t_FmPcd              *p_FmPcd = (t_FmPcd*)h_FmPcd;
   t_FmPcdKgRegs        *p_Regs = p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(((valueId == 0) || (valueId == 1)), E_INVALID_VALUE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE);

    if(valueId == 0)
        WRITE_UINT32(p_Regs->kggdv0r,value);
    else
        WRITE_UINT32(p_Regs->kggdv1r,value);
    return E_OK;
}

t_Error FM_PCD_KgSetEmptyClsPlanGrp(t_Handle h_FmPcd)
{
    t_FmPcd                     *p_FmPcd = (t_FmPcd*)h_FmPcd;
    t_FmPcdKgClsPlanGrpParams   clsPlanGrp;
    t_Handle                    h_ClsPlanFrp;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE);

    p_FmPcd->p_FmPcdKg->isDriverEmptyClsPlanGrp = TRUE;

    /* prepare a clsPlan group for all ports that are not using the clsPlan mechanism */
    clsPlanGrp.h_NetEnv = CAST_UINT32_TO_POINTER((uint32_t)DRIVER_PRIVATE_NET_ENV_ID+1);
    clsPlanGrp.numOfOptions = 0;
    h_ClsPlanFrp = FM_PCD_KgSetClsPlanGrp(p_FmPcd, &clsPlanGrp);
    if(!h_ClsPlanFrp)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, NO_MSG);
    p_FmPcd->p_FmPcdKg->emptyClsPlanGrpId = (uint8_t)(CAST_POINTER_TO_UINT32(h_ClsPlanFrp)-1);

    return E_OK;
}

t_Error FM_PCD_KgDeleteEmptyClsPlanGrp(t_Handle h_FmPcd)
{
   t_FmPcd  *p_FmPcd = (t_FmPcd*)h_FmPcd;
   t_Error  err = E_OK;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE);

    p_FmPcd->p_FmPcdKg->isDriverEmptyClsPlanGrp = FALSE;

    err = FM_PCD_KgDeleteClsPlanGrp(p_FmPcd, CAST_UINT32_TO_POINTER((uint32_t)(p_FmPcd->p_FmPcdKg->emptyClsPlanGrpId+1)));
    if(err)
        RETURN_ERROR(MAJOR, err, NO_MSG);

    return E_OK;
}

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
t_Error FM_PCD_KgDumpRegs(t_Handle h_FmPcd)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd*)h_FmPcd;
    int                 i = 0, j = 0;
    uint8_t             hardwarePortId;
    uint32_t            tmpKgarReg;
    t_Error             err = E_OK;

    DECLARE_DUMP;

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE);

    DUMP_SUBTITLE(("\n"));
    DUMP_TITLE(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs, ("FmPcdKgRegs Regs"));

    DUMP_VAR(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs,kggcr);
    DUMP_VAR(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs,kgeer);
    DUMP_VAR(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs,kgeeer);
    DUMP_VAR(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs,kgseer);
    DUMP_VAR(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs,kgseeer);
    DUMP_VAR(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs,kggsr);
    DUMP_VAR(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs,kgtpc);
    DUMP_VAR(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs,kgserc);
    DUMP_VAR(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs,kgfdor);
    DUMP_VAR(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs,kggdv0r);
    DUMP_VAR(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs,kggdv1r);
    DUMP_VAR(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs,kgfer);
    DUMP_VAR(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs,kgfeer);
    DUMP_VAR(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs,kgar);

    DUMP_SUBTITLE(("\n"));
    for(j = 0;j<FM_PCD_KG_NUM_OF_SCHEMES;j++)
    {
        tmpKgarReg = FmPcdKgBuildReadSchemeActionReg((uint8_t)j);
        WriteKgarWait(p_FmPcd, tmpKgarReg);

        DUMP_TITLE(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs, ("FmPcdKgIndirectAccessSchemeRegs Scheme %d Regs", j));

        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs,kgse_mode);
        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs,kgse_ekfc);
        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs,kgse_ekdv);
        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs,kgse_bmch);
        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs,kgse_bmcl);
        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs,kgse_fqb);
        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs,kgse_hc);
        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs,kgse_ppc);

        DUMP_TITLE(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs.kgse_gec, ("kgse_gec"));
        DUMP_SUBSTRUCT_ARRAY(i, FM_PCD_KG_NUM_OF_GENERIC_REGS)
        {
            DUMP_MEMORY(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs.kgse_gec[i], sizeof(uint32_t));
        }

        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs,kgse_spc);
        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs,kgse_dv0);
        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs,kgse_dv1);
        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs,kgse_ccbs);
        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs,kgse_mv);
    }
    DUMP_SUBTITLE(("\n"));

    for (i=0;i<PCD_MAX_NUM_OF_PORTS;i++)
    {

        GET_GLOBAL_PORTID_FROM_PCD_PORTS_TABLE(hardwarePortId, i);

        tmpKgarReg = FmPcdKgBuildReadPortSchemeBindActionReg(hardwarePortId);

        err = WriteKgarWait(p_FmPcd, tmpKgarReg);
        if(err)
            RETURN_ERROR(MINOR, err, NO_MSG);

        DUMP_TITLE(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.portRegs, ("FmPcdKgIndirectAccessPortRegs PCD Port %d regs", hardwarePortId));

        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.portRegs, kgoe_sp);
        DUMP_VAR(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.portRegs, kgoe_cpp);

    }

    DUMP_SUBTITLE(("\n"));
    for(j=0;j<FM_PCD_MAX_NUM_OF_CLS_PLANS/CLS_PLAN_NUM_PER_GRP;j++)
    {
        DUMP_TITLE(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.clsPlanRegs, ("FmPcdKgIndirectAccessClsPlanRegs Regs group %d", j));
        DUMP_TITLE(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.clsPlanRegs.kgcpe, ("kgcpe"));

        tmpKgarReg = FmPcdKgBuildReadClsPlanBlockActionReg((uint8_t)j);
        err = WriteKgarWait(p_FmPcd, tmpKgarReg);
        if(err)
            RETURN_ERROR(MINOR, err, NO_MSG);
        DUMP_SUBSTRUCT_ARRAY(i, 8)
            DUMP_MEMORY(&p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.clsPlanRegs.kgcpe[i], sizeof(uint32_t));
    }
    return E_OK;
}
#endif /* (defined(DEBUG_ERRORS) && ... */
#endif /* !CONFIG_GUEST_PARTITION */


t_Handle FM_PCD_KgSetScheme(t_Handle h_FmPcd,  t_FmPcdKgSchemeParams *p_Scheme)
{
    t_FmPcd                             *p_FmPcd = (t_FmPcd*)h_FmPcd;
#ifndef CONFIG_MULTI_PARTITION_SUPPORT
    uint32_t                            tmpReg;
    t_FmPcdKgInterModuleSchemeRegs      schemeRegs;
    t_FmPcdKgInterModuleSchemeRegs      *p_MemRegs;
    uint8_t                             i;
    t_Error                             err = E_OK;
    uint32_t                            tmpKgarReg;
    uint8_t                             physicalSchemeId, relativeSchemeId;
#endif /* !CONFIG_MULTI_PARTITION_SUPPORT */

    SANITY_CHECK_RETURN_VALUE(p_FmPcd, E_INVALID_HANDLE, NULL);
    SANITY_CHECK_RETURN_VALUE(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE, NULL);
    SANITY_CHECK_RETURN_VALUE(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE, NULL);

    if (p_FmPcd->h_Hc)
        return FmHcPcdKgSetScheme(p_FmPcd->h_Hc, p_Scheme);
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    else
    {
        REPORT_ERROR(MAJOR, E_INVALID_HANDLE, ("No HC obj. must have HC in guest partition!"));
        return NULL;
    }
#else /* CONFIG_MULTI_PARTITION_SUPPORT */

    TRY_LOCK_RET_NULL(p_FmPcd->lock);

    /* if not called for modification, check first that this scheme is unused */
    if(!p_Scheme->modify)
    {
        /* check that schameId is in range */
        if(p_Scheme->id.relativeSchemeId >= p_FmPcd->p_FmPcdKg->numOfSchemes)
        {
            REPORT_ERROR(MAJOR, E_NOT_IN_RANGE, ("Scheme is out of range"));
            RELEASE_LOCK(p_FmPcd->lock);
            return NULL;
        }
        relativeSchemeId = p_Scheme->id.relativeSchemeId;

        TRY_LOCK_RET_NULL(p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].lock);
        RELEASE_LOCK(p_FmPcd->lock);

        physicalSchemeId = p_FmPcd->p_FmPcdKg->schemesIds[relativeSchemeId];

        /* read specified scheme into scheme registers */
        tmpKgarReg = FmPcdKgBuildReadSchemeActionReg(physicalSchemeId);
        WriteKgarWait(p_FmPcd, tmpKgarReg);
        tmpReg = GET_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs.kgse_mode);
        if (tmpReg & KG_SCH_MODE_EN)
        {
            REPORT_ERROR(MAJOR, E_ALREADY_EXISTS, ("Scheme is already used"));
            RELEASE_LOCK(p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].lock);
            return NULL;
        }

    }
    else
    {
        physicalSchemeId = (uint8_t)(CAST_POINTER_TO_UINT32(p_Scheme->id.h_Scheme)-1);
        relativeSchemeId = FmPcdKgGetRelativeSchemeId(p_FmPcd, physicalSchemeId);

        TRY_LOCK_RET_NULL(p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].lock);
        RELEASE_LOCK(p_FmPcd->lock);

        if(relativeSchemeId == FM_PCD_KG_NUM_OF_SCHEMES)
        {
            REPORT_ERROR(MAJOR, E_NOT_IN_RANGE, NO_MSG);
            RELEASE_LOCK(p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].lock);
            return NULL;
        }
    }

    err = FmPcdKgBuildScheme(h_FmPcd, p_Scheme, &schemeRegs);
    if(err)
    {
        REPORT_ERROR(MAJOR, err, NO_MSG);
        FmPcdKgInvalidateSchemeSw(h_FmPcd, relativeSchemeId);
        RELEASE_LOCK(p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].lock);
        return NULL;
    }

    /* configure all 21 scheme registers */
    p_MemRegs = &p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs;
    WRITE_UINT32(p_MemRegs->kgse_ppc, schemeRegs.kgse_ppc);
    WRITE_UINT32(p_MemRegs->kgse_ccbs, schemeRegs.kgse_ccbs);
    WRITE_UINT32(p_MemRegs->kgse_mode, schemeRegs.kgse_mode);
    WRITE_UINT32(p_MemRegs->kgse_mv, schemeRegs.kgse_mv);
    WRITE_UINT32(p_MemRegs->kgse_dv0, schemeRegs.kgse_dv0);
    WRITE_UINT32(p_MemRegs->kgse_dv1, schemeRegs.kgse_dv1);
    WRITE_UINT32(p_MemRegs->kgse_ekdv, schemeRegs.kgse_ekdv);
    WRITE_UINT32(p_MemRegs->kgse_ekfc, schemeRegs.kgse_ekfc);
    WRITE_UINT32(p_MemRegs->kgse_bmch, schemeRegs.kgse_bmch);
    WRITE_UINT32(p_MemRegs->kgse_bmcl, schemeRegs.kgse_bmcl);
    WRITE_UINT32(p_MemRegs->kgse_hc, schemeRegs.kgse_hc);
    WRITE_UINT32(p_MemRegs->kgse_spc, schemeRegs.kgse_spc);
    WRITE_UINT32(p_MemRegs->kgse_fqb, schemeRegs.kgse_fqb);
    for(i=0 ; i<FM_PCD_KG_NUM_OF_GENERIC_REGS ; i++)
        WRITE_UINT32(p_MemRegs->kgse_gec[i], schemeRegs.kgse_gec[i]);

    /* call indirect command for scheme write */
    tmpKgarReg = FmPcdKgBuildWriteSchemeActionReg(physicalSchemeId, p_Scheme->schemeCounter.update);

    WriteKgarWait(p_FmPcd, tmpKgarReg);

    FmPcdKgValidateSchemeSw(h_FmPcd, relativeSchemeId);

    RELEASE_LOCK(p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].lock);

    return CAST_UINT32_TO_POINTER((uint32_t)physicalSchemeId+1);
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
}

t_Error  FM_PCD_KgDeleteScheme(t_Handle h_FmPcd, t_Handle h_Scheme)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd*)h_FmPcd;
#ifndef CONFIG_MULTI_PARTITION_SUPPORT
    uint8_t             physicalSchemeId;
    uint32_t            tmpKgarReg;
    t_Error             err = E_OK;
    uint8_t             relativeSchemeId;
#endif /* !CONFIG_MULTI_PARTITION_SUPPORT */

    SANITY_CHECK_RETURN_ERROR(p_FmPcd, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE);
    SANITY_CHECK_RETURN_ERROR(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE);

    if (p_FmPcd->h_Hc)
        return FmHcPcdKgDeleteScheme(p_FmPcd->h_Hc, h_Scheme);
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    else
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("No HC obj. must have HC in guest partition!"));

#else

    physicalSchemeId = (uint8_t)(CAST_POINTER_TO_UINT32(h_Scheme)-1);
    relativeSchemeId = FmPcdKgGetRelativeSchemeId(p_FmPcd, physicalSchemeId);

    if(relativeSchemeId == FM_PCD_KG_NUM_OF_SCHEMES)
        REPORT_ERROR(MAJOR, E_NOT_IN_RANGE, NO_MSG);

    TRY_LOCK_RET_ERR(p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].lock);

    /* check that no port is bound to this scheme */
    err = FmPcdKgCheckInvalidateSchemeSw(h_FmPcd, relativeSchemeId);
    if(err)
       RETURN_ERROR(MINOR, err, NO_MSG);

    /* clear mode register, including enable bit */
    WRITE_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs.kgse_mode, 0);

    /* call indirect command for scheme write */
    tmpKgarReg = FmPcdKgBuildWriteSchemeActionReg(physicalSchemeId, FALSE);

    WriteKgarWait(p_FmPcd, tmpKgarReg);

    FmPcdKgInvalidateSchemeSw(h_FmPcd, relativeSchemeId);

    RELEASE_LOCK(p_FmPcd->p_FmPcdKg->schemes[relativeSchemeId].lock);

    return E_OK;
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
}

t_Handle FM_PCD_KgSetClsPlanGrp(t_Handle h_FmPcd, t_FmPcdKgClsPlanGrpParams *p_Grp)
{
    t_FmPcd                         *p_FmPcd = (t_FmPcd*)h_FmPcd;
#ifndef CONFIG_MULTI_PARTITION_SUPPORT
    t_FmPcdKgInterModuleClsPlanSet  clsPlanSet;
    t_Handle                        h_ClsPlanGrp;
#endif /* !CONFIG_MULTI_PARTITION_SUPPORT */

    if (p_FmPcd->h_Hc)
        return FmHcPcdKgSetClsPlanGrp(p_FmPcd->h_Hc, p_Grp);
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    else
    {
        REPORT_ERROR(MAJOR, E_INVALID_HANDLE, ("No HC obj. must have HC in guest partition!"));
        return NULL;
    }
#else
    h_ClsPlanGrp = FmPcdKgBuildClsPlanGrp(p_FmPcd, p_Grp, &clsPlanSet);
    if(!h_ClsPlanGrp)
    {
        REPORT_ERROR(MAJOR, E_INVALID_HANDLE, NO_MSG);
        return NULL;
    }

    /* write clsPlan entries to memory */
    KgSetClsPlan(p_FmPcd, &clsPlanSet);


    return h_ClsPlanGrp;
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
}

t_Error FM_PCD_KgDeleteClsPlanGrp(t_Handle h_FmPcd, t_Handle h_ClsPlanGrp)
{
    t_FmPcd                         *p_FmPcd = (t_FmPcd*)h_FmPcd;
#ifndef CONFIG_MULTI_PARTITION_SUPPORT
    uint8_t                         grpId = (uint8_t)(CAST_POINTER_TO_UINT32(h_ClsPlanGrp)-1);
    t_FmPcdKgInterModuleClsPlanSet  clsPlanSet;
#endif /* !CONFIG_MULTI_PARTITION_SUPPORT */

    if (p_FmPcd->h_Hc)
        return FmHcPcdKgDeleteClsPlanGrp(p_FmPcd->h_Hc, h_ClsPlanGrp);
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    else
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("No HC obj. must have HC in guest partition!"));

#else
    /* clear clsPlan entries in memory */
    clsPlanSet.baseEntry = p_FmPcd->p_FmPcdKg->clsPlanGrps[grpId].baseEntry;
    clsPlanSet.numOfClsPlanEntries = p_FmPcd->p_FmPcdKg->clsPlanGrps[grpId].sizeOfGrp;
    memset(clsPlanSet.vectors, 0, p_FmPcd->p_FmPcdKg->clsPlanGrps[grpId].sizeOfGrp*sizeof(uint32_t));
    KgSetClsPlan(p_FmPcd, &clsPlanSet );

    FmPcdKgDestroyClsPlanGrp(h_FmPcd, grpId);

    return E_OK;
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
}

uint32_t  FM_PCD_KgGetSchemeCounter(t_Handle h_FmPcd, t_Handle h_Scheme)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd*)h_FmPcd;
#ifndef CONFIG_MULTI_PARTITION_SUPPORT
    uint32_t            tmpKgarReg;
    uint8_t             physicalSchemeId;
#endif /* !CONFIG_MULTI_PARTITION_SUPPORT */

    SANITY_CHECK_RETURN_VALUE(h_FmPcd, E_INVALID_HANDLE, 0);
    SANITY_CHECK_RETURN_VALUE(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE, 0);
    SANITY_CHECK_RETURN_VALUE(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE, 0);

    if (p_FmPcd->h_Hc)
        return FmHcPcdKgGetSchemeCounter(p_FmPcd->h_Hc, h_Scheme);
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    else
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("No HC obj. must have HC in guest partition!"));

#else
    physicalSchemeId = (uint8_t)(CAST_POINTER_TO_UINT32(h_Scheme)-1);

    if(FmPcdKgGetRelativeSchemeId(p_FmPcd, physicalSchemeId) == FM_PCD_KG_NUM_OF_SCHEMES)
        REPORT_ERROR(MAJOR, E_NOT_IN_RANGE, NO_MSG);

    tmpKgarReg = FmPcdKgBuildReadSchemeActionReg(physicalSchemeId);
    WriteKgarWait(p_FmPcd, tmpKgarReg);
    if (!(GET_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs.kgse_mode) & KG_SCH_MODE_EN))
       REPORT_ERROR(MAJOR, E_ALREADY_EXISTS, ("Scheme is Invalid"));

    return GET_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs.kgse_spc);
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
}

t_Error  FM_PCD_KgSetSchemeCounter(t_Handle h_FmPcd, t_Handle h_Scheme, uint32_t value)
{
    t_FmPcd             *p_FmPcd = (t_FmPcd*)h_FmPcd;
#ifndef CONFIG_MULTI_PARTITION_SUPPORT
    uint32_t            tmpKgarReg;
    uint8_t             physicalSchemeId;
#endif /* !CONFIG_MULTI_PARTITION_SUPPORT */

    SANITY_CHECK_RETURN_VALUE(h_FmPcd, E_INVALID_HANDLE, 0);
    SANITY_CHECK_RETURN_VALUE(p_FmPcd->p_FmPcdKg, E_INVALID_HANDLE, 0);
    SANITY_CHECK_RETURN_VALUE(!p_FmPcd->p_FmPcdDriverParam, E_INVALID_STATE, 0);

    if (p_FmPcd->h_Hc)
        return FmHcPcdKgSetSchemeCounter(p_FmPcd->h_Hc, h_Scheme, value);
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    else
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("No HC obj. must have HC in guest partition!"));

#else
    physicalSchemeId = (uint8_t)(CAST_POINTER_TO_UINT32(h_Scheme)-1);
    /* check that schameId is in range */
    if(FmPcdKgGetRelativeSchemeId(p_FmPcd, physicalSchemeId) == FM_PCD_KG_NUM_OF_SCHEMES)
        REPORT_ERROR(MAJOR, E_NOT_IN_RANGE, NO_MSG);

    /* read specified scheme into scheme registers */
    tmpKgarReg = FmPcdKgBuildReadSchemeActionReg(physicalSchemeId);
    WriteKgarWait(p_FmPcd, tmpKgarReg);
    if (!(GET_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs.kgse_mode) & KG_SCH_MODE_EN))
       RETURN_ERROR(MAJOR, E_ALREADY_EXISTS, ("Scheme is Invalid"));

    /* change counter value */
    WRITE_UINT32(p_FmPcd->p_FmPcdKg->p_FmPcdKgRegs->indirectAccessRegs.schemeRegs.kgse_spc, value);

    /* call indirect command for scheme write */
    tmpKgarReg = FmPcdKgBuildWriteSchemeActionReg(physicalSchemeId, TRUE);

    WriteKgarWait(p_FmPcd, tmpKgarReg);

    return E_OK;
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
}
