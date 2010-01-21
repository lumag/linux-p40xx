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
 @File          FM_muram.c

 @Description   FM MURAM ...
*//***************************************************************************/
#include "error_ext.h"
#include "std_ext.h"
#include "mm_ext.h"
#include "fm_muram_ext.h"


#define __ERR_MODULE__  MODULE_FM_MURAM


t_Handle FM_MURAM_ConfigAndInit(uint64_t baseAddress, uint32_t size)
{
    t_Handle h_Mem;

    if(!baseAddress)
    {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("baseAddress 0 is not supported"));
        return NULL;
    }

    if(baseAddress%4)
    {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("baseAddress not 4 bytes aligned!"));
        return NULL;
    }

#ifndef VERIFICATION_SUPPORT
    IOMemSet32(CAST_UINT64_TO_POINTER(baseAddress), 0, size);
#endif /* VERIFICATION_SUPPORT */

    if (MM_Init(&h_Mem, baseAddress, size) != E_OK)
        return NULL;
    if (!h_Mem)
        REPORT_ERROR(MAJOR, E_INVALID_HANDLE, ("FM-MURAM partition!!!"));

    return h_Mem;
}

t_Error  FM_MURAM_Free(t_Handle h_FmMuram)
{
    if (h_FmMuram)
        MM_Free(h_FmMuram);

    return E_OK;
}

void  * FM_MURAM_AllocMem(t_Handle h_FmMuram, uint32_t size, uint32_t align)
{
    uint64_t addr;

    SANITY_CHECK_RETURN_VALUE(h_FmMuram, E_INVALID_HANDLE, NULL);

    addr = MM_Get(h_FmMuram, size, (int32_t)align ,"FM MURAM");

    if(addr == ILLEGAL_BASE)
        return NULL;

    return CAST_UINT64_TO_POINTER(addr);
}

t_Error FM_MURAM_FreeMem(t_Handle h_FmMuram, void *ptr)
{
    SANITY_CHECK_RETURN_ERROR(h_FmMuram, E_INVALID_HANDLE);

    if (MM_Put(h_FmMuram, CAST_POINTER_TO_UINT64(ptr)) == 0)
        RETURN_ERROR(MINOR, E_INVALID_HANDLE, ("memory pointer!!!"));

    return E_OK;
}
