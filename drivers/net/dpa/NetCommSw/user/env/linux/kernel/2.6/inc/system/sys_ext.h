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

    /* Must close the sub-modules list */
    e_SYS_SUBMODULE_DUMMY_LAST

} e_SysModule;


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
