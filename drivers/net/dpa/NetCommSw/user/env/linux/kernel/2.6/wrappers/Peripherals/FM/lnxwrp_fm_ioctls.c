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

/**************************************************************************//**
 @File          lnxwrp_fm.c

 @Author        Shlomi Gridish

 @Description   FM Linux wrapper functions.
*//***************************************************************************/

/* Linux Headers ------------------- */
#include <linux/version.h>

#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#define MODVERSIONS
#endif
#ifdef MODVERSIONS
#include <config/modversions.h>
#endif /* MODVERSIONS */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <asm/uaccess.h>
#include <asm/errno.h>
#include <sysdev/fsl_soc.h>

/* NetCommSw Headers --------------- */
#include "std_ext.h"
#include "error_ext.h"
#include "sprint_ext.h"
#include "sys_io_ext.h"

#include "fm_ioctls.h"
#include "fm_pcd_ioctls.h"
#include "fm_port_ioctls.h"

#include "lnxwrp_fm.h"


static t_Error LnxwrpFmPcdIOCTL(t_LnxWrpFmDev *p_LnxWrpFmDev, unsigned int cmd, unsigned long arg)
{
    t_Error err = E_READ_FAILED;

    switch (cmd)
    {
        case FM_PCD_IOC_PRS_LOAD_SW:
            break;

        case FM_PCD_IOC_ENABLE:
            return FM_PCD_Enable(p_LnxWrpFmDev->h_PcdDev);
        case FM_PCD_IOC_DISABLE:
            return FM_PCD_Disable(p_LnxWrpFmDev->h_PcdDev);

        case FM_PCD_IOC_FORCE_INTR:
        {
            int exception;
            if (get_user(exception, (int *)arg))
                break;
            return FM_PCD_ForceIntr(p_LnxWrpFmDev->h_PcdDev, (e_FmPcdExceptions)exception);
        }

        case FM_PCD_IOC_SET_EXCEPTION:
        {
            ioc_fm_pcd_exception_params_t *param;

            param = (ioc_fm_pcd_exception_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_exception_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_pcd_exception_params_t *)arg, sizeof(ioc_fm_pcd_exception_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            err = FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev, param->exception, param->enable);
            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_KG_SET_ADDITIONAL_DATA_AFTER_PARSING:
        {
            int payloadOffset;
            if (get_user(payloadOffset, (int *)arg))
                break;
            return FM_PCD_KgSetAdditionalDataAfterParsing(p_LnxWrpFmDev->h_PcdDev, (uint8_t)payloadOffset);
        }

        case FM_PCD_IOC_KG_SET_EMPTY_CLS_PLAN_GRP:
            return FM_PCD_KgSetEmptyClsPlanGrp(p_LnxWrpFmDev->h_PcdDev);
        case FM_PCD_IOC_KG_DELETE_EMPTY_CLS_PLAN_GRP:
            return FM_PCD_KgDeleteEmptyClsPlanGrp(p_LnxWrpFmDev->h_PcdDev);

        case FM_PCD_IOC_KG_SET_DFLT_VALUE:
        {
            ioc_fm_pcd_kg_dflt_value_params_t *param;

            param = (ioc_fm_pcd_kg_dflt_value_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_kg_dflt_value_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_pcd_kg_dflt_value_params_t *)arg, sizeof(ioc_fm_pcd_kg_dflt_value_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            err = FM_PCD_KgSetDfltValue(p_LnxWrpFmDev->h_PcdDev, param->valueId, param->value);
            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_SET_NET_ENV_CHARACTERISTICS:
        {
            ioc_fm_pcd_net_env_params_t  *param;

            param = (ioc_fm_pcd_net_env_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_net_env_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_pcd_net_env_params_t *)arg, sizeof(ioc_fm_pcd_net_env_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            param->id = FM_PCD_SetNetEnvCharacteristics(p_LnxWrpFmDev->h_PcdDev, (t_FmPcdNetEnvParams*)param);
            if (param->id && !copy_to_user((ioc_fm_pcd_net_env_params_t *)arg, param, sizeof(ioc_fm_pcd_net_env_params_t)))
                err = E_OK;
            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_DELETE_NET_ENV_CHARACTERISTICS:
        {
            ioc_fm_obj_t id;
            if (copy_from_user(&id, (ioc_fm_obj_t *)arg, sizeof(ioc_fm_obj_t)))
                break;
            return FM_PCD_DeleteNetEnvCharacteristics(p_LnxWrpFmDev->h_PcdDev, id.obj);
        }

        case FM_PCD_IOC_KG_SET_CLS_PLAN_GRP:
        {
            ioc_fm_pcd_kg_cls_plan_grp_params_t *param;

            param = (ioc_fm_pcd_kg_cls_plan_grp_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_kg_cls_plan_grp_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_pcd_kg_cls_plan_grp_params_t *)arg, sizeof(ioc_fm_pcd_kg_cls_plan_grp_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            param->id = FM_PCD_KgSetClsPlanGrp(p_LnxWrpFmDev->h_PcdDev, (t_FmPcdKgClsPlanGrpParams*)param);
            if (param->id && !copy_to_user((ioc_fm_pcd_kg_cls_plan_grp_params_t *)arg, param, sizeof(ioc_fm_pcd_kg_cls_plan_grp_params_t)))
                err = E_OK;

            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_KG_DEL_CLS_PLAN:
        {
            ioc_fm_obj_t id;
            if (copy_from_user(&id, (ioc_fm_obj_t *)arg, sizeof(ioc_fm_obj_t)))
                break;
            return FM_PCD_KgDeleteClsPlanGrp(p_LnxWrpFmDev->h_PcdDev, id.obj);
        }

        case FM_PCD_IOC_KG_SET_SCHEME:
        {
            ioc_fm_pcd_kg_scheme_params_t               *param;

            param = (ioc_fm_pcd_kg_scheme_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_kg_scheme_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_pcd_kg_scheme_params_t *)arg, sizeof(ioc_fm_pcd_kg_scheme_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            param->id = FM_PCD_KgSetScheme(p_LnxWrpFmDev->h_PcdDev, (t_FmPcdKgSchemeParams*)param);
            if (param->id && !copy_to_user((ioc_fm_pcd_kg_scheme_params_t *)arg, param, sizeof(ioc_fm_pcd_kg_scheme_params_t)))
                err = E_OK;

            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_KG_DEL_SCHEME:
        {
            ioc_fm_obj_t id;
            if (copy_from_user(&id, (ioc_fm_obj_t *)arg, sizeof(ioc_fm_obj_t)))
                break;
            return FM_PCD_KgDeleteScheme(p_LnxWrpFmDev->h_PcdDev, id.obj);
        }

        case FM_PCD_IOC_CC_SET_NODE:
        {
            ioc_fm_pcd_cc_node_params_t *param;
            uint8_t                     gbl_mask[4];
            uint8_t                     *keys;
            uint8_t                     *masks;
            int                         i,k;

            param = (ioc_fm_pcd_cc_node_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_cc_node_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            keys = (uint8_t *)XX_Malloc(sizeof(uint8_t)*IOC_FM_PCD_MAX_NUM_OF_KEYS*IOC_FM_PCD_MAX_SIZE_OF_KEY);
            if (!keys)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD keys"));
            memset(keys, 0, sizeof(uint8_t)*IOC_FM_PCD_MAX_NUM_OF_KEYS*IOC_FM_PCD_MAX_SIZE_OF_KEY);

            masks = (uint8_t *)XX_Malloc(sizeof(uint8_t)*IOC_FM_PCD_MAX_NUM_OF_KEYS*IOC_FM_PCD_MAX_SIZE_OF_KEY);
            if (!masks)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD masks"));
            memset(masks, 0, sizeof(uint8_t)*IOC_FM_PCD_MAX_NUM_OF_KEYS*IOC_FM_PCD_MAX_SIZE_OF_KEY);

            if (copy_from_user(param, (ioc_fm_pcd_cc_node_params_t *)arg, sizeof(ioc_fm_pcd_cc_node_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            if (param->keys_params.p_glbl_mask &&
                (param->keys_params.key_size <= 4)) {
                if (copy_from_user(&gbl_mask,
                                   param->keys_params.p_glbl_mask,
                                   param->keys_params.key_size)) {
                    XX_Free(masks);
                    XX_Free(keys);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
                param->keys_params.p_glbl_mask = gbl_mask;
            }

            for (i=0,k=0; i<param->keys_params.num_of_keys; i++, k+=IOC_FM_PCD_MAX_SIZE_OF_KEY) {
                if (copy_from_user(&keys[k],
                               param->keys_params.key_params[i].p_key,
                               param->keys_params.key_size)) {
                    XX_Free(masks);
                    XX_Free(keys);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
                param->keys_params.key_params[i].p_key = &keys[k];

                if (param->keys_params.key_params[i].p_mask) {
                    if (copy_from_user(&masks[k],
                                   param->keys_params.key_params[i].p_mask,
                                   param->keys_params.key_size)) {
                        XX_Free(masks);
                        XX_Free(keys);
                        XX_Free(param);
                        RETURN_ERROR(MINOR, err, NO_MSG);
                    }
                    param->keys_params.key_params[i].p_mask = &masks[k];
                }
            }
            param->id = FM_PCD_CcSetNode(p_LnxWrpFmDev->h_PcdDev, (t_FmPcdCcNodeParams*)param);
            if (param->id && !copy_to_user((ioc_fm_pcd_cc_node_params_t *)arg, param, sizeof(ioc_fm_pcd_cc_node_params_t)))
                err = E_OK;

            XX_Free(masks);
            XX_Free(keys);
            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_CC_DELETE_NODE:
        {
            ioc_fm_obj_t id;
            if (copy_from_user(&id, (ioc_fm_obj_t *)arg, sizeof(ioc_fm_obj_t)))
                break;
            return FM_PCD_CcDeleteNode(p_LnxWrpFmDev->h_PcdDev, id.obj);
        }

        case FM_PCD_IOC_CC_BUILD_TREE:
        {
            ioc_fm_pcd_cc_tree_params_t         *param;
            ioc_fm_pcd_cc_next_engine_params_t  *next_engine_per_entries_in_grp;
            uint8_t                             numOfEntriesInGroup;
            int                                 i;

            param = (ioc_fm_pcd_cc_tree_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_cc_tree_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_pcd_cc_tree_params_t *)arg, sizeof(ioc_fm_pcd_cc_tree_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            for (i=0; i<param->num_of_groups; i++) {
                if (param->fm_pcd_cc_group_params[i].p_next_engine_per_entries_in_grp) {
                    numOfEntriesInGroup = (uint8_t)( 0x01 << param->fm_pcd_cc_group_params[i].num_of_distinction_units);
                    if ((numOfEntriesInGroup ==0 ) || (numOfEntriesInGroup >= 16)) {
                        XX_Free(param);
                        RETURN_ERROR(MINOR, E_INVALID_VALUE, ("numOfEntriesInGroup"));
                    }

                    next_engine_per_entries_in_grp =
                        (ioc_fm_pcd_cc_next_engine_params_t *)XX_Malloc(numOfEntriesInGroup*sizeof(ioc_fm_pcd_cc_next_engine_params_t));
                    if (!next_engine_per_entries_in_grp)
                        RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

                    if (copy_from_user(next_engine_per_entries_in_grp,
                                       param->fm_pcd_cc_group_params[i].p_next_engine_per_entries_in_grp,
                                       numOfEntriesInGroup*sizeof(ioc_fm_pcd_cc_next_engine_params_t))) {
                        XX_Free(param);
                        RETURN_ERROR(MINOR, err, NO_MSG);
                    }
                    param->fm_pcd_cc_group_params[i].p_next_engine_per_entries_in_grp = next_engine_per_entries_in_grp;
                }
            }

            param->id = FM_PCD_CcBuildTree(p_LnxWrpFmDev->h_PcdDev, (t_FmPcdCcTreeParams*)param);
            if (param->id && !copy_to_user((ioc_fm_pcd_cc_tree_params_t *)arg, param, sizeof(ioc_fm_pcd_cc_tree_params_t)))
                err = E_OK;

            for (i=0; i<param->num_of_groups; i++)
                if (param->fm_pcd_cc_group_params[i].p_next_engine_per_entries_in_grp)
                    XX_Free(param->fm_pcd_cc_group_params[i].p_next_engine_per_entries_in_grp);
            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_CC_DELETE_TREE:
        {
            ioc_fm_obj_t id;
            if (copy_from_user(&id, (ioc_fm_obj_t *)arg, sizeof(ioc_fm_obj_t)))
                break;
            return FM_PCD_CcDeleteTree(p_LnxWrpFmDev->h_PcdDev, id.obj);
        }

        case FM_PCD_IOC_PLCR_SET_PROFILE:
        {
            ioc_fm_pcd_plcr_profile_params_t            *param;

            param = (ioc_fm_pcd_plcr_profile_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_plcr_profile_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_pcd_plcr_profile_params_t *)arg, sizeof(ioc_fm_pcd_plcr_profile_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            if (!param->modify &&
                (((t_FmPcdPlcrProfileParams*)param)->id.newParams.profileType != e_FM_PCD_PLCR_SHARED))
            {
                t_Handle h_Port;

                switch (((fm_pcd_port_params_t*)((t_FmPcdPlcrProfileParams*)param)->id.newParams.h_FmPort)->port_type)
                {
                    case (e_IOC_FM_PORT_TYPE_OFFLINE_PARSING):
                        h_Port = p_LnxWrpFmDev->opPorts[((fm_pcd_port_params_t*)((t_FmPcdPlcrProfileParams*)param)->id.newParams.h_FmPort)->port_id].h_Dev;
                        break;
                    case (e_IOC_FM_PORT_TYPE_RX):
                        h_Port = p_LnxWrpFmDev->rxPorts[((fm_pcd_port_params_t*)((t_FmPcdPlcrProfileParams*)param)->id.newParams.h_FmPort)->port_id].h_Dev;
                        break;
                    case (e_IOC_FM_PORT_TYPE_RX_10G):
                        h_Port = p_LnxWrpFmDev->rxPorts[((fm_pcd_port_params_t*)((t_FmPcdPlcrProfileParams*)param)->id.newParams.h_FmPort)->port_id + FM_MAX_NUM_OF_1G_RX_PORTS].h_Dev;
                        break;
                    default:
                        XX_Free(param);
                        RETURN_ERROR(MINOR, E_INVALID_SELECTION, NO_MSG);
                }

                ((t_FmPcdPlcrProfileParams*)param)->id.newParams.h_FmPort = h_Port;
            }

            param->id = FM_PCD_PlcrSetProfile(p_LnxWrpFmDev->h_PcdDev, (t_FmPcdPlcrProfileParams*)param);
            if (param->id && !copy_to_user((ioc_fm_pcd_plcr_profile_params_t *)arg, param, sizeof(ioc_fm_pcd_plcr_profile_params_t)))
                err = E_OK;

            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_PLCR_DEL_PROFILE:
        {
            ioc_fm_obj_t id;
            if (copy_from_user(&id, (ioc_fm_obj_t *)arg, sizeof(ioc_fm_obj_t)))
                break;
            return FM_PCD_PlcrDeleteProfile(p_LnxWrpFmDev->h_PcdDev, id.obj);
        }

        case FM_PCD_IOC_CC_TREE_MODIFY_NEXT_ENGINE:
        {
            ioc_fm_pcd_cc_tree_modify_next_engine_params_t            *param;

            param = (ioc_fm_pcd_cc_tree_modify_next_engine_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_cc_tree_modify_next_engine_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_pcd_cc_tree_modify_next_engine_params_t *)arg, sizeof(ioc_fm_pcd_cc_tree_modify_next_engine_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            err = FM_PCD_CcTreeModifyNextEngine(p_LnxWrpFmDev->h_PcdDev,
                                                param->id,
                                                param->grp_indx,
                                                param->indx,
                                                (t_FmPcdCcNextEngineParams*)(&param->cc_next_engine_params));
            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_CC_NODE_MODIFY_NEXT_ENGINE:
        {
            ioc_fm_pcd_cc_node_modify_next_engine_params_t            *param;

            param = (ioc_fm_pcd_cc_node_modify_next_engine_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_cc_node_modify_next_engine_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_pcd_cc_node_modify_next_engine_params_t *)arg, sizeof(ioc_fm_pcd_cc_node_modify_next_engine_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            err = FM_PCD_CcNodeModifyNextEngine(p_LnxWrpFmDev->h_PcdDev,
                                                param->id,
                                                param->key_indx,
                                                (t_FmPcdCcNextEngineParams*)(&param->cc_next_engine_params));
            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_CC_NODE_MODIFY_MISS_NEXT_ENGINE:
        {
            ioc_fm_pcd_cc_node_modify_next_engine_params_t            *param;

            param = (ioc_fm_pcd_cc_node_modify_next_engine_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_cc_node_modify_next_engine_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_pcd_cc_node_modify_next_engine_params_t *)arg, sizeof(ioc_fm_pcd_cc_node_modify_next_engine_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            err = FM_PCD_CcNodeModifyMissNextEngine(p_LnxWrpFmDev->h_PcdDev,
                                                    param->id,
                                                    (t_FmPcdCcNextEngineParams*)(&param->cc_next_engine_params));
            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_CC_NODE_REMOVE_KEY:
        {
            ioc_fm_pcd_cc_node_remove_key_params_t            *param;

            param = (ioc_fm_pcd_cc_node_remove_key_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_cc_node_remove_key_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_pcd_cc_node_remove_key_params_t *)arg, sizeof(ioc_fm_pcd_cc_node_remove_key_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            err = FM_PCD_CcNodeRemoveKey(p_LnxWrpFmDev->h_PcdDev, param->id, param->key_indx);
            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_CC_NODE_ADD_KEY:
        {
            ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t            *param;

            param = (ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t *)arg, sizeof(ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            err = FM_PCD_CcNodeAddKey(p_LnxWrpFmDev->h_PcdDev,
                                      param->id,
                                      param->key_indx,
                                      param->key_size,
                                      (t_FmPcdCcKeyParams*)(&param->key_params));
            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_CC_NODE_MODIFY_KEY_AND_NEXT_ENGINE:
        {
            ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t            *param;

            param = (ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t *)arg, sizeof(ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            err = FM_PCD_CcNodeModifyKeyAndNextEngine(p_LnxWrpFmDev->h_PcdDev,
                                                      param->id,
                                                      param->key_indx,
                                                      param->key_size,
                                                      (t_FmPcdCcKeyParams*)(&param->key_params));
            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_CC_NODE_MODIFY_KEY:
        {
            ioc_fm_pcd_cc_node_modify_key_params_t  *param;
            uint8_t                                 *key;
            uint8_t                                 *mask;

            param = (ioc_fm_pcd_cc_node_modify_key_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_cc_node_modify_key_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_pcd_cc_node_modify_key_params_t *)arg, sizeof(ioc_fm_pcd_cc_node_modify_key_params_t))) {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            if (param->p_key) {
                key = (uint8_t *)XX_Malloc(sizeof(uint8_t)*IOC_FM_PCD_MAX_SIZE_OF_KEY);
                if (!key) {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD key"));
                }
                memset(key, 0, sizeof(uint8_t)*IOC_FM_PCD_MAX_SIZE_OF_KEY);

                if (copy_from_user(key, param->p_key, param->key_size)) {
                    XX_Free(key);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
                param->p_key = key;
            }

            if (param->p_mask) {
                mask = (uint8_t *)XX_Malloc(sizeof(uint8_t)*IOC_FM_PCD_MAX_SIZE_OF_KEY);
                if (!mask) {
                    XX_Free(key);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD mask"));
                }
                memset(mask, 0, sizeof(uint8_t)*IOC_FM_PCD_MAX_SIZE_OF_KEY);

                if (copy_from_user(mask, param->p_mask, param->key_size)) {
                    XX_Free(mask);
                    XX_Free(key);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
                param->p_mask = mask;
            }

            err = FM_PCD_CcNodeModifyKey(p_LnxWrpFmDev->h_PcdDev,
                                         param->id,
                                         param->key_indx,
                                         param->key_size,
                                         param->p_key,
                                         param->p_mask);
            XX_Free(mask);
            XX_Free(key);
            XX_Free(param);
            break;
        }

        default:
            RETURN_ERROR(MINOR, E_INVALID_SELECTION, ("IOCTL cmd (0x%08x)!", cmd));
    }

    return err;
}

t_Error LnxwrpFmIOCTL(t_LnxWrpFmDev *p_LnxWrpFmDev, unsigned int cmd, unsigned long arg)
{
    t_Error err = E_READ_FAILED;

    DBG(TRACE, ("p_LnxWrpFmDev - 0x%08x, cmd - 0x%08x, arg - 0x%08x", (uint32_t)p_LnxWrpFmDev, cmd, arg));

    switch (cmd)
    {
        case FM_IOC_SET_PORTS_BANDWIDTH:
        {
            ioc_ports_param_t *param;

            param = (ioc_ports_param_t *)XX_Malloc(sizeof(ioc_ports_param_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_ports_param_t *)arg, sizeof(ioc_ports_param_t)))
            {
                XX_Free(param);
                return err;
            }

            err =  FM_SetPortsBandwidth(p_LnxWrpFmDev->h_Dev,(t_PortsParam*)param);
            XX_Free(param);
            return err;
        }

        case FM_IOC_GET_REVISION:
        {
            ioc_fm_revision_info_t *param;

            param = (ioc_fm_revision_info_t *)XX_Malloc(sizeof(ioc_fm_revision_info_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            FM_GetRevision(p_LnxWrpFmDev->h_Dev, (t_FmRevisionInfo*)param);
            if (copy_to_user((ioc_fm_revision_info_t *)arg, param, sizeof(ioc_fm_revision_info_t)))
                err = E_WRITE_FAILED;
            else
                err = E_OK;

            XX_Free(param);
            return err;
        }

        case FM_IOC_SET_COUNTER:
        {
            ioc_fm_counters_params_t *param;

            param = (ioc_fm_counters_params_t *)XX_Malloc(sizeof(ioc_fm_counters_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_counters_params_t *)arg, sizeof(ioc_fm_counters_params_t)))
            {
                XX_Free(param);
                return err;
            }

            err = FM_SetCounter(p_LnxWrpFmDev->h_Dev, param->cnt, param->val);

            XX_Free(param);
            return err;
        }

        case FM_IOC_GET_COUNTER:
        {
            ioc_fm_counters_params_t *param;

            param = (ioc_fm_counters_params_t *)XX_Malloc(sizeof(ioc_fm_counters_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            if (copy_from_user(param, (ioc_fm_counters_params_t *)arg, sizeof(ioc_fm_counters_params_t)))
            {
                XX_Free(param);
                return err;
            }

            param->val = FM_GetCounter(p_LnxWrpFmDev->h_Dev, param->cnt);
            if (copy_to_user((ioc_fm_counters_params_t *)arg, param, sizeof(ioc_fm_counters_params_t)))
                err = E_WRITE_FAILED;

            XX_Free(param);
            return err;
        }

        case FM_IOC_FORCE_INTR:
        {
            ioc_fm_exceptions param;

            if (get_user(param, (ioc_fm_exceptions*)arg))
                break;
            return FM_ForceIntr(p_LnxWrpFmDev->h_Dev, (e_FmExceptions)param);
        }

        default:
            return LnxwrpFmPcdIOCTL(p_LnxWrpFmDev, cmd, arg);
    }

    RETURN_ERROR(MINOR, E_INVALID_OPERATION, ("IOCTL FM"));
}

t_Error LnxwrpFmPortIOCTL(t_LnxWrpFmPortDev *p_LnxWrpFmPortDev, unsigned int cmd, unsigned long arg)
{
    t_Error err = E_READ_FAILED;
    DBG(TRACE, ("p_LnxWrpFmPortDev - 0x%08x, cmd - 0x%08x, arg - 0x%08x", (uint32_t)p_LnxWrpFmPortDev, cmd, arg));

    switch (cmd)
    {
        case FM_PORT_IOC_DISABLE:
            FM_PORT_Disable(p_LnxWrpFmPortDev->h_Dev);
            return E_OK;
        case FM_PORT_IOC_ENABLE:
            FM_PORT_Enable(p_LnxWrpFmPortDev->h_Dev);
            return E_OK;

        case FM_PORT_IOC_SET_ERRORS_ROUTE:
        {
            int errs;
            if (get_user(errs, (int *)arg))
                break;
            return FM_PORT_SetErrorsRoute(p_LnxWrpFmPortDev->h_Dev, (fmPortFrameErrSelect_t)errs);
        }

        case FM_PORT_IOC_ALLOC_PCD_FQIDS:
        {
            ioc_fm_port_pcd_fqids_params_t        *param;

            param = (ioc_fm_port_pcd_fqids_params_t *)XX_Malloc(sizeof(ioc_fm_port_pcd_fqids_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));

            if (copy_from_user(param, (ioc_fm_port_pcd_fqids_params_t *)arg, sizeof(ioc_fm_port_pcd_fqids_params_t)))
            {
                XX_Free(param);
                return err;
            }

            if (!p_LnxWrpFmPortDev->pcd_owner_params.cb)
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("No one to listen on this PCD!!!"));

            if (p_LnxWrpFmPortDev->pcd_owner_params.cb (p_LnxWrpFmPortDev->pcd_owner_params.dev,
                                                        param->num_fqids,
                                                        param->alignment,
                                                        &param->base_fqid))
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("can't allocate fqids for PCD!!!"));

            if (copy_to_user((ioc_fm_port_pcd_fqids_params_t *)arg, param, sizeof(ioc_fm_port_pcd_fqids_params_t)))
            {
                XX_Free(param);
                RETURN_ERROR(MAJOR, E_WRITE_FAILED, NO_MSG);
            }

            XX_Free(param);
            return E_OK;
        }

        case FM_PORT_IOC_SET_PCD:
        {
            ioc_fm_port_pcd_params_t        *port_pcd_params;
            ioc_fm_port_pcd_prs_params_t    *port_pcd_prs_params;
            ioc_fm_port_pcd_cc_params_t     *port_pcd_cc_params;
            ioc_fm_port_pcd_kg_params_t     *port_pcd_kg_params;
            ioc_fm_port_pcd_plcr_params_t   *port_pcd_plcr_params;
            uint8_t                         copy_fail = 0;

            if ((port_pcd_params = (ioc_fm_port_pcd_params_t *)XX_Malloc(sizeof(ioc_fm_port_pcd_params_t)))) {
                if ((port_pcd_prs_params = (ioc_fm_port_pcd_prs_params_t *)XX_Malloc(sizeof(ioc_fm_port_pcd_prs_params_t)))) {
                    if ((port_pcd_cc_params = (ioc_fm_port_pcd_cc_params_t *)XX_Malloc(sizeof(ioc_fm_port_pcd_cc_params_t)))) {
                        if ((port_pcd_kg_params = (ioc_fm_port_pcd_kg_params_t *)XX_Malloc(sizeof(ioc_fm_port_pcd_kg_params_t)))) {
                            port_pcd_plcr_params = (ioc_fm_port_pcd_plcr_params_t *)XX_Malloc(sizeof(ioc_fm_port_pcd_plcr_params_t));
                            if (!port_pcd_plcr_params) {
                                XX_Free(port_pcd_params);
                                XX_Free(port_pcd_prs_params);
                                XX_Free(port_pcd_cc_params);
                                XX_Free(port_pcd_kg_params);
                                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));
                            }
                        }
                        else {
                            XX_Free(port_pcd_params);
                            XX_Free(port_pcd_prs_params);
                            XX_Free(port_pcd_cc_params);
                            RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));
                        }
                    }
                    else {
                        XX_Free(port_pcd_params);
                        XX_Free(port_pcd_prs_params);
                        RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));
                    }
                }
                else {
                    XX_Free(port_pcd_params);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));
                }
            }
            else
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));

            copy_fail = 0;
            if (copy_from_user(port_pcd_params, (ioc_fm_port_pcd_params_t *)arg, sizeof(ioc_fm_port_pcd_params_t)))
                copy_fail = 1;
            if (port_pcd_params->p_prs_params && !copy_fail)
            {
                if (copy_from_user(port_pcd_prs_params,
                                    port_pcd_params->p_prs_params,
                                    sizeof(ioc_fm_port_pcd_prs_params_t)))
                  copy_fail = 1;
                else
                  port_pcd_params->p_prs_params = port_pcd_prs_params;
            }
            if (port_pcd_params->p_cc_params && !copy_fail)
            {
                if (copy_from_user(port_pcd_cc_params,
                                    port_pcd_params->p_cc_params,
                                    sizeof(ioc_fm_port_pcd_cc_params_t)))
                  copy_fail = 1;
                else
                  port_pcd_params->p_cc_params = port_pcd_cc_params;
            }
            if (port_pcd_params->p_kg_params && !copy_fail)
            {
                if (copy_from_user(port_pcd_kg_params,
                                    port_pcd_params->p_kg_params,
                                    sizeof(ioc_fm_port_pcd_kg_params_t)))
                  copy_fail = 1;
                else
                  port_pcd_params->p_kg_params = port_pcd_kg_params;
            }
            if (port_pcd_params->p_plcr_params && !copy_fail)
            {
                if (copy_from_user(port_pcd_plcr_params,
                                    port_pcd_params->p_plcr_params,
                                    sizeof(ioc_fm_port_pcd_plcr_params_t)))
                  copy_fail = 1;
                else
                  port_pcd_params->p_plcr_params = port_pcd_plcr_params;
            }
            if (!copy_fail)
               err = FM_PORT_SetPCD(p_LnxWrpFmPortDev->h_Dev, (t_FmPortPcdParams *)port_pcd_params);
            else
               err = E_READ_FAILED;

            XX_Free(port_pcd_params);
            XX_Free(port_pcd_prs_params);
            XX_Free(port_pcd_cc_params);
            XX_Free(port_pcd_kg_params);
            XX_Free(port_pcd_plcr_params);
            return err;
        }

        case FM_PORT_IOC_DELETE_PCD:
            return FM_PORT_DeletePCD(p_LnxWrpFmPortDev->h_Dev);

        case FM_PORT_IOC_PCD_KG_MODIFY_INITIAL_SCHEME:
        {
            ioc_fm_pcd_kg_scheme_select_t        *param;

            param = (ioc_fm_pcd_kg_scheme_select_t *)XX_Malloc(sizeof(ioc_fm_pcd_kg_scheme_select_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));

            if (copy_from_user(param, (ioc_fm_pcd_kg_scheme_select_t *)arg, sizeof(ioc_fm_pcd_kg_scheme_select_t)))
            {
                XX_Free(param);
                RETURN_ERROR(MAJOR, E_READ_FAILED, NO_MSG);
            }

            err =  FM_PORT_PcdKgModifyInitialScheme(p_LnxWrpFmPortDev->h_Dev, (t_FmPcdKgSchemeSelect *)param);

            XX_Free(param);
            return err;
        }

        case FM_PORT_IOC_PCD_PLCR_MODIFY_INITIAL_PROFILE:
        {
            ioc_fm_obj_t id;
            if (copy_from_user(&id, (ioc_fm_obj_t *)arg, sizeof(ioc_fm_obj_t)))
                break;
            return FM_PORT_PcdPlcrModifyInitialProfile(p_LnxWrpFmPortDev->h_Dev, id.obj);
        }

        case FM_PORT_IOC_PCD_KG_BIND_SCHEMES:
        {
            ioc_fm_pcd_port_schemes_params_t        *param;

            param = (ioc_fm_pcd_port_schemes_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_port_schemes_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));

            if (copy_from_user(param, (ioc_fm_pcd_port_schemes_params_t *)arg, sizeof(ioc_fm_pcd_port_schemes_params_t)))
            {
                XX_Free(param);
                RETURN_ERROR(MAJOR, E_WRITE_FAILED, NO_MSG);
            }

            err = FM_PORT_PcdKgBindSchemes(p_LnxWrpFmPortDev->h_Dev, (t_FmPcdPortSchemesParams *)param);

            XX_Free(param);
            return err;
        }

        case FM_PORT_IOC_PCD_KG_UNBIND_SCHEMES:
        {
            ioc_fm_pcd_port_schemes_params_t        *param;

            param = (ioc_fm_pcd_port_schemes_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_port_schemes_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));

            if (copy_from_user(param, (ioc_fm_pcd_port_schemes_params_t *)arg, sizeof(ioc_fm_pcd_port_schemes_params_t)))
            {
                XX_Free(param);
                RETURN_ERROR(MAJOR, E_WRITE_FAILED, NO_MSG);
            }

            err =  FM_PORT_PcdKgUnbindSchemes(p_LnxWrpFmPortDev->h_Dev, (t_FmPcdPortSchemesParams *)param);

            XX_Free(param);
            return err;
        }

        case FM_PORT_IOC_PCD_PRS_MODIFY_START_OFFSET:
        {
            ioc_fm_pcd_prs_start_t        *param;

            param = (ioc_fm_pcd_prs_start_t *)XX_Malloc(sizeof(ioc_fm_pcd_prs_start_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));

            if (copy_from_user(param, (ioc_fm_pcd_prs_start_t *)arg, sizeof(ioc_fm_pcd_prs_start_t)))
            {
                XX_Free(param);
                RETURN_ERROR(MAJOR, E_WRITE_FAILED, NO_MSG);
            }
            err = FM_PORT_PcdPrsModifyStartOffset(p_LnxWrpFmPortDev->h_Dev, (t_FmPcdPrsStart *)param);

            XX_Free(param);
            return err;
        }

        case FM_PORT_IOC_PCD_PLCR_ALLOC_PROFILES:
        {
            int num;
            if (get_user(num, (int *)arg))
                break;
            return FM_PORT_PcdPlcrAllocProfiles(p_LnxWrpFmPortDev->h_Dev, (uint16_t)num);
        }
        case FM_PORT_IOC_PCD_PLCR_FREE_PROFILES:
            return FM_PORT_PcdPlcrFreeProfiles(p_LnxWrpFmPortDev->h_Dev);

        case FM_PORT_IOC_DETACH_PCD:
            return FM_PORT_DetachPCD(p_LnxWrpFmPortDev->h_Dev);

        case FM_PORT_IOC_ATTACH_PCD:
            return FM_PORT_AttachPCD(p_LnxWrpFmPortDev->h_Dev);

        case FM_PORT_IOC_PCD_KG_MODIFY_CLS_PLAN_GRP:
        {
            ioc_fm_pcd_port_cls_plan_params_t        *param;

            param = (ioc_fm_pcd_port_cls_plan_params_t *)XX_Malloc(sizeof(ioc_fm_pcd_port_cls_plan_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));

            if (copy_from_user(param, (ioc_fm_pcd_port_cls_plan_params_t *)arg, sizeof(ioc_fm_pcd_port_cls_plan_params_t)))
            {
                XX_Free(param);
                RETURN_ERROR(MAJOR, E_WRITE_FAILED, NO_MSG);
            }
            err = FM_PORT_PcdKgModifyClsPlanGrp (p_LnxWrpFmPortDev->h_Dev,
                                                 param->use_cls_plan,
                                                 param->new_cls_plan_grp);

            XX_Free(param);
            return err;
        }

        case FM_PORT_IOC_PCD_CC_MODIFY_TREE:
        {
            ioc_fm_obj_t id;
            if (copy_from_user(&id, (ioc_fm_obj_t *)arg, sizeof(ioc_fm_obj_t)))
                break;
            return FM_PORT_PcdCcModifyTree(p_LnxWrpFmPortDev->h_Dev, id.obj);
        }

        default:
            RETURN_ERROR(MINOR, E_INVALID_SELECTION, ("IOCTL port cmd!"));
    }

    RETURN_ERROR(MINOR, E_INVALID_OPERATION, ("IOCTL port"));
}
