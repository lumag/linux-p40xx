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

#include "error_ext.h"
#include "std_ext.h"
#include "string_ext.h"
#include "part_ext.h"
#include "xx_ext.h"

#include "P4080.h"


/*****************************************************************************/
static e_ModuleId GetModuleIdByBase(t_Handle h_P4080, uint32_t baseAddress)
{
    t_P4080   *p_P4080 = (t_P4080 *)h_P4080;
    e_ModuleId  moduleId;

    SANITY_CHECK_RETURN_VALUE(p_P4080, E_INVALID_HANDLE, e_MODULE_ID_DUMMY_LAST);

    for (moduleId = (e_ModuleId)0; moduleId < e_MODULE_ID_DUMMY_LAST; moduleId++)
    {
        if (baseAddress == p_P4080->baseAddresses[moduleId])
        {
            return moduleId;
        }
    }

    return e_MODULE_ID_DUMMY_LAST;
}


/*****************************************************************************/
t_Handle P4080_ConfigAndInit(uint32_t baseAddress)
{
    t_P4080   *p_P4080;
    t_Error     errCode;

    p_P4080 = (t_P4080 *)XX_Malloc(sizeof(t_P4080));
    if (!p_P4080)
    {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("P4080 driver structure"));
        return NULL;
    }

    memset(p_P4080, 0, sizeof(t_P4080));

    /* Initialize 'part' parameters with P4080 parameters and service routines. */
    ((t_Part *)p_P4080)->f_GetModuleBase = P4080_GetModuleBase;
    ((t_Part *)p_P4080)->f_GetModuleIdByBase = GetModuleIdByBase;
    ((t_Part *)p_P4080)->f_GetRevInfo = (t_RevInfoCallback *)P4080_GetRevInfo;

    errCode = P4080_MngInit(p_P4080, baseAddress);
    if (errCode != E_OK)
    {
        P4080_Free(p_P4080);
        REPORT_ERROR(MAJOR, errCode, NO_MSG);
        return NULL;
    }

    return p_P4080;
}

/*****************************************************************************/
t_Error P4080_Free(t_Handle h_P4080)
{
    t_P4080 *p_P4080 = (t_P4080 *)h_P4080;

    SANITY_CHECK_RETURN_ERROR(p_P4080, E_INVALID_HANDLE);

    P4080_MngFree(p_P4080);

    XX_Free(p_P4080);

    return E_OK;
}

/*****************************************************************************/
uint32_t P4080_GetModuleBase(t_Handle h_P4080, e_ModuleId module)
{
    t_P4080 *p_P4080 = (t_P4080 *)h_P4080;

    SANITY_CHECK_RETURN_VALUE(p_P4080, E_INVALID_HANDLE, 0);

    return p_P4080->baseAddresses[module];
}

/*****************************************************************************/
uint32_t P4080_GetPramSize(t_Handle h_P4080)
{
    SANITY_CHECK_RETURN_VALUE(h_P4080, E_INVALID_HANDLE, 0);

    UNUSED(h_P4080);
    return FM_MURAM_SIZE;
}

/*****************************************************************************/
e_P4080DeviceName P4080_GetRevInfo(t_Handle h_P4080)
{
    t_P4080  *p_P4080 = (t_P4080 *)h_P4080;

    SANITY_CHECK_RETURN_VALUE(p_P4080, E_INVALID_HANDLE, e_P4080_REV_INVALID);

    REPORT_ERROR(MINOR, E_NOT_SUPPORTED, NO_MSG);
    return e_P4080_REV_INVALID;
}

/*****************************************************************************/
t_Error P4080_GetE500Factor(t_Handle h_P4080, uint32_t *p_E500MulFactor, uint32_t *p_E500DivFactor)
{
    t_P4080   *p_P4080 = (t_P4080 *)h_P4080;

    SANITY_CHECK_RETURN_ERROR(p_P4080, E_INVALID_HANDLE);

    *p_E500MulFactor = 8;
    *p_E500DivFactor = 3;
    return E_OK;
}

/*****************************************************************************/
uint32_t P4080_GetCcbFactor(t_Handle h_P4080)
{
    t_P4080   *p_P4080 = (t_P4080 *)h_P4080;

    SANITY_CHECK_RETURN_VALUE(p_P4080, E_INVALID_HANDLE, 0);

    return 8;
}
