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

#ifndef __SYS_EXT_H
#define __SYS_EXT_H

#include "std_ext.h"


typedef enum e_SysModule
{
    e_SYS_MODULE_NONE = 0,

    /* ----------- Modules ----------- */
    e_SYS_MODULE_PLATFORM,
    e_SYS_MODULE_DEV_MNG,
    e_SYS_MODULE_DATA_POOL_MNG,
    e_SYS_MODULE_TRAFFIC_MNG,
    e_SYS_MODULE_APP,
    /* Stacks */
    e_SYS_MODULE_PCI_STACK,
    e_SYS_MODULE_ESDHC_STACK,
    e_SYS_MODULE_RIO_STACK,
    e_SYS_MODULE_I2C_STACK,
    e_SYS_MODULE_SPI_STACK,
    e_SYS_MODULE_USB_STACK,
    e_SYS_MODULE_IMA_STACK,
    /* Wrappers */
    e_SYS_MODULE_WRP_ECM,
    e_SYS_MODULE_WRP_ARBITER,
    e_SYS_MODULE_WRP_DUART,
    e_SYS_MODULE_WRP_UART,
    e_SYS_MODULE_WRP_QE,
    e_SYS_MODULE_WRP_UPC,
    e_SYS_MODULE_WRP_UCC_ATM,
    e_SYS_MODULE_WRP_UCC_POS,
    e_SYS_MODULE_WRP_UCC_GETH,
    e_SYS_MODULE_WRP_UCC_HDLC,
    e_SYS_MODULE_WRP_UCC_TRNS,
    e_SYS_MODULE_WRP_UCC_UART,
    e_SYS_MODULE_WRP_QMC,
    e_SYS_MODULE_WRP_QMC_HDLC,
    e_SYS_MODULE_WRP_QMC_TRANS,
    e_SYS_MODULE_WRP_L2_SWITCH,
    e_SYS_MODULE_WRP_SI,
    e_SYS_MODULE_WRP_TDM,
    e_SYS_MODULE_WRP_MCC,
    e_SYS_MODULE_WRP_MCC_HDLC,
    e_SYS_MODULE_WRP_MCC_TRANS,
    e_SYS_MODULE_WRP_MCC_SS7,
    e_SYS_MODULE_WRP_PPPOHT,
    e_SYS_MODULE_WRP_IW_PPPOHT,
    e_SYS_MODULE_WRP_PCI,
    e_SYS_MODULE_WRP_ESDHC,
    e_SYS_MODULE_WRP_RIO,
    e_SYS_MODULE_WRP_I2C,
    e_SYS_MODULE_WRP_SPI,
    e_SYS_MODULE_WRP_ETSEC,
    e_SYS_MODULE_WRP_SRIO,
    e_SYS_MODULE_WRP_IMA,
    e_SYS_MODULE_WRP_MTC,
    e_SYS_MODULE_WRP_QE_IW,
    e_SYS_MODULE_WRP_QE_FILTERING,
    e_SYS_MODULE_WRP_QE_RTC,
    e_SYS_MODULE_WRP_VP,
    e_SYS_MODULE_WRP_GTIMERS,
    e_SYS_MODULE_WRP_DMA,
    e_SYS_MODULE_WRP_PTP,

    /* Bus Device Drivers */
    e_SYS_MODULE_MPC8568_RIO_DEV,
    e_SYS_MODULE_MPC8568_PCI_DEV,
    e_SYS_MODULE_MPC836X_PCI_DEV,
    e_SYS_MODULE_MPC832X_PCI_DEV,
    e_SYS_MODULE_MPC8568_PCIE_DEV,
    e_SYS_MODULE_MPC8569_PCIE_DEV,
    e_SYS_MODULE_MPC8568_SPI_FLASH_DEV,
    e_SYS_MODULE_MPC8568_SPI_LB_DEV,
    e_SYS_MODULE_SD_DEV,
    e_SYS_MODULE_SPD_I2C_DEV,
    e_SYS_MODULE_DS1374_I2C_DEV,
    e_SYS_MODULE_PCA9555_I2C_DEV,
    e_SYS_MODULE_M24256_I2C_DEV,
    e_SYS_MODULE_AT24C01A_I2C_DEV,

    /* Must close the modules list and open the sub-modules list */
    e_SYS_SUBMODULE_DUMMY_FIRST,

    /* --------- Sub-modules --------- */
    e_SYS_SUBMODULE_DATA_POOL,
    e_SYS_SUBMODULE_PG,
    e_SYS_SUBMODULE_TG,
    e_SYS_SUBMODULE_TA,
    e_SYS_SUBMODULE_DEC,
    e_SYS_SUBMODULE_PART,
    e_SYS_SUBMODULE_IPIC,
    e_SYS_SUBMODULE_EPIC,
    e_SYS_SUBMODULE_QE_IC,
    e_SYS_SUBMODULE_LAW,
    e_SYS_SUBMODULE_DDR,
    e_SYS_SUBMODULE_LBC,
    e_SYS_SUBMODULE_ECM,
    e_SYS_SUBMODULE_ARBITER,
    e_SYS_SUBMODULE_L2,
    e_SYS_SUBMODULE_QE,
    e_SYS_SUBMODULE_QE_TIMERS,
    e_SYS_SUBMODULE_GTIMERS,
    e_SYS_SUBMODULE_PAR_IO,
    e_SYS_SUBMODULE_SI,
    e_SYS_SUBMODULE_TDM,
    e_SYS_SUBMODULE_TDM_RX_FRAME,
    e_SYS_SUBMODULE_TDM_TX_FRAME,
    e_SYS_SUBMODULE_MCC,
    e_SYS_SUBMODULE_MCC_HDLC_CH,
    e_SYS_SUBMODULE_MCC_TRANS_CH,
    e_SYS_SUBMODULE_MCC_SS7_CH,
    e_SYS_SUBMODULE_UPC,
    e_SYS_SUBMODULE_DUART,
    e_SYS_SUBMODULE_I2C_CTRL,
    e_SYS_SUBMODULE_DMA,
    e_SYS_SUBMODULE_DMA_CH,
    e_SYS_SUBMODULE_PM,
    e_SYS_SUBMODULE_RTC,
    e_SYS_SUBMODULE_TLU,
    e_SYS_SUBMODULE_SEC,
    e_SYS_SUBMODULE_PCI_CTRL,
    e_SYS_SUBMODULE_PCIE_CTRL,
    e_SYS_SUBMODULE_ESDHC,
    e_SYS_SUBMODULE_SRIO_PORT,
    e_SYS_SUBMODULE_SRIO_MU,
    e_SYS_SUBMODULE_SPI_CTRL,
    e_SYS_SUBMODULE_ETSEC,
    e_SYS_SUBMODULE_USB,
    e_SYS_SUBMODULE_UCC_GETH,
    e_SYS_SUBMODULE_UCC_ATM_COMMON,
    e_SYS_SUBMODULE_UCC_ATM_AAL2_COMMON,
    e_SYS_SUBMODULE_UCC_ATM_CTRL,
    e_SYS_SUBMODULE_UCC_POS_COMMON,
    e_SYS_SUBMODULE_UCC_POS_CTRL,
    e_SYS_SUBMODULE_EOS_LINK,
    e_SYS_SUBMODULE_PPPOS_LINK,
    e_SYS_SUBMODULE_UTOPIA_PORT,
    e_SYS_SUBMODULE_UCC_ATM_POLICER,
    e_SYS_SUBMODULE_UCC_ATM_AAL0,
    e_SYS_SUBMODULE_UCC_ATM_AAL5,
    e_SYS_SUBMODULE_UCC_ATM_AAL1,
    e_SYS_SUBMODULE_UCC_ATM_AAL2,
    e_SYS_SUBMODULE_UCC_ATM_AAL2_TQD,
    e_SYS_SUBMODULE_UCC_ATM_AAL2_CID,
    e_SYS_SUBMODULE_UCC_HDLC,
    e_SYS_SUBMODULE_UCC_TRNS,
    e_SYS_SUBMODULE_UCC_UART,
    e_SYS_SUBMODULE_QMC,
    e_SYS_SUBMODULE_QMC_HDLC_CH,
    e_SYS_SUBMODULE_QMC_TRANS_CH,
    e_SYS_SUBMODULE_L2_SWITCH,
    e_SYS_SUBMODULE_L2_SWITCH_PORT,
    e_SYS_SUBMODULE_PPPOHT,
    e_SYS_SUBMODULE_PPPOHT_BUNDLE,
    e_SYS_SUBMODULE_PPPOHT_LINK,
    e_SYS_SUBMODULE_PPPOHT_RX_FBP,
    e_SYS_SUBMODULE_PPPOHT_TX_FBPS,
    e_SYS_SUBMODULE_IW_PPPOHT,
    e_SYS_SUBMODULE_IW_PPPOHT_BUNDLE,
    e_SYS_SUBMODULE_IW_PPPOHT_LINK,
    e_SYS_SUBMODULE_IMA,
    e_SYS_SUBMODULE_IMA_GROUP_PORT,
    e_SYS_SUBMODULE_IMA_GROUP,
    e_SYS_SUBMODULE_IMA_LINK,
    e_SYS_SUBMODULE_MTC_COMMON,
    e_SYS_SUBMODULE_MTC,
    e_SYS_SUBMODULE_MTC_UCC,
    e_SYS_SUBMODULE_MTC_MCC,
    e_SYS_SUBMODULE_MTC_MCC_COMMON,
    e_SYS_SUBMODULE_IW_COMMON,
    e_SYS_SUBMODULE_IW_DEV,
    e_SYS_SUBMODULE_IW_CONN,
    e_SYS_SUBMODULE_IW_CDESC,
    e_SYS_SUBMODULE_IW_MCAST_GROUP,
    e_SYS_SUBMODULE_IW_IP_REASS,
    e_SYS_SUBMODULE_IW_PLCR,
    e_SYS_SUBMODULE_IW_QM,
    e_SYS_SUBMODULE_QE_FILTER,
    e_SYS_SUBMODULE_QE_TABLE,
    e_SYS_SUBMODULE_QE_RTC,
    e_SYS_SUBMODULE_VP_DEV,
    e_SYS_SUBMODULE_VP_PORT,
    e_SYS_SUBMODULE_MII_MNG,
    e_SYS_SUBMODULE_PTP,
    e_SYS_SUBMODULE_BM,
    e_SYS_SUBMODULE_BM_PORTAL,
    e_SYS_SUBMODULE_BM_CE_PORTAL,
    e_SYS_SUBMODULE_BM_CI_PORTAL,
    e_SYS_SUBMODULE_QM,
    e_SYS_SUBMODULE_QM_PORTAL,
    e_SYS_SUBMODULE_QM_CE_PORTAL,
    e_SYS_SUBMODULE_QM_CI_PORTAL,
    e_SYS_SUBMODULE_FM,
    e_SYS_SUBMODULE_FM_MURAM,
    e_SYS_SUBMODULE_FM_PORT_HO,
    e_SYS_SUBMODULE_FM_PORT_10GRx,
    e_SYS_SUBMODULE_FM_PORT_1GRx,
    e_SYS_SUBMODULE_FM_PORT_10GTx,
    e_SYS_SUBMODULE_FM_PORT_1GTx,
    e_SYS_SUBMODULE_FM_PORT_10GMAC,
    e_SYS_SUBMODULE_FM_PORT_1GMAC,
    e_SYS_SUBMODULE_IW_IPHC_HC,
    e_SYS_SUBMODULE_IW_IPHC_HDEC,
    /* Must close the sub-modules list */
    e_SYS_SUBMODULE_DUMMY_LAST

} e_SysModule;

/**************************************************************************//**
 @Function      SYS_GetHandle

 @Description   Returns a specific object handle.

                This routine may be used to get the handle of any module or
                sub-module in the system.

                For singleton objects, it is recommended to use the
                SYS_GetUniqueHandle() routine.

 @Param[in]     module  - Module/sub-module type.
 @Param[in]     id      - For sub-modules, this is the unique object ID;
                          For modules, this value must always be zero.

 @Return        The handle of the specified object if exists;
                NULL if the object is not known or is not initialized.
*//***************************************************************************/
t_Handle SYS_GetHandle(e_SysModule module, uint32_t id);

/**************************************************************************//**
 @Function      SYS_GetUniqueHandle

 @Description   Returns a specific object handle (for singleton objects).

                This routine may be used to get the handle of any singleton
                module or sub-module in the system.

                This routine simply calls the SYS_GetHandle() routine with
                the \c id parameter set to zero.

 @Param[in]     module - Module/sub-module type.

 @Return        The handle of the specified object if exists;
                NULL if the object is not known or is not initialized.
*//***************************************************************************/
static __inline__ t_Handle SYS_GetUniqueHandle(e_SysModule module)
{
    return SYS_GetHandle(module, 0);
}

/**************************************************************************//**
 @Function      SYS_ForceHandle

 @Description   Forces a handle for a specific object in the system.

                This routine allows forcing an object handle into the system
                and thus bypassing the normal initialization flow.

                The forced handle must be removed as soon as it is not valid
                anymore, using the SYS_RemoveForcedHandle() routine.

 @Param[in]     module      - The object (module/sub-module) type.
 @Param[in]     id          - Unique object ID;
 @Param[in]     h_Module    - The object handle;

 @Return        E_OK on success; Error code otherwise.

 @Cautions      This routine must not be used in normal flow - it serves only
                rare and special cases in platform initialization.
*//***************************************************************************/
t_Error SYS_ForceHandle(e_SysModule module, uint32_t id, t_Handle h_Module);

/**************************************************************************//**
 @Function      SYS_RemoveForcedHandle

 @Description   Removes a previously forced handle of a specific object.

                This routine must be called to remove object handles that
                were previously forced using the SYS_ForceHandle() routine.

 @Param[in]     module      - The object (module/sub-module) type.
 @Param[in]     id          - Unique object ID;

 @Return        None.

 @Cautions      This routine must not be used in normal flow - it serves only
                rare and special cases in platform initialization.
*//***************************************************************************/
void SYS_RemoveForcedHandle(e_SysModule module, uint32_t id);

typedef struct t_SysObjectDescriptor
{
    e_SysModule module;
    uint32_t    id;
    void        *p_Settings;

} t_SysObjectDescriptor;


#define SYS_MAX_ADV_CONFIG_ARGS      4

typedef struct t_SysObjectAdvConfigEntry
{
    void        *p_Function;
    uint32_t    args[SYS_MAX_ADV_CONFIG_ARGS]; //@@@@ uint64_t (CW bug) ?
} t_SysObjectAdvConfigEntry;


#define ADV_CONFIG_DONE         NULL, { 0 }
#define ADV_CONFIG_NONE         (t_SysObjectAdvConfigEntry[]){ ADV_CONFIG_DONE }


#define PARAMS(_num, _params)   ADV_CONFIG_PARAMS_##_num _params
#define NO_PARAMS

#define ADV_CONFIG_PARAMS_1(_type) \
    , (_type)p_Entry->args[0]

#define ADV_CONFIG_PARAMS_2(_type0, _type1) \
    , (_type0)p_Entry->args[0], (_type1)p_Entry->args[1]

#define ADV_CONFIG_PARAMS_3(_type0, _type1, _type2) \
    , (_type0)p_Entry->args[0], (_type1)p_Entry->args[1], (_type2)p_Entry->args[2]

#define ADV_CONFIG_PARAMS_4(_type0, _type1, _type2, _type3) \
    , (_type0)p_Entry->args[0], (_type1)p_Entry->args[1], (_type2)p_Entry->args[2], (_type3)p_Entry->args[3]


#define SET_ADV_CONFIG_ARGS_1(_arg0)        \
    p_Entry->args[0] = (uint32_t)(_arg0);   \

#define SET_ADV_CONFIG_ARGS_2(_arg0, _arg1) \
    p_Entry->args[0] = (uint32_t)(_arg0);   \
    p_Entry->args[1] = (uint32_t)(_arg1);   \

#define SET_ADV_CONFIG_ARGS_3(_arg0, _arg1, _arg2)  \
    p_Entry->args[0] = (uint32_t)(_arg0);           \
    p_Entry->args[1] = (uint32_t)(_arg1);           \
    p_Entry->args[2] = (uint32_t)(_arg2);           \

#define SET_ADV_CONFIG_ARGS_4(_arg0, _arg1, _arg2, _arg3)   \
    p_Entry->args[0] = (uint32_t)(_arg0);                   \
    p_Entry->args[1] = (uint32_t)(_arg1);                   \
    p_Entry->args[2] = (uint32_t)(_arg2);                   \
    p_Entry->args[3] = (uint32_t)(_arg3);                   \

#define ARGS(_num, _params) SET_ADV_CONFIG_ARGS_##_num _params
#define NO_ARGS

#define ADD_ADV_CONFIG(_func, _param)       \
    do {                                    \
        if (i<max){                         \
            p_Entry = &p_Entrys[i];         \
            p_Entry->p_Function = _func;    \
            _param                          \
            i++;                            \
        }                                   \
        else                                \
            REPORT_ERROR(MINOR, E_INVALID_VALUE, ("number of advance-configuration exceeded!!!"));\
    } while (0)

#define ADD_ADV_CONFIG_START(_p_Entries, _maxEntries)                   \
    {                                                                   \
        t_SysObjectAdvConfigEntry   *p_Entry;                           \
        t_SysObjectAdvConfigEntry   *p_Entrys = (_p_Entries);           \
        int                         i=0, max = (_maxEntries);           \
        for (; i<FM_MAX_NUM_OF_ADV_SETTINGS;i++)                        \
            if (!p_LnxWrpFmPortDev->settings.advConfig[i].p_Function)   \
                break;

#define ADD_ADV_CONFIG_END \
    }

#define ADV_CONFIG_CHECK_START(_p_Entry)                        \
    {                                                           \
        t_SysObjectAdvConfigEntry   *p_Entry = _p_Entry;        \
        t_Error                     errCode;                    \

#define ADV_CONFIG_CHECK(_handle, _func, _params)               \
        if (p_Entry->p_Function == _func)                       \
        {                                                       \
            errCode = _func(_handle _params);                   \
        } else

#define ADV_CONFIG_CHECK_END                                    \
        {                                                       \
            REPORT_ERROR(MAJOR, E_INVALID_SELECTION,            \
                         ("Advanced configuration routine"));   \
            return NULL;                                        \
        }                                                       \
        if (errCode != E_OK)                                    \
        {                                                       \
            REPORT_ERROR(MAJOR, errCode, NO_MSG);               \
            return NULL;                                        \
        }                                                       \
    }



#define CAST_ID_TO_HANDLE(_id)      ((t_Handle)(_id))
#define CAST_HANDLE_TO_ID(_h)       ((uint32_t)(_h))


#define SYS_MAX_TEST_GROUP_NAME_LENGTH      14
#define SYS_MAX_TEST_DESCRIPTION_LENGTH     64

typedef struct t_SysTestDescriptor
{
    char        testGroup[SYS_MAX_TEST_GROUP_NAME_LENGTH];
    uint16_t    testId;
    char        description[SYS_MAX_TEST_DESCRIPTION_LENGTH];
    t_Error     (*f_RunTest)(void *p_TestParam);
    void        (*f_KillTest)(void);
    void        *p_TestParam;

} t_SysTestDescriptor;


typedef struct t_SysRuntimeLayout
{
    t_SysObjectDescriptor   **p_LogicalObjects;
    t_SysTestDescriptor     **p_TestDescriptors;

} t_SysRuntimeLayout;

typedef struct t_SysPeripheralLayout
{
    t_SysObjectDescriptor   **p_PeripheralObjects;
    t_SysRuntimeLayout      *p_RuntimeLayouts;

} t_SysPeripheralLayout;

typedef struct t_SysSystemLayout
{
    t_SysObjectDescriptor   **p_SystemObjects;
    t_SysPeripheralLayout   *p_PeripheralLayouts;

} t_SysSystemLayout;

typedef struct t_SysUseCaseLayout
{
    t_SysObjectDescriptor   *p_PlatformObject;
    t_SysSystemLayout       *p_SystemLayouts;

} t_SysUseCaseLayout;



t_SysUseCaseLayout * USER_BuildLayout(int argc, char *argv[]);

void USER_FreeLayout(t_SysUseCaseLayout *p_UseCaseLayout);



#define SYS_SYSTEM_OBJECTS(...)     (t_SysObjectDescriptor*[]){ __VA_ARGS__, NULL }
#define SYS_PERIPHERAL_OBJECTS(...) (t_SysObjectDescriptor*[]){ __VA_ARGS__, NULL }
#define SYS_LOGICAL_OBJECTS(...)    (t_SysObjectDescriptor*[]){ __VA_ARGS__, NULL }
#define SYS_TEST_DESCRIPTORS(...)   (t_SysTestDescriptor*[]){ __VA_ARGS__, NULL }


#define SYS_BEGIN_SYSTEM_LAYOUTS \
        (t_SysSystemLayout[]){
#define SYS_END_SYSTEM_LAYOUTS \
        , { NULL, NULL } }

#define SYS_BEGIN_PERIPHERAL_LAYOUTS \
        (t_SysPeripheralLayout[]){
#define SYS_END_PERIPHERAL_LAYOUTS \
        , { NULL, NULL } }

#define SYS_BEGIN_RUNTIME_LAYOUTS \
        (t_SysRuntimeLayout[]){
#define SYS_END_RUNTIME_LAYOUTS \
        , { NULL, NULL } }



typedef t_Handle (t_SysModuleInitFuncNoParams)(void);
typedef t_Handle (t_SysModuleInitFuncWithParams)(void *p_ModuleParams);

typedef struct t_SysRegistryEntry
{
    e_SysModule     module;
    bool            noInitParams;
    union
    {
        t_SysModuleInitFuncNoParams     *f_InitNoParams;
        t_SysModuleInitFuncWithParams   *f_InitWithParams;
    };
    t_Error         (*f_Free)(t_Handle h_Module);

} t_SysRegistryEntry;


#define SYS_REGISTER_MODULE_VOID_PARAM(mod, initFunc, freeFunc) \
    {(mod), TRUE,  .f_InitNoParams   = (initFunc), (freeFunc) }

#define SYS_REGISTER_MODULE_WITH_PARAM(mod, initFunc, freeFunc) \
    {(mod), FALSE, .f_InitWithParams = (t_SysModuleInitFuncWithParams *)(initFunc), (freeFunc) }

#define SYS_REGISTER_MODULE_DONE \
    {e_SYS_MODULE_NONE, TRUE, NULL, NULL }

typedef struct t_SysSubModuleRegisterParam
{
    e_SysModule owner;
    uint8_t     numOfSubModules;
    e_SysModule *p_SubModules;
    t_Error     (*f_InitSubModule)(t_Handle h_Module, t_SysObjectDescriptor *p_SubModuleDesc);
    t_Error     (*f_FreeSubModule)(t_Handle h_Module, e_SysModule subModule, uint32_t id);
    t_Handle    (*f_GetSubModule)(t_Handle h_Module, e_SysModule subModule, uint32_t id);

} t_SysSubModuleRegisterParam;


typedef struct t_SysSubModuleUnregisterParam
{
    e_SysModule owner;
    uint8_t     numOfSubModules;
    e_SysModule *p_SubModules;

} t_SysSubModuleUnregisterParam;


void SYS_Init(void);
void SYS_Free(void);


#endif /* __SYS_EXT_H */
