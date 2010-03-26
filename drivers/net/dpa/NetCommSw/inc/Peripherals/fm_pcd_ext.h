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
 @File          fm_pcd_ext.h

 @Description   FM PCD ...
*//***************************************************************************/
#ifndef __FM_PCD_EXT
#define __FM_PCD_EXT

#include "std_ext.h"
#include "net_ext.h"
#include "fm_ext.h"
#include "list_ext.h"


/**************************************************************************//**
 @Group         FM_grp Frame Manager API

 @Description   FM API functions, definitions and enums

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Group         FM_PCD_grp FM PCD

 @Description   FM PCD API functions, definitions and enums

                The FM PCD module is responsible for the initialization of all
                global classifying FM modules. This includes the parser general and
                common registers, the key generator global and common registers,
                and the Policer global and common registers.
                In addition, the FM PCD SW module will initialize all required
                key generator schemes, coarse classification flows, and Policer
                profiles. When An FM module is configured to work with one of these
                entities, it will register to it using the FM PORT API. The PCD
                module will manage the PCD resources - i.e. resource management of
                Keygen schemes, etc.

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Collection    General PCD defines
*//***************************************************************************/
typedef uint32_t fmPcdEngines_t; /**< options as defined below: */

#define FM_PCD_NONE                                 0                   /**< No PCD Engine indicated */
#define FM_PCD_PRS                                  0x80000000          /**< Parser indicated */
#define FM_PCD_KG                                   0x40000000          /**< Keygen indicated */
#define FM_PCD_CC                                   0x20000000          /**< Coarse classification indicated */
#define FM_PCD_PLCR                                 0x10000000          /**< Policer indicated */

#define FM_PCD_MAX_NUM_OF_PRIVATE_HDRS              3                   /**< Number of units/headers saved for user */

#define FM_PCD_PRS_NUM_OF_HDRS                      16                  /**< Number of headers supported by HW parser */
#define FM_PCD_MAX_NUM_OF_DISTINCTION_UNITS         (32 - FM_PCD_MAX_NUM_OF_PRIVATE_HDRS)
                                                                        /**< Maximum number of netenv distinction units */
#define MAX_NUM_OF_OPTIONS                          8                   /**< Maximum number of netenv distinction units options */
#define FM_PCD_MAX_NUM_OF_INTERCHANGABLE_HDRS       4                   /**< Maximum number of interchangable headers in a distinction unit */
#define FM_PCD_KG_NUM_OF_GENERIC_REGS               8                   /**< Total number of generic KG registers */
#define FM_PCD_KG_MAX_NUM_OF_EXTRACTS_PER_KEY       35                  /**< Max number allowed on any configuration.
                                                                             For reason of HW implemetation, in most
                                                                             cases less than this will be allowed. The
                                                                             driver will return error in initialization
                                                                             time if resource is overused. */
#define FM_PCD_KG_NUM_OF_EXTRACT_MASKS              4                   /**< Total number of masks allowed on KG extractions. */
#define FM_PCD_KG_NUM_OF_DEFAULT_GROUPS             16                  /**< Number of default value logical groups */

#define FM_PCD_PRS_NUM_OF_LABELS                    32                  /**< Max number of SW parser label */
#define FM_SW_PRS_SIZE                              0x00000800          /**< Total size of sw parser area */
#define PRS_SW_OFFSET                               0x00000040          /**< Size of illegal addresses at the beginning
                                                                             of the SW parser area */
#define PRS_SW_TAIL_SIZE                            4                   /**< Number of bytes that must be cleared at
                                                                             the end of the SW parser area */
#define FM_SW_PRS_MAX_IMAGE_SIZE                    (FM_SW_PRS_SIZE-PRS_SW_OFFSET-PRS_SW_TAIL_SIZE)
                                                                        /**< Max possible size of SW parser code */
/* @} */

/**************************************************************************//**
 @Group         FM_PCD_init_grp FM PCD Initialization Unit

 @Description   FM PCD Initialization Unit

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Description   PCD counters
*//***************************************************************************/
typedef enum e_FmPcdCounters {
    e_FM_PCD_KG_COUNTERS_TOTAL,                                 /**< Policer counter */
    e_FM_PCD_PLCR_COUNTERS_YELLOW,                              /**< Policer counter */
    e_FM_PCD_PLCR_COUNTERS_RED,                                 /**< Policer counter */
    e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_RED,                    /**< Policer counter */
    e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_YELLOW,                 /**< Policer counter */
    e_FM_PCD_PLCR_COUNTERS_TOTAL,                               /**< Policer counter */
    e_FM_PCD_PLCR_COUNTERS_LENGTH_MISMATCH,                     /**< Policer counter */
    e_FM_PCD_PRS_COUNTERS_PARSE_DISPATCH,                       /**< Parser counter */
    e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED,             /**< Parser counter */
    e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED,             /**< Parser counter */
    e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED,             /**< Parser counter */
    e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED,           /**< Parser counter */
    e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED_WITH_ERR,    /**< Parser counter */
    e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED_WITH_ERR,    /**< Parser counter */
    e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED_WITH_ERR,    /**< Parser counter */
    e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED_WITH_ERR,  /**< Parser counter */
    e_FM_PCD_PRS_COUNTERS_SOFT_PRS_CYCLES,                      /**< Parser counter */
    e_FM_PCD_PRS_COUNTERS_SOFT_PRS_STALL_CYCLES,                /**< Parser counter */
    e_FM_PCD_PRS_COUNTERS_HARD_PRS_CYCLE_INCL_STALL_CYCLES,     /**< Parser counter */
    e_FM_PCD_PRS_COUNTERS_MURAM_READ_CYCLES,                    /**< MURAM counter */
    e_FM_PCD_PRS_COUNTERS_MURAM_READ_STALL_CYCLES,              /**< MURAM counter */
    e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_CYCLES,                   /**< MURAM counter */
    e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_STALL_CYCLES,             /**< MURAM counter */
    e_FM_PCD_PRS_COUNTERS_FPM_COMMAND_STALL_CYCLES              /**< FPM counter */
} e_FmPcdCounters;

/**************************************************************************//**
 @Description   PCD interrupts
*//***************************************************************************/
typedef enum e_FmPcdExceptions {

    /*TODO - to understand how it has to be done*/
    /*maybe module of PCD + specific interrupts*/
    e_FM_PCD_KG_ERR_EXCEPTION_DOUBLE_ECC,                   /**< Keygen ECC error */
    e_FM_PCD_PLCR_ERR_EXCEPTION_DOUBLE_ECC,                 /**< Read Buffer ECC error */
    e_FM_PCD_KG_ERR_EXCEPTION_KEYSIZE_OVERFLOW,             /**< Write Buffer ECC error on system side */
    e_FM_PCD_PLCR_ERR_EXCEPTION_INIT_ENTRY_ERROR,           /**< Write Buffer ECC error on FM side */
    e_FM_PCD_PLCR_EXCEPTION_PRAM_SELF_INIT_COMPLETE,        /**< Self init complete */
    e_FM_PCD_PLCR_EXCEPTION_ATOMIC_ACTION_COMPLETE,         /**< Atomic action complete */
    e_FM_PCD_PRS_EXCEPTION_DOUBLE_ECC,                      /**< Parser ECC error */
    e_FM_PCD_PRS_EXCEPTION_SINGLE_ECC,                      /**< Parser single ECC */
    e_FM_PCD_PRS_EXCEPTION_ILLEGAL_ACCESS,                  /**< Parser illegal access */
    e_FM_PCD_PRS_EXCEPTION_PORT_ILLEGAL_ACCESS              /**< Parser port illegal access */
} e_FmPcdExceptions;


/**************************************************************************//**
 @Description   t_FmPcdExceptions - Exceptions user callback routine, will be called upon an
                exception passing the exception identification.

 @Param[in]     h_App      - User's application descriptor.
 @Param[in]     exception  - The exception.
  *//***************************************************************************/
typedef void (t_FmPcdException) ( t_Handle h_App, e_FmPcdExceptions exception);

/**************************************************************************//**
 @Description   t_FmPcdSchemeErrorExceptionsCallback - Exceptions user callback routine,
                will be called upon an exception passing the exception identification.

 @Param[in]     h_App           - User's application descriptor.
 @Param[in]     exception       - The exception.
 @Param[in]     index           - id of the relevant source (may be scheme or profile id).
 *//***************************************************************************/
typedef void (t_FmPcdIdException) ( t_Handle           h_App,
                                    e_FmPcdExceptions  exception,
                                    uint16_t           index);

/**************************************************************************//**
 @Description   t_FmPcdQmEnqueueCB - TBD.

 @Param[in]     h_App           - User's application descriptor.
 @Param[in]     fqid            - TBD.
 @Param[in]     p_Fd            - TBD.

 @Return        E_OK on success; Error code otherwise.
 *//***************************************************************************/
typedef t_Error (t_FmPcdQmEnqueueCB) ( t_Handle h_QmArg, uint32_t fqid, void *p_Fd);

/**************************************************************************//**
 @Description   A structure for Host-Command
                When using Host command for PCD functionalities, a dedicated port
                must be used. If this routine is called for a PCD in a single partition
                environment, or it is the Master partition in a Multi partition
                environment, The port will be initialized by the PCD driver
                initialization routine.
 *//***************************************************************************/
typedef struct t_FmPcdHcParams {
#ifndef CONFIG_GUEST_PARTITION
    uint64_t                portBaseAddr;       /**< Host-Command Port Virtual Address of
                                                     memory mapped registers.*/
    uint8_t                 portId;             /**< Host-Command Port Id (0-6 relative
                                                     to Host-Command/Offline parsing ports) */
    uint32_t                errFqid;            /**< Host-Command Port Error Queue Id. */
    uint32_t                confFqid;           /**< Host-Command Port Confirmation queue Id. */
    uint8_t                 deqSubPortal;       /**< Host-Command Port Subportal for dequeue. */
#endif /* !CONFIG_GUEST_PARTITION */
    uint32_t                enqFqid;            /**< Host-Command enqueue Queue Id. */
    t_FmPcdQmEnqueueCB      *f_QmEnqueueCB;     /**< TBD */
    t_Handle                h_QmArg;            /**< TBD */
} t_FmPcdHcParams;

/**************************************************************************//**
 @Description   The main structure for PCD initialization
 *//***************************************************************************/
typedef struct t_FmPcdParams {
    bool                        prsSupport;             /**< TRUE if Parser will be used for any
                                                             of the FM ports */
    bool                        ccSupport;              /**< TRUE if Coarse Classification will be used for any
                                                             of the FM ports */
    bool                        kgSupport;              /**< TRUE if Keygen will be used for any
                                                             of the FM ports */
    bool                        plcrSupport;            /**< TRUE if Policer will be used for any
                                                             of the FM ports */
    t_Handle                    h_Fm;                   /**< A handle to the FM module */
    t_Handle                    h_FmMuram;              /**< Relevant only if ccEnable is enabled. */

#ifdef CONFIG_MULTI_PARTITION_SUPPORT
    uint8_t                     numOfSchemes;           /**< Number of schemes dedicated to this partition. */
    uint16_t                    numOfClsPlanEntries;    /**< Number of clsPlan entries dedicated to this partition,
                                                             Must be a power of 2. */
    uint8_t                     partitionId;            /**< Guest Partition Id */
#else
    bool                        useHostCommand;
#endif  /* CONFIG_MULTI_PARTITION_SUPPORT */
    t_FmPcdHcParams             hc;                     /**< Host Command parameters */

#ifndef CONFIG_GUEST_PARTITION
    t_FmPcdException            *f_FmPcdException;      /**< Callback routine to be called of PCD exception */
    t_FmPcdIdException          *f_FmPcdIdException;    /**< Callback routine to be used for a single scheme and
                                                             profile exceptions */
    t_Handle                    h_App;                  /**< A handle to an application layer object; This handle will
                                                             be passed by the driver upon calling the above callbacks */
#endif /* !CONFIG_GUEST_PARTITION */
} t_FmPcdParams;


/**************************************************************************//**
 @Function      FM_PCD_Config

 @Description   Basic configuration of the PCD module.
                Creates descriptor for the FM PCD module.

 @Param[in]     p_FmPcdParams    A structure of parameters for the initialization of PCD.

 @Return        A handle to the initialized module.
*//***************************************************************************/
t_Handle FM_PCD_Config(t_FmPcdParams *p_FmPcdParams);

/**************************************************************************//**
 @Function      FM_PCD_Init

 @Description   Initialization of the PCD module.

 @Param[in]     h_FmPcd - FM PCD module descriptor.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
t_Error FM_PCD_Init(t_Handle h_FmPcd);

/**************************************************************************//**
 @Function      FM_PCD_Free

 @Description   Frees all resources that were assigned to FM module.

                Calling this routine invalidates the descriptor.

 @Param[in]     h_FmPcd - FM PCD module descriptor.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
t_Error FM_PCD_Free(t_Handle h_FmPcd);

/**************************************************************************//**
 @Group         FM_PCD_advanced_init_grp    FM PCD Advanced Configuration Unit

 @Description   Configuration functions used to change default values.

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Function      FM_PCD_ConfigPlcrNumOfSharedProfiles

 @Description   Calling this routine changes the internal driver data base
                from its default selection of exceptions enablement.
                [DEFAULT_numOfSharedPlcrProfiles].

 @Param[in]     h_FmPcd                     FM PCD module descriptor.
 @Param[in]     numOfSharedPlcrProfiles     Number of profiles to
                                            be shared between ports on this partition

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
t_Error FM_PCD_ConfigPlcrNumOfSharedProfiles(t_Handle h_FmPcd, uint16_t numOfSharedPlcrProfiles);

/**************************************************************************//**
 @Function      FM_PCD_ConfigException

 @Description   Calling this routine changes the internal driver data base
                from its default selection of exceptions enablement.
                By default all exceptions are enabled.

 @Param[in]     h_FmPcd         FM PCD module descriptor.
 @Param[in]     exception       The exception to be selected.
 @Param[in]     enable          TRUE to enable interrupt, FALSE to mask it.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
t_Error FM_PCD_ConfigException(t_Handle h_FmPcd, e_FmPcdExceptions exception, bool enable);

/**************************************************************************//**
 @Function      FM_PCD_ConfigPlcrAutoRefreshMode

 @Description   Calling this routine changes the internal driver data base
                from its default selection of exceptions enablement.
                By default autorefresh is enabled.

 @Param[in]     h_FmPcd         FM PCD module descriptor.
 @Param[in]     enable          TRUE to enable, FALSE to disable

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
t_Error FM_PCD_ConfigPlcrAutoRefreshMode(t_Handle h_FmPcd, bool enable);

/**************************************************************************//**
 @Function      FM_PCD_ConfigPrsMaxCycleLimit

 @Description   Calling this routine changes the internal data structure for
                the maximum parsing time from its default value
                [DEFAULT_prsMaxParseCycleLimit].

 @Param[in]     h_FmPcd         FM PCD module descriptor.
 @Param[in]     value           0 to disable the mechanism, or new
                                maximum parsing time.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
t_Error FM_PCD_ConfigPrsMaxCycleLimit(t_Handle h_FmPcd,uint16_t value);

/** @} */ /* end of FM_PCD_advanced_init_grp group */
/** @} */ /* end of FM_PCD_init_grp group */


/**************************************************************************//**
 @Group         FM_PCD_Runtime_grp FM PCD Runtime Unit

 @Description   FM PCD Runtime Unit

                The runtime control allows creation of PCD infrastructure modules
                such as Network Environment Characteristics, Classification Plan
                Groups and Coarse Classification Trees.
                It also allows on-the-fly initialization, modification and removal
                of PCD modules such as Keygen schemes, coarse classification nodes
                and Policer profiles.


                In order to explain the programming model of the PCD driver interface
                a few terms should be explained, and will be used below.
                  * Distinction Header - One of the 16 protocols supported by the FM parser,
                    or one of the shim headers (1-3). May be a header with a special
                    option (see below).
                  * Interchangeable Headers Group- This is a group of Headers recognized
                    by either one of them. For example, if in a specific context the user
                    chooses to treat IPv4 and IPV6 in the same way, they may create an
                    Interchangable Headers Unit consisting of these 2 headers.
                  * A Distinction Unit - a Distinction Header or an Interchangeable Headers
                    Group.
                  * Header with special option - applies to ethernet, mpls, vlan, ipv4 and
                    ipv6, includes multicast, broadcast and other protocol specific options.
                    In terms of hardware it relates to the options available in the classification
                    plan.
                  * Network Environment Characteristics - a set of Distinction Units that define
                    the total recognizable header selection for a certain environment. This is
                    NOT the list of all headers that will ever appear in a flow, but rather
                    everything that needs distinction in a flow, where distinction is made by keygen
                    schemes and coarse classification action descriptors.

                The PCD runtime modules initialization is done in stages. The first stage after
                initializing the PCD module itself is to establish a Network Flows Environment
                Definition. The application may choose to establish one or more such environments.
                Later, when needed, the application will have to state, for some of its modules,
                to which single environment it belongs.

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Description   A structure for sw parser labels
 *//***************************************************************************/
typedef struct t_FmPcdPrsLabelParams {
    uint32_t                instructionOffset;              /**< SW parser label instruction offset (2 bytes
                                                                 resolution), relative to Parser RAM. */
    e_NetHeaderType         hdr;                            /**< The existance of this header will envoke
                                                                 the sw parser code. */
    uint8_t                 indexPerHdr;                    /**< Normally 0, if more than one sw parser
                                                                 attachments for the same header, use this
                                                                 index to distinguish between them. */
} t_FmPcdPrsLabelParams;

/**************************************************************************//**
 @Description   A structure for sw parser
 *//***************************************************************************/
typedef struct t_FmPcdPrsSwParams {
    bool                    override;                   /**< FALSE to invoke a check that nothing else
                                                             was loaded to this address, including
                                                             internal patched.
                                                             TRUE to override any existing code.*/
    uint32_t                size;                       /**< SW parser code size */
    uint16_t                base;                       /**< SW parser base (in instruction counts!
                                                             muat be larger than 0x20)*/
    uint8_t                 *p_Code;                    /**< SW parser code */
    uint32_t                swPrsDataParams[FM_PCD_PRS_NUM_OF_HDRS];
                                                        /**< SW parser data (parameters) */
    uint8_t                 numOfLabels;                /**< Number of labels for SW parser. */
    t_FmPcdPrsLabelParams   labelsTable[FM_PCD_PRS_NUM_OF_LABELS];
                                                        /**< SW parser labels table, containing n
                                                             umOfLabels entries */
} t_FmPcdPrsSwParams;

/**************************************************************************//**
 @Function      FM_PCD_Enable

 @Description   This routine should be called after PCD is initialized for enabling all
                PCD engines according to their existing configuration.

 @Param[in]     h_FmPcd         FM PCD module descriptor.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following PCD_Init() and when PCD is disabled.
*//***************************************************************************/
t_Error FM_PCD_Enable(t_Handle h_FmPcd);

/**************************************************************************//**
 @Function      FM_PCD_Disable

 @Description   This routine may be called when PCD is enabled in order to
                disable all PCD engines. It may be called
                only when none of the ports in the system are using the PCD.

 @Param[in]     h_FmPcd         FM PCD module descriptor.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following PCD_Init() and when PCD is enabled.
*//***************************************************************************/
t_Error FM_PCD_Disable(t_Handle h_FmPcd);

/**************************************************************************//**
 @Function      FM_PCD_KgSetEmptyClsPlanGrp

 @Description   This routine may always be called, and MUST be called when
                not all ports in the partition are actively using the classification
                plan mechanism.
                When called, the driver automatically saves 8 classification
                plans for ports that do NOT use the classification plan mechanism, to
                avoid this (in order to save those entries) this routine may
                be ommited when all ports are using the classification
                plan machanism.

 @Param[in]     h_FmPcd         FM PCD module descriptor.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following PCD_Init().
 *//***************************************************************************/
t_Error FM_PCD_KgSetEmptyClsPlanGrp(t_Handle h_FmPcd);

/**************************************************************************//**
 @Function      FM_PCD_KgDeleteEmptyClsPlanGrp

 @Description   This routine may be called only when all ports in the
                system are actively using the classification plan scheme.
                In such cases, if empty clsPlan was already set,
                it is recommended in order to save resources.

 @Param[in]     h_FmPcd         FM PCD module descriptor.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following PCD_Init().
                Note that this routine may not be called if any of the FM ports
                is not using the classification plan mechanism.
*//***************************************************************************/
t_Error FM_PCD_KgDeleteEmptyClsPlanGrp(t_Handle h_FmPcd);

/**************************************************************************//**
 @Function      FM_PCD_GetCounter

 @Description   Reads one of the FM PCD counters.

 @Param[in]     h_FmPcd     FM PCD module descriptor.
 @Param[in]     counter     The requested counter.

 @Return        Counter's current value.

 @Cautions      Allowed only following FM_PCD_Init().
                Note that it is user's responsibilty to call this routine only
                for enabled counters, and there will be no indication if a
                disabled counter is accessed.
*//***************************************************************************/
uint32_t FM_PCD_GetCounter(t_Handle h_FmPcd, e_FmPcdCounters counter);

#ifndef CONFIG_GUEST_PARTITION
/**************************************************************************//**
@Function      FM_PCD_PrsLoadSw

@Description   This routine may be called in order to load software parsing code.


@Param[in]     h_FmPcd         FM PCD module descriptor.
@Param[in]     p_SwPrs         A pointer to a structure of software
                               parser parameters, including the software
                               parser image.

@Return        E_OK on success; Error code otherwise.

@Cautions      Allowed only following PCD_Init() and when PCD is disabled.
*//***************************************************************************/
t_Error FM_PCD_PrsLoadSw(t_Handle h_FmPcd, t_FmPcdPrsSwParams *p_SwPrs);

/**************************************************************************//**
 @Function      FM_PCD_KgSetDfltValue

 @Description   Calling this routine sets a global default value to be used
                by the keygen when parser does not recognize a required
                field/header.
                By default default values are 0.

 @Param[in]     h_FmPcd         FM PCD module descriptor.
 @Param[in]     valueId         0,1 - one of 2 global default values.
 @Param[in]     value           The requested default value.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following PCD_Init() and when PCD is disabled.
*//***************************************************************************/
t_Error FM_PCD_KgSetDfltValue(t_Handle h_FmPcd, uint8_t valueId, uint32_t value);

/**************************************************************************//**
 @Function      FM_PCD_ConfigKgAdditionalDataAfterParsing

 @Description   Calling this routine allows the keygen to access data past
                the parser finidhing point.

 @Param[in]     h_FmPcd         FM PCD module descriptor.
 @Param[in]     payloadOffset   the number of bytes beyond the parser location.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following PCD_Init() and when PCD is disabled.

*//***************************************************************************/
t_Error FM_PCD_KgSetAdditionalDataAfterParsing(t_Handle h_FmPcd, uint8_t payloadOffset);

/**************************************************************************//**
 @Function      FM_PCD_SetException

 @Description   Calling this routine enables/disables PCD interrupts.

 @Param[in]     h_FmPcd         FM PCD module descriptor.
 @Param[in]     exception       The exception to be selected.
 @Param[in]     enable          TRUE to enable interrupt, FALSE to mask it.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following PCD_Init().
*//***************************************************************************/
t_Error FM_PCD_SetException(t_Handle h_FmPcd, e_FmPcdExceptions exception, bool enable);

/**************************************************************************//**
 @Function      FM_PCD_SetCounter

 @Description   Sets a value to an enabled counter. Use "0" to reset the counter.

 @Param[in]     h_FmPcd     FM PCD module descriptor.
 @Param[in]     counter     The requested counter.
 @Param[in]     value       The requested value to be written into the counter.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following PCD_Init().
*//***************************************************************************/
t_Error FM_PCD_SetCounter(t_Handle h_FmPcd, e_FmPcdCounters counter, uint32_t value);

/**************************************************************************//**
 @Function      FM_PCD_SetPlcrStatistics

 @Description   This routine may be used to enable/disable policer statistics
                counter. By default the statistics is enabled.

 @Param[in]     h_FmPcd         FM PCD module descriptor
 @Param[in]     enable          TRUE to enable, FALSE to disable.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
t_Error FM_PCD_SetPlcrStatistics(t_Handle h_FmPcd, bool enable);

/**************************************************************************//**
 @Function      FM_PCD_PrsStatistics

 @Description   Defines whether to gather parser statistics including all ports.

 @Param[in]     h_FmPcd     FM PCD module descriptor.
 @Param[in]     enable      TRUE to enable, FALSE to disable.

 @Return        None

 @Cautions      Allowed only following PCD_Init().
*//***************************************************************************/
void FM_PCD_PrsStatistics(t_Handle h_FmPcd, bool enable);
#endif /* !CONFIG_GUEST_PARTITION */

/**************************************************************************//**
 @Function      FM_PCD_ForceIntr

 @Description   Causes an interrupt event on the requested source.

 @Param[in]     h_FmPcd     FM PCD module descriptor.
 @Param[in]     exception       An exception to be forced.

 @Return        E_OK on success; Error code if the exception is not enabled,
                or is not able to create interrupt.

 @Cautions      Allowed only following PCD_Init().
*//***************************************************************************/
t_Error FM_PCD_ForceIntr (t_Handle h_FmPcd, e_FmPcdExceptions exception);

/**************************************************************************//**
 @Function      FM_PCD_HcTxConf

 @Description   TBD

 @Param[in]     h_FmPcd         A handle to an FM PCD Module.
 @Param[in]     p_Fd            TBD

 @Cautions      Allowed only following FM_PCD_Init(). Allowed only if 'useHostCommand'
                option was selected in the initialization.
*//***************************************************************************/
void FM_PCD_HcTxConf(t_Handle h_FmPcd, t_FmFD *p_Fd);

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
/**************************************************************************//**
 @Function      FM_PCD_DumpRegs

 @Description   Dumps all PCD registers

 @Param[in]     h_FmPcd         A handle to an FM PCD Module.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
t_Error FM_PCD_DumpRegs(t_Handle h_FmPcd);

/**************************************************************************//**
 @Function      FM_PCD_KgDumpRegs

 @Description   Dumps all PCD KG registers

 @Param[in]     h_FmPcd         A handle to an FM PCD Module.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
t_Error FM_PCD_KgDumpRegs(t_Handle h_FmPcd);

/**************************************************************************//**
 @Function      FM_PCD_PlcrDumpRegs

 @Description   Dumps all PCD Plcr registers

 @Param[in]     h_FmPcd         A handle to an FM PCD Module.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
t_Error FM_PCD_PlcrDumpRegs(t_Handle h_FmPcd);

/**************************************************************************//**
 @Function      FM_PCD_PrsDumpRegs

 @Description   Dumps all PCD Prs registers

 @Param[in]     h_FmPcd         A handle to an FM PCD Module.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
t_Error FM_PCD_PrsDumpRegs(t_Handle h_FmPcd);
#endif /* (defined(DEBUG_ERRORS) && ... */

#ifdef VERIFICATION_SUPPORT
/********************* VERIFICATION ONLY ********************************/
/**************************************************************************//**
 @Function      FM_PCD_BackdoorSet

 @Description   Reads the DMA current status

 @Param[in]     h_FmPcd             A handle to an FM Module.
 @Param[out]    moduleId            Selected block.
 @Param[out]    offset              Register offset withing the block.
 @Param[out]    value               Value to write.


 @Return        None

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
void FM_PCD_BackdoorSet (t_Handle h_FmPcd, e_ModuleId moduleId, uint32_t offset, uint32_t value);

/**************************************************************************//**
 @Function      FM_PCD_BackdoorGet

 @Description   Reads the DMA current status

 @Param[in]     h_FmPcd             A handle to an FM Module.
 @Param[out]    moduleId            Selected block.
 @Param[out]    offset              Register offset withing the block.

 @Return        Value read

 @Cautions      Allowed only following FM_Init().
*//***************************************************************************/
uint32_t     FM_PCD_BackdoorGet(t_Handle h_FmPcd, e_ModuleId moduleId, uint32_t offset);
#endif /*VERIFICATION_SUPPORT*/


/**************************************************************************//**
 @Group         FM_PCD_Runtime_tree_buildgrp FM PCD Tree building Unit

 @Description   FM PCD Runtime Unit

                This group contains routines for setting, deleting and modifying
                PCD resources, for defining the total PCD tree.
 @{
*//***************************************************************************/

/**************************************************************************//**
 @Collection    Definitions of coarse classification
                parameters as required by keygen (when coarse classification
                is the next engine after this scheme).
*//***************************************************************************/
#define         MAX_NUM_OF_PCD_CC_NODES     255
#define         MAX_NUM_OF_PCD_CC_TREES     8
#define         MAX_NUM_OF_PCD_CC_GROUPS    16
#define         MAX_NUM_OF_CC_UNITS         4
#define         MAX_NUM_OF_KEYS             256
#define         MAX_SIZE_OF_KEY             56
/* @} */

/**************************************************************************//**
 @Collection    A set of definitions to allow protocol
                special option description.
*//***************************************************************************/
typedef uint32_t        protocolOpt_t;          /**< A general type to define a protocol option. */

typedef protocolOpt_t   ethProtocolOpt_t;       /**< Ethernet protocol options. */
#define ETH_BROADCAST               0x80000000  /**< Ethernet Broadcast. */
#define ETH_MULTICAST               0x40000000  /**< Ethernet Multicast. */

typedef protocolOpt_t   vlanProtocolOpt_t;      /**< Vlan protocol options. */
#define VLAN_STACKED                0x20000000  /**< Vlan Stacked. */

typedef protocolOpt_t   mplsProtocolOpt_t;      /**< MPLS protocol options. */
#define MPLS_STACKED                0x10000000  /**< MPLS Stacked. */

typedef protocolOpt_t   ipv4ProtocolOpt_t;      /**< IPv4 protocol options. */
#define IPV4_BROADCAST_1            0x08000000  /**< IPv4 Broadcast. */
#define IPV4_MULTICAST_1            0x04000000  /**< IPv4 Multicast. */
#define IPV4_UNICAST_2              0x02000000  /**< Tunneled IPv4 - Unicast. */
#define IPV4_MULTICAST_BROADCAST_2  0x01000000  /**< Tunneled IPv4 - Broadcast/Multicast. */

typedef protocolOpt_t   ipv6ProtocolOpt_t;      /**< IPv6 protocol options. */
#define IPV6_MULTICAST_1            0x00800000  /**< IPv6 Multicast. */
#define IPV6_UNICAST_2              0x00400000  /**< Tunneled IPv6 - Unicast. */
#define IPV6_MULTICAST_2            0x00200000  /**< Tunneled IPv6 - Multicast. */
/* @} */

/**************************************************************************//**
 @Description   A type used for returning the order of the key extraction.
                each value in this array represents the index of the extraction
                command as defined by the user in the initialization extraction array.
                The valid size of this array is the user define number of extractions
                required (also marked by the second '0' in this array).
*//***************************************************************************/
typedef    uint8_t    t_FmPcdKgKeyOrder [FM_PCD_KG_MAX_NUM_OF_EXTRACTS_PER_KEY];

/**************************************************************************//**
 @Description   All PCD engines
*//***************************************************************************/
typedef enum e_FmPcdEngine {
    e_FM_PCD_DONE,      /**< No PCD Engine indicated */
    e_FM_PCD_KG,        /**< Keygen indicated */
    e_FM_PCD_CC,        /**< Coarse classification indicated */
    e_FM_PCD_PLCR,      /**< Policer indicated */
    e_FM_PCD_PRS        /**< Parser indicated */
} e_FmPcdEngine;

/**************************************************************************//**
 @Description   An enum for selecting extraction by header types
*//***************************************************************************/
typedef enum e_FmPcdExtractByHdrType {
    e_FM_PCD_EXTRACT_FROM_HDR,      /**< Extract bytes from header */
    e_FM_PCD_EXTRACT_FROM_FIELD,    /**< Extract bytes from header field */
    e_FM_PCD_EXTRACT_FULL_FIELD     /**< Extract a full field */
} e_FmPcdExtractByHdrType;

/**************************************************************************//**
 @Description   An enum for selecting extraction source
                (when it is not the header)
*//***************************************************************************/
typedef enum e_FmPcdExtractFrom {
    e_FM_PCD_EXTRACT_FROM_FRAME_START,          /**< Extract from beginning of frame */
    e_FM_PCD_KG_EXTRACT_FROM_DFLT_VALUE,        /**< Extract from a default value */
    e_FM_PCD_KG_EXTRACT_FROM_PARSE_RESULT,      /**< Extract from the parser result */
    e_FM_PCD_KG_EXTRACT_FROM_CURR_END_OF_PARSE  /**< Extract from the point where parsing had finished */
} e_FmPcdExtractFrom;

/**************************************************************************//**
 @Description   An enum for selecting extraction type
*//***************************************************************************/
typedef enum e_FmPcdExtractType {
    e_FM_PCD_EXTRACT_BY_HDR,                /**< Extract according to header */
    e_FM_PCD_EXTRACT_NON_HDR,               /**< Extract from data that is not the header */
    e_FM_PCD_KG_EXTRACT_PORT_PRIVATE_INFO   /**< Extract private info as specified by user */
} e_FmPcdExtractType;

/**************************************************************************//**
 @Description   An enum for selecting a default
*//***************************************************************************/
typedef enum e_FmPcdKgExtractDfltSelect {
    e_FM_PCD_KG_DFLT_GBL_0,          /**< Default selection is KG register 0 */
    e_FM_PCD_KG_DFLT_GBL_1,          /**< Default selection is KG register 1 */
    e_FM_PCD_KG_DFLT_PRIVATE_0,      /**< Default selection is a per scheme register 0 */
    e_FM_PCD_KG_DFLT_PRIVATE_1,      /**< Default selection is a per scheme register 1 */
    e_FM_PCD_KG_DFLT_ILLEGAL         /**< Illegal selection */
} e_FmPcdKgExtractDfltSelect;

/**************************************************************************//**
 @Description   An enum defining all default groups -
                each group shares a default value, one of 4 user
                initialized values.
*//***************************************************************************/
typedef enum e_FmPcdKgKnownFieldsDfltTypes {
    e_FM_PCD_KG_MAC_ADDR,               /**< MAC Address */
    e_FM_PCD_KG_TCI,                    /**< TCI field */
    e_FM_PCD_KG_ENET_TYPE,              /**< ENET Type */
    e_FM_PCD_KG_PPP_SESSION_ID,         /**< PPP Session id */
    e_FM_PCD_KG_PPP_PROTOCOL_ID,        /**< PPP Protocol id */
    e_FM_PCD_KG_MPLS_LABEL,             /**< MPLS label */
    e_FM_PCD_KG_IP_ADDR,                /**< IP addr */
    e_FM_PCD_KG_PROTOCOL_TYPE,          /**< Protocol type */
    e_FM_PCD_KG_IP_TOS_TC,              /**< TOS or TC */
    e_FM_PCD_KG_IPV6_FLOW_LABEL,        /**< IPV6 flow label */
    e_FM_PCD_KG_IPSEC_SPI,              /**< IPSEC SPI */
    e_FM_PCD_KG_L4_PORT,                /**< L4 Port */
    e_FM_PCD_KG_TCP_FLAG,               /**< TCP Flag */
    e_FM_PCD_KG_GENERIC_FROM_DATA,      /**< grouping implemented by sw,
                                             any data extraction that is not the full
                                             field described above  */
    e_FM_PCD_KG_GENERIC_FROM_DATA_NO_V, /**< grouping implemented by sw,
                                             any data extraction without validation */
    e_FM_PCD_KG_GENERIC_NOT_FROM_DATA   /**< grouping implemented by sw,
                                             extraction from parser result or
                                             direct use of default value  */
} e_FmPcdKgKnownFieldsDfltTypes;

/**************************************************************************//**
 @Description   enum for defining header index when headers may repeat
*//***************************************************************************/
typedef enum e_FmPcdHdrIndex {
    e_FM_PCD_HDR_INDEX_NONE     =   0,      /**< used when multiple headers not used, also
                                                 to specify regular IP (not tunneled). */
    e_FM_PCD_HDR_INDEX_1,                   /**< may be used for VLAN, MPLS, tunneled IP */
    e_FM_PCD_HDR_INDEX_2,                   /**< may be used for MPLS, tunneled IP */
    e_FM_PCD_HDR_INDEX_3,                   /**< may be used for MPLS */
    e_FM_PCD_HDR_INDEX_LAST     =   0xFF    /**< may be used for VLAN, MPLS */
} e_FmPcdHdrIndex;

/**************************************************************************//**
 @Description   A structure for selcting the policer profile functional type
*//***************************************************************************/
typedef enum e_FmPcdProfileTypeSelection {
    e_FM_PCD_PLCR_PORT_PRIVATE,             /**< Port dedicated profile */
    e_FM_PCD_PLCR_SHARED                    /**< Shared profile (shared within partition) */
} e_FmPcdProfileTypeSelection;

/**************************************************************************//**
 @Description   A structure for selcting the policer profile algorithem
*//***************************************************************************/
typedef enum e_FmPcdPlcrAlgorithmSelection {
    e_FM_PCD_PLCR_PASS_THROUGH,         /**< Policer pass through */
    e_FM_PCD_PLCR_RFC_2698,             /**< Policer algorythm RFC 2698 */
    e_FM_PCD_PLCR_RFC_4115              /**< Policer algorythm RFC 4115 */
} e_FmPcdPlcrAlgorithmSelection;

/**************************************************************************//**
 @Description   A structure for selcting the policer profile color mode
*//***************************************************************************/
typedef enum e_FmPcdPlcrColorMode {
    e_FM_PCD_PLCR_COLOR_BLIND,          /**< Color blind */
    e_FM_PCD_PLCR_COLOR_AWARE           /**< Color aware */
} e_FmPcdPlcrColorMode;

/**************************************************************************//**
 @Description   A structure for selcting the policer profile color functional mode
*//***************************************************************************/
typedef enum e_FmPcdPlcrColor {
    e_FM_PCD_PLCR_GREEN,                /**< Green */
    e_FM_PCD_PLCR_YELLOW,               /**< Yellow */
    e_FM_PCD_PLCR_RED,                  /**< Red */
    e_FM_PCD_PLCR_OVERRIDE              /**< Color override */
} e_FmPcdPlcrColor;

/**************************************************************************//**
 @Description   A structure for selcting the policer profile packet frame length selector
*//***************************************************************************/
typedef enum e_FmPcdPlcrFrameLengthSelect {
  e_FM_PCD_PLCR_L2_FRM_LEN,             /**< L2 frame length */
  e_FM_PCD_PLCR_L3_FRM_LEN,             /**< L3 frame length */
  e_FM_PCD_PLCR_L4_FRM_LEN,             /**< L4 frame length */
  e_FM_PCD_PLCR_FULL_FRM_LEN            /**< Full frame length */
} e_FmPcdPlcrFrameLengthSelect;

/**************************************************************************//**
 @Description   An enum for selecting rollback frame
*//***************************************************************************/
typedef enum e_FmPcdPlcrRollBackFrameSelect {
  e_FM_PCD_PLCR_ROLLBACK_L2_FRM_LEN,    /**< Rollback L2 frame length */
  e_FM_PCD_PLCR_ROLLBACK_FULL_FRM_LEN   /**< Rollback Full frame length */
} e_FmPcdPlcrRollBackFrameSelect;

/**************************************************************************//**
 @Description   A structure for selcting the policer profile packet or byte mode
*//***************************************************************************/
typedef enum e_FmPcdPlcrRateMode {
    e_FM_PCD_PLCR_BYTE_MODE,            /**< Byte mode */
    e_FM_PCD_PLCR_PACKET_MODE           /**< Packet mode */
} e_FmPcdPlcrRateMode;

/**************************************************************************//**
 @Description   An enum for defining action of frame
*//***************************************************************************/
typedef enum e_FmPcdPlcrDoneAction {
    e_FM_PCD_PLCR_ENQ_FRAME,            /**< Enqueue frame */
    e_FM_PCD_PLCR_DROP_FRAME            /**< Drop frame */
} e_FmPcdPlcrDoneAction;

/**************************************************************************//**
 @Description   A structure for selcting the policer counter
*//***************************************************************************/
typedef enum e_FmPcdPlcrProfileCounters {
    e_FM_PCD_PLCR_PROFILE_GREEN_PACKET_TOTAL_COUNTER,               /**< Green packets counter */
    e_FM_PCD_PLCR_PROFILE_YELLOW_PACKET_TOTAL_COUNTER,              /**< Yellow packets counter */
    e_FM_PCD_PLCR_PROFILE_RED_PACKET_TOTAL_COUNTER,                 /**< Red packets counter */
    e_FM_PCD_PLCR_PROFILE_RECOLOURED_YELLOW_PACKET_TOTAL_COUNTER,   /**< Recolored yellow packets counter */
    e_FM_PCD_PLCR_PROFILE_RECOLOURED_RED_PACKET_TOTAL_COUNTER       /**< Recolored red packets counter */
} e_FmPcdPlcrProfileCounters;


/**************************************************************************//**
 @Description   A Union of protocol dependent special options
*//***************************************************************************/
typedef union u_FmPcdHdrProtocolOpt {
    ethProtocolOpt_t    ethOpt;     /**< Ethernet options */
    vlanProtocolOpt_t   vlanOpt;    /**< Vlan options */
    mplsProtocolOpt_t   mplsOpt;    /**< MPLS options */
    ipv4ProtocolOpt_t   ipv4Opt;    /**< IPv4 options */
    ipv6ProtocolOpt_t   ipv6Opt;    /**< IPv6 options */
} u_FmPcdHdrProtocolOpt;

/**************************************************************************//**
 @Description   A union holding all known protocol fields
*//***************************************************************************/
typedef union t_FmPcdFields {
    headerFieldEth_t        eth;        /**< eth      */
    headerFieldVlan_t       vlan;       /**< vlan     */
    headerFieldLlcSnap_t    llcSnap;    /**< llcSnap  */
    headerFieldPppoe_t      pppoe;      /**< pppoe    */
    headerFieldMpls_t       mpls;       /**< mpls     */
    headerFieldIpv4_t       ipv4;       /**< ipv4     */
    headerFieldIpv6_t       ipv6;       /**< ipv6     */
    headerFieldUdp_t        udp;        /**< udp      */
    headerFieldTcp_t        tcp;        /**< tcp      */
    headerFieldSctp_t       sctp;       /**< sctp     */
    headerFieldDccp_t       dccp;       /**< dccp     */
    headerFieldGre_t        gre;        /**< gre      */
    headerFieldMinencap_t   minencap;   /**< minencap */
    headerFieldIpsecAh_t    ipsecAh;    /**< ipsecAh  */
    headerFieldIpsecEsp_t   ipsecEsp;   /**< ipsecEsp */
} t_FmPcdFields;

/**************************************************************************//**
 @Description   structure for defining header extraction for key generation
*//***************************************************************************/
typedef struct t_FmPcdFromHdr {
    uint8_t             size;           /**< Size in byte */
    uint8_t             offset;         /**< Byte offset */
} t_FmPcdFromHdr;

/**************************************************************************//**
 @Description   structure for defining field extraction for key generation
*//***************************************************************************/
typedef struct t_FmPcdFromField {
    t_FmPcdFields       field;          /**< Field selection */
    uint8_t             size;           /**< Size in byte */
    uint8_t             offset;         /**< Byte offset */
} t_FmPcdFromField;

/**************************************************************************//**
 @Description   A structure of parameters used to define a single network
                environment unit.
                A unit should be defined if it will later be used by one or
                more PCD engines to distinguich between flows.
*//***************************************************************************/
typedef struct t_FmPcdDistinctionUnit {
    struct {
        e_NetHeaderType         hdr;        /**< One of the headers supported by the FM */
        u_FmPcdHdrProtocolOpt   opt;        /**< only one option !! */
    } hdrs[FM_PCD_MAX_NUM_OF_INTERCHANGABLE_HDRS];
} t_FmPcdDistinctionUnit;

/**************************************************************************//**
 @Description   A structure of parameters used to define the different
                units supported by a specific PCD Network Environment
                Characteristics module. Each unit represent
                a protocol or a group of protocols that may be used later
                by the different PCD engined to distinguich between flows.
*//***************************************************************************/
typedef struct t_FmPcdNetEnvParams {
    uint8_t                 numOfDistinctionUnits;                      /**< Number of different units to be identified */
    t_FmPcdDistinctionUnit  units[FM_PCD_MAX_NUM_OF_DISTINCTION_UNITS]; /**< An array of numOfDistinctionUnits of the
                                                                             different units to be identified */
} t_FmPcdNetEnvParams;

/**************************************************************************//**
 @Description   structure for defining a single extraction action
                when creating a key
*//***************************************************************************/
typedef struct t_FmPcdExtractEntry {
    e_FmPcdExtractType                  type;           /**< Extraction type select */
    union {
        struct {                        /**< used when type = e_FM_PCD_KG_EXTRACT_BY_HDR */
            e_NetHeaderType             hdr;            /**< Header selection */
            bool                        ignoreProtocolValidation;   /**< Ignore protocol validation */
            e_FmPcdHdrIndex             hdrIndex;       /**< Relevant only for MPLS, VLAN and tunneled
                                                             IP. Otherwise should be cleared.*/
            e_FmPcdExtractByHdrType     type;           /**< Header extraction type select */
            union {
                t_FmPcdFromHdr          fromHdr;        /**< Extract bytes from header parameters */
                t_FmPcdFromField        fromField;      /**< Extract bytes from field parameters*/
                t_FmPcdFields           fullField;      /**< Extract full filed parameters*/
            } extractByHdrType;
        } extractByHdr;
        struct {                        /**< used when type = e_FM_PCD_KG_EXTRACT_NON_HDR */
            e_FmPcdExtractFrom          src;            /**< Non-header extraction source */
            uint8_t                     offset;         /**< Byte offset */
            uint8_t                     size;           /**< Size in byte */
        } extractNonHdr;
    } extractParams;
} t_FmPcdExtractEntry;

/**************************************************************************//**
 @Description   A structure for defining masks for each extracted
                field in the key.
*//***************************************************************************/
typedef struct t_FmPcdKgExtractMask {
    uint8_t                             extractArrayIndex;       /**< Index in the extraction array, as initialized by user */
    uint8_t                             offset;                  /**< Byte offset */
    uint8_t                             mask;                    /**< A byte mask (selected bits will be used) */
} t_FmPcdKgExtractMask;

/**************************************************************************//**
 @Description   A structure for defining default selection per groups
                of fields
*//***************************************************************************/
typedef struct t_FmPcdKgExtractDflt {
    e_FmPcdKgKnownFieldsDfltTypes       type;                /**< Default type select*/
    e_FmPcdKgExtractDfltSelect          dfltSelect;          /**< Default register select */
} t_FmPcdKgExtractDflt;

/**************************************************************************//**
 @Description   A structure for defining all parameters needed for
                generation a key and using a hash function
*//***************************************************************************/
typedef struct t_FmPcdKgKeyExtractAndHashParams {
    uint32_t                    privateDflt0;                /**< Scheme default register 0 */
    uint32_t                    privateDflt1;                /**< Scheme default register 1 */
    uint8_t                     numOfUsedExtracts;           /**< defines the valid size of the following array */
    t_FmPcdExtractEntry         extractArray [FM_PCD_KG_MAX_NUM_OF_EXTRACTS_PER_KEY]; /**< An array of extractions definition. */
    uint8_t                     numOfUsedDflts;              /**< defines the valid size of the following array */
    t_FmPcdKgExtractDflt        dflts[FM_PCD_KG_NUM_OF_DEFAULT_GROUPS];
    uint8_t                     numOfUsedMasks;              /**< defines the valid size of the following array */
    t_FmPcdKgExtractMask        masks[FM_PCD_KG_NUM_OF_EXTRACT_MASKS];
    uint8_t                     hashShift;                   /**< Select the 24 bits out of the 64 hash result */
    uint32_t                    hashDistributionNumOfFqids;  /**< must be > 1 and a power of 2. Represents the range
                                                                  of queues for the key and hash functionality */
    uint8_t                     hashDistributionFqidsShift;  /**< selects the FQID bits that will be effected by the hash */
} t_FmPcdKgKeyExtractAndHashParams;

/**************************************************************************//**
 @Description   A structure of parameters for defining a single
                Fqid mask (extracted OR).
*//***************************************************************************/
typedef struct t_FmPcdKgExtractedOrForFqid {
    e_FmPcdExtractType              type;               /**< Extraction type select */
    union {
        struct {                                        /**< used when type = e_FM_PCD_KG_EXTRACT_BY_HDR */
            e_NetHeaderType         hdr;
            e_FmPcdHdrIndex         hdrIndex;           /**< Relevant only for MPLS, VLAN and tunneled
                                                             IP. Otherwise should be cleared.*/
            bool                    ignoreProtocolValidation;
                                                        /**< continue extraction even if protocol is not recognized */
        } extractByHdr;
        e_FmPcdExtractFrom          src;                /**< used when type = e_FM_PCD_KG_EXTRACT_NON_HDR */
    } extractParams;
    uint8_t                         extractionOffset;   /**< Offset for extraction (in bytes).  */
    e_FmPcdKgExtractDfltSelect      dfltValue;          /**< Select register from which extraction is taken if
                                                             field not found */
    uint8_t                         mask;               /**< Extraction mask (specified bits are used) */
    uint8_t                         bitOffsetInFqid;    /**< out of 24 bits Qid  (max offset = 16) */
} t_FmPcdKgExtractedOrForFqid;

/**************************************************************************//**
 @Description   A structure for configuring scheme counter
*//***************************************************************************/
typedef struct t_FmPcdKgSchemeCounter {
    bool        update;     /**< FALSE to keep the current counter state
                                 and continue from that point, TRUE to update/reset
                                 the counter when the scheme is written. */
    uint32_t    value;      /**< If update=TRUE, this value will be written into the
                                 counter. clear this field to reset the counter. */
} t_FmPcdKgSchemeCounter;

/**************************************************************************//**
 @Description   A structure for defining policer profile
                parameters as required by keygen (when policer
                is the next engine after this scheme).
*//***************************************************************************/
typedef struct t_FmPcdKgPlcrProfile {
    bool                sharedProfile;              /**< TRUE if this profile is shared between ports
                                                         (i.e. managed by master partition) May not be TRUE
                                                         if profile is after Coarse Classification*/
    bool                direct;                     /**< if TRUE, directRelativeProfileId only selects the profile
                                                         id, if FALSE fqidOffsetRelativeProfileIdBase is used
                                                         together with fqidOffsetShift and numOfProfiles
                                                         parameters, to define a range of profiles from
                                                         which the keygen result will determine the
                                                         destination policer profile.  */
    union {
        uint16_t        directRelativeProfileId;         /**< Used if 'direct' is TRUE, to select policer profile.
                                                         This parameter should
                                                         indicate the policer profile offset within the port's
                                                         policer profiles or SHARED window. */
        struct {
            uint8_t     fqidOffsetShift;            /**< shift of KG results without the qid base */
            uint8_t     fqidOffsetRelativeProfileIdBase;/**< OR of KG results without the qid base
                                                         This parameter should
                                                         indicate the policer profile offset within the port's
                                                         policer profiles windowor SHARED window depends on sharedProfile */
            uint8_t     numOfProfiles;              /**< Range of profiles starting at base */
        } indirectProfile;
    } profileSelect;
} t_FmPcdKgPlcrProfile;

/**************************************************************************//**
 @Description   A structure for CC parameters if CC is the next engine after KG
*//***************************************************************************/
typedef struct t_FmPcdKgCc {
    t_Handle                h_CcTree;           /**< A handle to a CC Tree */
    uint8_t                 grpId;              /**< CC group id within the CC tree */
    bool                    plcrNext;           /**< TRUE if after CC, in case of data frame,
                                                     policing is required. */
    t_FmPcdKgPlcrProfile    plcrProfile;        /**< only if plcrNext=TRUE */
} t_FmPcdKgCc;

/**************************************************************************//**
 @Description   A structure for initializing a keygen single scheme
*//***************************************************************************/
typedef struct t_FmPcdKgSchemeParams {
    bool                                modify;                 /**< IN: TRUE to change an existing scheme */
    union
    {
        uint8_t                         relativeSchemeId;       /**< IN: if modify=FALSE:Partition relative scheme id */
        t_Handle                        h_Scheme;               /**< IN: if modify=TRUE: a handle of the existing scheme */
    }id;
    bool                                alwaysDirect;           /**< IN: This scheme is reached only directly, i.e.                                                              no need for match vector. Keygen will ignore
                                                                     it when matching   */
    struct {                                                    /**< IN: HL Relevant only if alwaysDirect = FALSE */
        t_Handle                        h_NetEnv;               /**< IN: A handle to the Network environment as returned
                                                                     by FM_PCD_SetNetEnvCharacteristics() */
        uint8_t                         numOfDistinctionUnits;  /**< IN: Number of netenv units listed in unitIds array */
        uint8_t                         unitIds[FM_PCD_MAX_NUM_OF_DISTINCTION_UNITS];      /**< Indexes as passed to SetNetEnvCharacteristics array*/
    } netEnvParams;
    bool                                useHash;                /**< IN: use the KG Hash functionality  */
    t_FmPcdKgKeyExtractAndHashParams    keyExtractAndHashParams;
                                                                /**< IN: used only if useHash = TRUE */
    uint32_t                            baseFqid;               /**< IN: Base FQID */
    uint8_t                             numOfUsedFqidMasks;     /**< IN: Number of Fqid masks listed in fqidMasks array*/
    t_FmPcdKgExtractedOrForFqid         fqidMasks[FM_PCD_KG_NUM_OF_GENERIC_REGS];
                                                                /**< IN: FM_PCD_KG_NUM_OF_GENERIC_REGS
                                                                     registers are shared between qidMasks
                                                                     functionality and some of the extraction
                                                                     actions. Normally only some will be used
                                                                     for qidMask. Driver will return error if
                                                                     resource is full at initialization time. */
    e_FmPcdEngine                       nextEngine;             /**< IN: may be BMI, PLCR or CC */
    union {                                                     /**< IN: depends on nextEngine */
        t_FmPcdKgPlcrProfile            plcrProfile;            /**< IN: Used when next engine is PLCR */
        t_FmPcdKgCc                     cc;                     /**< IN: Used when next engine is CC */
    } kgNextEngineParams;
    t_FmPcdKgSchemeCounter              schemeCounter;          /**< IN: A strcucture of parameters for updating
                                                                     the scheme counter */
    t_FmPcdKgKeyOrder                   orderedArray;           /**< OUT: A structure holding the order of the key extraction.
                                                                     Relevant only is 'useHash' is TRUE. each value in this
                                                                     array represents the index of the
                                                                     extraction command as defined by the application in
                                                                     the initialization extraction array.
                                                                     The valid size of this array is the application define number of extractions
                                                                     required (also marked by the second '0' in this array).*/
} t_FmPcdKgSchemeParams;

/**************************************************************************//**
 @Description   A structure for defining CC params when CC is the
                next engine after a CC node.
*//***************************************************************************/
typedef struct t_FmPcdCcNextCcParams {
    t_Handle    h_CcNode;                           /**< A handle of the next CC node */
} t_FmPcdCcNextCcParams;

/**************************************************************************//**
 @Description   A structure for defining PLCR params when PLCR is the
                next engine after a CC node.
*//***************************************************************************/
typedef struct t_FmPcdCcNextPlcrParams {
    bool        ctrlFlow;                           /**< TRUE if this is a control flow, FALSE
                                                         if this is data flow. */
    bool        sharedProfile;                      /**< Relevant only if ctrlFlow=TRUE:
                                                         TRUE if this profile is shared between ports */
    uint16_t    relativeProfileIdForCtrlFlow;        /**< Relevant only if ctrlFlow=TRUE:
                                                         (for data flow porfile id
                                                         is taken from keygen).
                                                         This parameter should
                                                         indicate the policer profile offset within the port's
                                                         policer profiles or from SHARED window.*/
    bool        fqidEnqForCtrlFlow;                 /**< Relevant only if ctrlFlow=TRUE:
                                                         TRUE if after the policer the frame should
                                                         be enqueued rather than return to Keygen */
    uint32_t    fqidForCtrlFlowForEnqueueAfterPlcr; /**< Relevant only if ctrlFlow=TRUE:
                                                         if fqidEnqForCtrlFlow= TRUE, FQID for enquing
                                                         the frame. Unused otherwize. */
} t_FmPcdCcNextPlcrParams;

/**************************************************************************//**
 @Description   A structure for defining enqueue params when BMI is the
                next engine after a CC node.
*//***************************************************************************/
typedef struct t_FmPcdCcNextEnqueueParams {
    bool        ctrlFlow;           /**< TRUE if this is a control flow, FALSE
                                         if this is data flow */
    uint32_t    fqidForCtrlFlow;    /**< Valid if ctrlFlow=TRUE, FQID for enquing the frame
                                         (for data flow FQID is taken from keygen). */
} t_FmPcdCcNextEnqueueParams;

/**************************************************************************//**
 @Description   A structure for defining KG params when KG is the
                next engine after a CC node.
*//***************************************************************************/
typedef struct t_FmPcdCcNextKgParams {
    bool        ctrlFlow;           /**< TRUE if this is a control flow, FALSE
                                         if this is data flow */
    t_Handle    h_DirectScheme;     /**< Direct scheme handle to go to. */
} t_FmPcdCcNextKgParams;

/**************************************************************************//**
 @Description   A structure for defining next engine params after a CC node.
*//***************************************************************************/
typedef struct t_FmPcdCcNextEngineParams {
    e_FmPcdEngine                       nextEngine;    /**< User has to init parameters
                                                            according to nextEngine definition */
    union {
            t_FmPcdCcNextCcParams       ccParams;      /**< Parameters in case next engine is CC */
            t_FmPcdCcNextPlcrParams     plcrParams;    /**< Parameters in case next engine is PLCR */
            t_FmPcdCcNextEnqueueParams  enqueueParams; /**< Parameters in case next engine is BMI */
            t_FmPcdCcNextKgParams       kgParams;      /**< Parameters in case next engine is KG */
    } params;
} t_FmPcdCcNextEngineParams;

/**************************************************************************//**
 @Description   A structure for defining a single CC Key parameters
*//***************************************************************************/
typedef struct t_FmPcdCcKeyParams {
    uint8_t                     *p_Key;     /**< pointer to the key of the size defined in keySize*/
    uint8_t                     *p_Mask;    /**< pointer to the Mask per key  of the size defined
                                                 in keySize. p_Key and p_Mask (if defined) has to be
                                                 of the same size defined in the keySize*/
    t_FmPcdCcNextEngineParams   ccNextEngineParams; /**< parameters for the next for the defined Key in the p_Key*/
} t_FmPcdCcKeyParams;

/**************************************************************************//**
 @Description   A structure for defining CC Keys parameters
*//***************************************************************************/
typedef struct t_KeysParams {
    uint8_t                     numOfKeys;      /**< num Of relevant Keys  */
    uint8_t                     keySize;        /**< size of the key - in the case of the extraction of
                                                     the type FULL_FIELD keySize has to be as standard size of the relevant
                                                     key. In the another type of extraction keySize has to be as size of extraction. */

    uint8_t                     *p_GlblMask;                /**< optional and can be initialized if:
                                                                 keySize <=4 or  maskForKey is not initialized */
    t_FmPcdCcKeyParams          keyParams[MAX_NUM_OF_KEYS];               /**< it's array with numOfKeys entries each entry in
                                                                 the array of the type t_FmPcdCcKeyParams */
    t_FmPcdCcNextEngineParams   ccNextEngineParamsForMiss;  /**< parameters for the next step of
                                                                 unfound (or undefined)  key */
} t_KeysParams;

/**************************************************************************//**
 @Description   A structure for defining the CC node params
*//***************************************************************************/
typedef struct t_FmPcdCcNodeParams {
    t_FmPcdExtractEntry         extractCcParams;    /**< params which defines extraction parameters */
    t_KeysParams                keysParams;         /**< params which defines Keys parameters of the
                                                         extraction defined in  extractParams */

} t_FmPcdCcNodeParams;

/**************************************************************************//**
 @Description   A structure for defining each CC tree group in term of
                NetEnv units and the action to be taken in each case.
                the unitIds list must be in order from lower to higher indexes.

                t_FmPcdCcNextEngineParams is a list of 2^numOfDistinctionUnits
                structures where each defines the next action to be taken for
                each units combination. for example:
                numOfDistinctionUnits = 2
                unitIds = {1,3}
                p_NextEnginePerEntriesInGrp[0] = t_FmPcdCcNextEngineParams for the case that
                                                        unit 1 - not found; unit 3 - not found;
                p_NextEnginePerEntriesInGrp[1] = t_FmPcdCcNextEngineParams for the case that
                                                        unit 1 - not found; unit 3 - found;
                p_NextEnginePerEntriesInGrp[2] = t_FmPcdCcNextEngineParams for the case that
                                                        unit 1 - found; unit 3 - not found;
                p_NextEnginePerEntriesInGrp[3] = t_FmPcdCcNextEngineParams for the case that
                                                        unit 1 - found; unit 3 - found;
*//***************************************************************************/
typedef struct t_FmPcdCcGrpParams {
        uint8_t                     numOfDistinctionUnits;          /**< up to 4 */
        uint8_t                     unitIds[MAX_NUM_OF_CC_UNITS];   /**< Indexes of the units as defined in
                                                                         FM_PCD_SetNetEnvCharacteristics() */
        t_FmPcdCcNextEngineParams   *p_NextEnginePerEntriesInGrp;   /**< Max size is 16 - if only one group used */
} t_FmPcdCcGrpParams;

/**************************************************************************//**
 @Description   A structure for defining the CC tree groups
*//***************************************************************************/
typedef struct t_FmPcdCcTreeParams {
        t_Handle                h_NetEnv;                               /**< A handle to the Network environment as returned
                                                                             by FM_PCD_SetNetEnvCharacteristics() */
        uint8_t                 numOfGrps;                              /**< Number of CC groups within the CC tree */
        t_FmPcdCcGrpParams      ccGrpParams[MAX_NUM_OF_PCD_CC_GROUPS];  /**< Parameters for each group. */
} t_FmPcdCcTreeParams;

/**************************************************************************//**
 @Description   A structure for initializing a keygen classification plan group
*//***************************************************************************/
typedef struct t_FmPcdKgClsPlanGrpParams {
    t_Handle        h_NetEnv;                       /**< A handle to the Network environment as returned
                                                         by FM_PCD_SetNetEnvCharacteristics() */

    uint8_t         numOfOptions;                   /**< Number of options, to define the size of the
                                                         following array. */
    protocolOpt_t   options[FM_PCD_MAX_NUM_OF_CLS_PLANS];
                                                    /**< an option may be a basic one, such as ipv6Multicast1,
                                                         or a combination of the basic ones such as
                                                         (ethBroadcast | ethMulticast) or
                                                         (ethBroadcast | ipv4Unicast2 | mplsStacked).
                                                         No more than a total of 8 basic options may
                                                         participate in this array. */
} t_FmPcdKgClsPlanGrpParams;

/**************************************************************************//**
 @Description   A structure for defining parameters for byte rate
*//***************************************************************************/
typedef struct t_FmPcdPlcrByteRateModeParams {
    e_FmPcdPlcrFrameLengthSelect    frameLengthSelection;   /**< Frame length selection */
    e_FmPcdPlcrRollBackFrameSelect  rollBackFrameSelection; /**< relevant option only e_FM_PCD_PLCR_L2_FRM_LEN,
                                                                 e_FM_PCD_PLCR_FULL_FRM_LEN */
} t_FmPcdPlcrByteRateModeParams;

/**************************************************************************//**
 @Description   A structure for selcting the policer profile RFC 2698 or RFC 4115 parameters
*//***************************************************************************/
typedef struct t_FmPcdPlcrNonPassthroughAlgParams {
    e_FmPcdPlcrRateMode              rateMode;                       /**< Byte / Packet */
    t_FmPcdPlcrByteRateModeParams    byteModeParams;                 /**< Valid for Byte NULL for Packet */
    uint32_t                         comittedInfoRate;               /**< KBits/Sec or Packets/Sec */
    uint32_t                         comittedBurstSize;              /**< KBits or Packets */
    uint32_t                         peakOrAccessiveInfoRate;        /**< KBits/Sec or Packets/Sec */
    uint32_t                         peakOrAccessiveBurstSize;       /**< KBits or Packets */
} t_FmPcdPlcrNonPassthroughAlgParams;

/**************************************************************************//**
 @Description   A union for defining Policer next engine parameters
*//***************************************************************************/
typedef union u_FmPcdPlcrNextEngineParams {
        e_FmPcdPlcrDoneAction           action;             /**< Action - when next engine is BMI (done) */
        t_Handle                        h_Profile;          /**< Policer profile handle -  used when next engine
                                                                 is PLCR, must be a SHARED profile */
        t_Handle                        h_DirectScheme;     /**< Direct scheme select - when next engine is Keygen */
} u_FmPcdPlcrNextEngineParams;

/**************************************************************************//**
 @Description   A structure for selcting the policer profile entry parameters
*//***************************************************************************/
typedef struct t_FmPcdPlcrProfileParams {
    bool                                modify;                     /**< TRUE to change an existing profile */
    union {
        struct {
            e_FmPcdProfileTypeSelection profileType;                /**< Type of policer profile */
            t_Handle                    h_FmPort;                   /**< Relevant for per-port profiles only */
            uint16_t                    relativeProfileId;          /**< Profile id - relative to shared group or to port */
        } newParams;                                                /**< use it when modify=FALSE */
        t_Handle                        h_Profile;                  /***< A handle to a profile - use it when modify=TRUE */
    } id;
    e_FmPcdPlcrAlgorithmSelection       algSelection;               /**< Profile Algoritem PASS_THROUGH, RFC_2698, RFC_4115 */
    e_FmPcdPlcrColorMode                colorMode;                  /**< COLOR_BLIND, COLOR_AWARE */

    union {
        e_FmPcdPlcrColor                dfltColor;                  /**< For Color-Blind Pass-Through mode. the policer will re-color
                                                                         any incoming packet with the default value. */
        e_FmPcdPlcrColor                override;                   /**< For Color-Aware modes. The profile response to a
                                                                         pre-color value of 2’b11. */
    } color;

    t_FmPcdPlcrNonPassthroughAlgParams  nonPassthroughAlgParams;    /**< RFC2698 or RFC4115 params */

    e_FmPcdEngine                       nextEngineOnGreen;          /**< Green next engine type */
    u_FmPcdPlcrNextEngineParams         paramsOnGreen;              /**< Green next engine params */

    e_FmPcdEngine                       nextEngineOnYellow;         /**< Yellow next engine type */
    u_FmPcdPlcrNextEngineParams         paramsOnYellow;             /**< Yellow next engine params */

    e_FmPcdEngine                       nextEngineOnRed;            /**< Red next engine type */
    u_FmPcdPlcrNextEngineParams         paramsOnRed;                /**< Red next engine params */

    bool                                trapProfileOnFlowA;         /**< Trap on flow A */
    bool                                trapProfileOnFlowB;         /**< Trap on flow B */
    bool                                trapProfileOnFlowC;         /**< Trap on flow C */
} t_FmPcdPlcrProfileParams;


/**************************************************************************//**
 @Function      FM_PCD_SetNetEnvCharacteristics

 @Description   Define a set of Network Environment Charecteristics.
                When setting an environment it is important to understand its
                application. It is not meant to describe the flows that will run
                on the ports using this environment, but what the user means TO DO
                with the PCD mechanisms in order to parse-classify-distribute those
                frames.
                By specifying a distinction unit, the user means it would use that option
                for distinction between frames at either a keygen scheme keygen or a coarse
                classification action descriptor. Using interchangeable headers to define a
                unit means that the user is indifferent to which of the interchangeable
                headers is present in the frame, and they want the distinction to be based
                on the presence of either one of them.
                Depending on context, there are limitations to the use of environments. A
                port using the PCD functionality is bound to an environment. Some or even
                all ports may share an environment but also an environment per port is
                possible. When initializing a scheme, a classification plan group (see below),
                or a coarse classification tree, one of the initialized environments must be
                stated and related to. When a port is bound to a scheme, a classification
                plan group, or a coarse classification tree, it MUST be bound to the same
                environment.
                The different PCD modules, may relate (for flows definition) ONLY on
                distinction units as defined by their environment. When initializing a
                scheme for example, it may not choose to select IPV4 as a match for
                recognizing flows unless it was defined in the relating environment. In
                fact, to guide the user through the configuration of the PCD, each module's
                characterization in terms of flows is not done using protocol names, but using
                environment indexes.
                In terms of HW implementation, the list of distinction units sets the LCV vectors
                and later used for match vector, classification plan vectors and coarse classification
                indexing.

 @Param[in]     h_FmPcd         FM PCD module descriptor.
 @Param[in]     p_NetEnvParams  A structure of parameters for the initialization of
                                the network environment.

 @Return        A handle to the initialized object on success; NULL code otherwise.

 @Cautions      Allowed only following PCD_Init().
*//***************************************************************************/
t_Handle FM_PCD_SetNetEnvCharacteristics(t_Handle h_FmPcd, t_FmPcdNetEnvParams *p_NetEnvParams);

/**************************************************************************//**
 @Function      FM_PCD_DeleteNetEnvCharacteristics

 @Description   Deletes a set of Network Environment Charecteristics.

 @Param[in]     h_FmPcd         FM PCD module descriptor.
 @Param[in]     h_NetEnv        A handle to the Network environment.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
t_Error FM_PCD_DeleteNetEnvCharacteristics(t_Handle h_FmPcd, t_Handle h_NetEnv);

/**************************************************************************//**
 @Function      FM_PCD_KgSetClsPlanGrp

 @Description   Define a classification plan group..
                A classification plan group is a set of classification plan
                entries consisting of a number of protocol options (as listed
                in HW spec), that is a subset of a previously defined environment,
                and that is relevant for one or more ports that will use that
                same environment.
                By specifying an option, the application means it would use that
                option for distinction between frames at either a keygen scheme
                keygen or a coarse classification action descriptor.
                When RX ports that want to use the classification plan mechanism
                are initialized, they will be bound to a classification plan
                group. Usage of the classification plan is optional.
                If not all ports use classification plan, it is user's responsibility
                to declare that by calling FM_PCD_KgSetEmptyClsPlanGrp. The driver
                will then allocate a minimal number of entries for that use, and all
                ports that do not use the classification plan mechanism will
                be internally bound to that empty group.

 @Param[in]     h_FmPcd         A handle to an FM PCD Module.
 @Param[in]     p_GrpParams     A structure of classification plan parameters.

 @Return        A handle to the initialized object on success; NULL code otherwise.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
t_Handle FM_PCD_KgSetClsPlanGrp(t_Handle h_FmPcd, t_FmPcdKgClsPlanGrpParams *p_GrpParams);

/**************************************************************************//**
 @Function      FM_PCD_KgDeleteClsPlanGrp

 @Description   Delete classification plan by writing reset value (0xFFFFFFFF)
                to it - pass all LCV bits.

 @Param[in]     h_FmPcd         A handle to an FM PCD Module.
 @Param[in]     h_ClsPlanGrp    a handle to an classification-plan-group.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
t_Error FM_PCD_KgDeleteClsPlanGrp(t_Handle h_FmPcd, t_Handle h_ClsPlanGrp);

/**************************************************************************//**
 @Function      FM_PCD_KgSetScheme

 @Description   Initializing or modifying and enabling a scheme for the keygen.
                This routine should be called for adding or modifying a scheme.
                When a scheme needs modifying, the API requires that it will be
                rewritten. In such a case 'modify' should be TRUE. If the
                routine is called for a valid scheme and 'modify' is FALSE,
                it will return error.

 @Param[in]     h_FmPcd         A handle to an FM PCD Module.
 @Param[in,out] p_Scheme        A structure of parameters for defining the scheme

 @Return        A handle to the initialized scheme on success; NULL code otherwise.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
t_Handle FM_PCD_KgSetScheme (t_Handle                h_FmPcd,
                             t_FmPcdKgSchemeParams   *p_Scheme);

/**************************************************************************//**
 @Function      FM_PCD_KgDeleteScheme

 @Description   Deleting an initialized scheme.

 @Param[in]     h_FmPcd         A handle to an FM PCD Module.
 @Param[in]     h_Scheme        scheme handle as returned by FM_PCD_KgSetScheme

 @Return        E_OK on success; Error code otherwise.
 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
t_Error     FM_PCD_KgDeleteScheme(t_Handle h_FmPcd, t_Handle h_Scheme);

/**************************************************************************//**
 @Function      FM_PCD_KgGetSchemeCounter

 @Description   Reads scheme packet counter.

 @Param[in]     h_FmPcd         FM PCD module descriptor.
 @Param[in]     h_Scheme        scheme handle as returned by FM_PCD_KgSetScheme.

 @Return        Counter's current value.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
uint32_t  FM_PCD_KgGetSchemeCounter(t_Handle h_FmPcd, t_Handle h_Scheme);

/**************************************************************************//**
 @Function      FM_PCD_KgSetSchemeCounter

 @Description   Writes scheme packet counter.

 @Param[in]     h_FmPcd         FM PCD module descriptor.
 @Param[in]     h_Scheme        scheme handle as returned by FM_PCD_KgSetScheme.
 @Param[in]     value           New scheme counter value - typically '0' for
                                resetting the counter.
 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
t_Error  FM_PCD_KgSetSchemeCounter(t_Handle h_FmPcd, t_Handle h_Scheme, uint32_t value);

/**************************************************************************//**
 @Function      FM_PCD_CcBuildTree

 @Description   This routine must be called to define a complete coarse
                classification tree. This is the way to define coarse
                classification to a certain flow - the keygen schemes
                may point only to trees defined in this way.

 @Param[in]     h_FmPcd                 FM PCD module descriptor.
 @Param[in]     p_FmPcdCcTreeParams     A structure of parameters to define the tree.

 @Return        A handle to the initialized object on success; NULL code otherwise.

 @Cautions      Allowed only following PCD_Init().
*//***************************************************************************/
t_Handle FM_PCD_CcBuildTree (t_Handle             h_FmPcd,
                             t_FmPcdCcTreeParams  *p_FmPcdCcTreeParams);

/**************************************************************************//**
 @Function      FM_PCD_CcDeleteTree

 @Description   Deleting an built tree.

 @Param[in]     h_FmPcd         A handle to an FM PCD Module.
 @Param[in]     h_CcTree        A handle to a CC tree.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
t_Error FM_PCD_CcDeleteTree(t_Handle h_FmPcd, t_Handle h_CcTree);

/**************************************************************************//**
 @Function      FM_PCD_CcSetNode

 @Description   This routine should be called for each CC (coarse classification)
                node. The whole CC tree should be built bottom up so that each
                node points to already defined nodes.

 @Param[in]     h_FmPcd             FM PCD module descriptor.
 @Param[in]     p_CcNodeParam       A structure of parameters defining the CC node

 @Return        A handle to the initialized object on success; NULL code otherwise.

 @Cautions      Allowed only following PCD_Init().
*//***************************************************************************/
t_Handle   FM_PCD_CcSetNode(t_Handle             h_FmPcd,
                            t_FmPcdCcNodeParams  *p_CcNodeParam);

/**************************************************************************//**
 @Function      FM_PCD_CcDeleteNode

 @Description   Deleting an built node.

 @Param[in]     h_FmPcd         A handle to an FM PCD Module.
 @Param[in]     h_CcNode        A handle to a CC node.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
t_Error FM_PCD_CcDeleteNode(t_Handle h_FmPcd, t_Handle h_CcNode);

/**************************************************************************//**
 @Function      FM_PCD_CcTreeModifyNextEngine

 @Description   Modify the Next Engine Parameters in the entry of the tree.

 @Param[in]     h_FmPcd                     A handle to an FM PCD Module.
 @Param[in]     h_CcTree                    A handle to the tree
 @Param[in]     grpId                       A Group index in the tree
 @Param[in]     index                       Entry index in the group defined by grpId
 @Param[in]     p_FmPcdCcNextEngineParams   A structure for defining new next engine params

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_CcBuildTree().
*//***************************************************************************/
t_Error FM_PCD_CcTreeModifyNextEngine(t_Handle h_FmPcd, t_Handle h_CcTree, uint8_t grpId, uint8_t index, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams);

/**************************************************************************//**
 @Function      FM_PCD_CcNodeModifyNextEngine

 @Description   Modify the Next Engine Parameters in the relevent key entry of the node.

 @Param[in]     h_FmPcd                     A handle to an FM PCD Module.
 @Param[in]     h_CcNode                    A handle to the node
 @Param[in]     keyIndex                    Key index for Next Engine Params modifications
 @Param[in]     p_FmPcdCcNextEngineParams   A structure for defining new next engine params

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_CcSetNode().
*//***************************************************************************/
t_Error FM_PCD_CcNodeModifyNextEngine(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams);

/**************************************************************************//**
 @Function      FM_PCD_CcNodeModifyMissNextEngine

 @Description   Modify the Next Engine Parameters of the Miss key case of the node.

 @Param[in]     h_FmPcd                     A handle to an FM PCD Module.
 @Param[in]     h_CcNode                    A handle to the node
 @Param[in]     p_FmPcdCcNextEngineParams   A structure for defining new next engine params

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_CcSetNode().
*//***************************************************************************/
t_Error FM_PCD_CcNodeModifyMissNextEngine(t_Handle h_FmPcd, t_Handle h_CcNode, t_FmPcdCcNextEngineParams *p_FmPcdCcNextEngineParams);

/**************************************************************************//**
 @Function      FM_PCD_CcNodeRemoveKey

 @Description   Remove the key (include Next Engine Parameters of this key) defined by the index of the relevant node .

 @Param[in]     h_FmPcd                     A handle to an FM PCD Module.
 @Param[in]     h_CcNode                    A handle to the node
 @Param[in]     keyIndex                    Key index for removing

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_CcSetNode() not only of the relevnt node but also
                the node that points to this node
*//***************************************************************************/
t_Error FM_PCD_CcNodeRemoveKey(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex);

/**************************************************************************//**
 @Function      FM_PCD_CcNodeAddKey

 @Description   Add the key(include Next Engine Parameters of this key)in the index defined by the keyIndex .

 @Param[in]     h_FmPcd                     A handle to an FM PCD Module.
 @Param[in]     h_CcNode                    A handle to the node
 @Param[in]     keyIndex                    Key index for adding
 @Param[in]     keySize                     Key size of added key
 @Param[in]     p_KeyParams                 A pointer to the parametrs includes new key with Next Engine Parameters

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_CcSetNode() not only of the relevnt node but also
                the node that points to this node
*//***************************************************************************/
t_Error FM_PCD_CcNodeAddKey(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex, uint8_t keySize, t_FmPcdCcKeyParams  *p_KeyParams);

/**************************************************************************//**
 @Function      FM_PCD_CcNodeModifyKeyAndNextEngine

 @Description   Modify the key and Next Engine Parameters of this key in the index defined by the keyIndex .

 @Param[in]     h_FmPcd                     A handle to an FM PCD Module.
 @Param[in]     h_CcNode                    A handle to the node
 @Param[in]     keyIndex                    Key index for adding
 @Param[in]     keySize                     Key size of added key
 @Param[in]     p_KeyParams                 A pointer to the parametrs includes modified key and modified Next Engine Parameters

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_CcSetNode() not only of the relevnt node but also
                the node that points to this node
*//***************************************************************************/
t_Error FM_PCD_CcNodeModifyKeyAndNextEngine(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex, uint8_t keySize, t_FmPcdCcKeyParams  *p_KeyParams);

/**************************************************************************//**
 @Function      FM_PCD_CcNodeModifyKey

 @Description   Modify the key  in the index defined by the keyIndex .

 @Param[in]     h_FmPcd                     A handle to an FM PCD Module.
 @Param[in]     h_CcNode                    A handle to the node
 @Param[in]     keyIndex                    Key index for adding
 @Param[in]     keySize                     Key size of added key
 @Param[in]     p_Key                       A pointer to the new key
 @Param[in]     p_Mask                      A pointer to the new mask if relevnt, otherwise pointer to NULL

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_CcSetNode() not only of the relevnt node but also
                the node that points to this node
*//***************************************************************************/
t_Error FM_PCD_CcNodeModifyKey(t_Handle h_FmPcd, t_Handle h_CcNode, uint8_t keyIndex, uint8_t keySize, uint8_t  *p_Key, uint8_t *p_Mask);

/**************************************************************************//**
 @Function      FM_PCD_PlcrSetProfile

 @Description   Sets a profile entry in the policer profile table.
                The routine overrides any existing value.

 @Param[in]     h_FmPcd           A handle to an FM PCD Module.
 @Param[in]     p_Profile         A structure of parameters for defining a
                                  policer profile entry.

 @Return        A handle to the initialized object on success; NULL code otherwise.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
t_Handle FM_PCD_PlcrSetProfile(t_Handle                  h_FmPcd,
                               t_FmPcdPlcrProfileParams  *p_Profile);

/**************************************************************************//**
 @Function      FM_PCD_PlcrDeleteProfile

 @Description   Delete a profile entry in the policer profile table.
                The routine set entry to invalid.

 @Param[in]     h_FmPcd         A handle to an FM PCD Module.
 @Param[in]     h_Profile       A handle to the profile.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
t_Error FM_PCD_PlcrDeleteProfile(t_Handle h_FmPcd, t_Handle h_Profile);

/**************************************************************************//**
 @Function      FM_PCD_PlcrGetProfileCounter

 @Description   Sets an entry in the classification plan.
                The routine overrides any existing value.

 @Param[in]     h_FmPcd             A handle to an FM PCD Module.
 @Param[in]     h_Profile       A handle to the profile.
 @Param[in]     counter             Counter selector.

 @Return        specific counter value.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
uint32_t FM_PCD_PlcrGetProfileCounter(t_Handle h_FmPcd, t_Handle h_Profile, e_FmPcdPlcrProfileCounters counter);

/**************************************************************************//**
 @Function      FM_PCD_PlcrSetProfileCounter

 @Description   Sets an entry in the classification plan.
                The routine overrides any existing value.

 @Param[in]     h_FmPcd         A handle to an FM PCD Module.
 @Param[in]     h_Profile       A handle to the profile.
 @Param[in]     counter         Counter selector.
 @Param[in]     value           value to set counter with.

 @Return        E_OK on success; Error code otherwise.

 @Cautions      Allowed only following FM_PCD_Init().
*//***************************************************************************/
t_Error FM_PCD_PlcrSetProfileCounter(t_Handle h_FmPcd, t_Handle h_Profile, e_FmPcdPlcrProfileCounters counter,    uint32_t value);

/** @} */ /* end of FM_PCD_Runtime_tree_buildgrp group */
/** @} */ /* end of FM_PCD_Runtime_grp group */
/** @} */ /* end of FM_PCD_grp group */
/** @} */ /* end of FM_grp group */

#endif /* __FM_PCD_EXT */
