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
#include <linux/of_platform.h>
#include <asm/uaccess.h>
#include <asm/errno.h>
#include <asm/qe.h>        /* For struct qe_firmware */
#include <sysdev/fsl_soc.h>

/* NetCommSw Headers --------------- */
#include "std_ext.h"
#include "error_ext.h"
#include "sprint_ext.h"
#include "debug_ext.h"
#include "sys_io_ext.h"
#include "procbuff_ext.h"

#include "fm_ioctls.h"

#include "lnxwrp_fm.h"


#define PROC_PRINT(args...) offset += sprintf(buf+offset,args)

#define ADD_ADV_CONFIG_NO_RET(_func, _param)    \
    do {                                        \
        if (i<max){                             \
            p_Entry = &p_Entrys[i];             \
            p_Entry->p_Function = _func;        \
            _param                              \
            i++;                                \
        }                                       \
        else                                    \
            REPORT_ERROR(MAJOR, E_INVALID_VALUE,\
                         ("Number of advanced-configuration entries exceeded"));\
    } while (0)


static t_LnxWrpFm   lnxWrpFm;


static int fm_proc_dump_stats(char *buffer, char **start, off_t offset,
                              int length, int *eof, void *data)
{
    t_LnxWrpFmDev               *p_LnxWrpFmDev = (t_LnxWrpFmDev*)data;
    t_FmDmaStatus               fmDmaStatus;
    t_Handle                    h_ProcBuff = ProcBuff_Init(buffer,start,offset,length,eof);
    unsigned long               flags;
    int                         numOfWrittenChars;

    if (!p_LnxWrpFmDev->active || !p_LnxWrpFmDev->h_Dev)
    {
        REPORT_ERROR(MINOR, E_INVALID_STATE, ("FM not initialized!"));
        return 0;
    }

    local_irq_save(flags);
    ProcBuff_Write (h_ProcBuff, "FM driver statistics:\n");

    memset(&fmDmaStatus, 0, sizeof(fmDmaStatus));
    FM_GetDmaStatus(p_LnxWrpFmDev->h_Dev, &fmDmaStatus);
    ProcBuff_Write (h_ProcBuff,
                    "\tFM DMA statistics:\n"
                    "cmqNotEmpty: %c\n"
                    "busError: %c\n"
                    "readBufEccError: %c\n"
                    "writeBufEccSysError: %c\n"
                    "writeBufEccFmError: %c\n",
                    fmDmaStatus.cmqNotEmpty ? "T" : "F",
                    fmDmaStatus.busError ? "T" : "F",
                    fmDmaStatus.readBufEccError ? "T" : "F",
                    fmDmaStatus.writeBufEccSysError ? "T" : "F",
                    fmDmaStatus.writeBufEccFmError ? "T" : "F"
                    );
    ProcBuff_Write (h_ProcBuff,
                    "\tFM counters:\n"
                    "e_FM_COUNTERS_ENQ_TOTAL_FRAME: %d\n"
                    "e_FM_COUNTERS_DEQ_TOTAL_FRAME: %d\n"
                    "e_FM_COUNTERS_DEQ_0: %d\n"
                    "e_FM_COUNTERS_DEQ_1: %d\n"
                    "e_FM_COUNTERS_DEQ_2: %d\n"
                    "e_FM_COUNTERS_DEQ_FROM_DEFAULT: %d\n"
                    "e_FM_COUNTERS_DEQ_FROM_CONTEXT: %d\n"
                    "e_FM_COUNTERS_DEQ_FROM_FD: %d\n"
                    "e_FM_COUNTERS_DEQ_CONFIRM: %d\n"
                    "e_FM_COUNTERS_SEMAPHOR_ENTRY_FULL_REJECT: %d\n"
                    "e_FM_COUNTERS_SEMAPHOR_QUEUE_FULL_REJECT: %d\n"
                    "e_FM_COUNTERS_SEMAPHOR_SYNC_REJECT: %d\n",
                    FM_GetCounter(p_LnxWrpFmDev->h_Dev, e_FM_COUNTERS_ENQ_TOTAL_FRAME),
                    FM_GetCounter(p_LnxWrpFmDev->h_Dev, e_FM_COUNTERS_DEQ_TOTAL_FRAME),
                    FM_GetCounter(p_LnxWrpFmDev->h_Dev, e_FM_COUNTERS_DEQ_0),
                    FM_GetCounter(p_LnxWrpFmDev->h_Dev, e_FM_COUNTERS_DEQ_1),
                    FM_GetCounter(p_LnxWrpFmDev->h_Dev, e_FM_COUNTERS_DEQ_2),
                    FM_GetCounter(p_LnxWrpFmDev->h_Dev, e_FM_COUNTERS_DEQ_FROM_DEFAULT),
                    FM_GetCounter(p_LnxWrpFmDev->h_Dev, e_FM_COUNTERS_DEQ_FROM_CONTEXT),
                    FM_GetCounter(p_LnxWrpFmDev->h_Dev, e_FM_COUNTERS_DEQ_FROM_FD),
                    FM_GetCounter(p_LnxWrpFmDev->h_Dev, e_FM_COUNTERS_DEQ_CONFIRM),
                    FM_GetCounter(p_LnxWrpFmDev->h_Dev, e_FM_COUNTERS_SEMAPHOR_ENTRY_FULL_REJECT),
                    FM_GetCounter(p_LnxWrpFmDev->h_Dev, e_FM_COUNTERS_SEMAPHOR_QUEUE_FULL_REJECT),
                    FM_GetCounter(p_LnxWrpFmDev->h_Dev, e_FM_COUNTERS_SEMAPHOR_SYNC_REJECT)
                    );

    numOfWrittenChars = ProcBuff_GetNumOfWrittenChars(h_ProcBuff);
    ProcBuff_Free(h_ProcBuff);
    local_irq_restore(flags);

    return numOfWrittenChars;
}

static int fm_pcd_proc_dump_stats(char *buffer, char **start, off_t offset,
                                  int length, int *eof, void *data)
{
    t_LnxWrpFmDev               *p_LnxWrpFmDev = (t_LnxWrpFmDev*)data;
    t_Handle                    h_ProcBuff = ProcBuff_Init(buffer,start,offset,length,eof);
    unsigned long               flags;
    int                         numOfWrittenChars;

    if (!p_LnxWrpFmDev->active || !p_LnxWrpFmDev->h_PcdDev)
    {
        REPORT_ERROR(MINOR, E_INVALID_STATE, ("FM-PCD not initialized!"));
        return 0;
    }

    local_irq_save(flags);
    ProcBuff_Write (h_ProcBuff,
                    "\tFM-PCD counters:\n"
                    "e_FM_COUNTERS_ENQ_TOTAL_FRAME: %d\n"
                    "e_FM_PCD_KG_COUNTERS_TOTAL: %d\n"
                    "e_FM_PCD_PLCR_COUNTERS_YELLOW: %d\n"
                    "e_FM_PCD_PLCR_COUNTERS_RED: %d\n"
                    "e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_RED: %d\n"
                    "e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_YELLOW: %d\n"
                    "e_FM_PCD_PLCR_COUNTERS_TOTAL: %d\n"
                    "e_FM_PCD_PLCR_COUNTERS_LENGTH_MISMATCH: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_PARSE_DISPATCH: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED_WITH_ERR: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED_WITH_ERR: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED_WITH_ERR: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED_WITH_ERR: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_SOFT_PRS_CYCLES: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_SOFT_PRS_STALL_CYCLES: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_HARD_PRS_CYCLE_INCL_STALL_CYCLES: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_MURAM_READ_CYCLES: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_MURAM_READ_STALL_CYCLES: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_CYCLES: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_STALL_CYCLES: %d\n"
                    "e_FM_PCD_PRS_COUNTERS_FPM_COMMAND_STALL_CYCLES: %d\n",
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_COUNTERS_ENQ_TOTAL_FRAME),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_KG_COUNTERS_TOTAL),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PLCR_COUNTERS_YELLOW),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PLCR_COUNTERS_RED),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_RED),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_YELLOW),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PLCR_COUNTERS_TOTAL),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PLCR_COUNTERS_LENGTH_MISMATCH),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_PARSE_DISPATCH),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED_WITH_ERR),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED_WITH_ERR),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED_WITH_ERR),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED_WITH_ERR),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_SOFT_PRS_CYCLES),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_SOFT_PRS_STALL_CYCLES),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_HARD_PRS_CYCLE_INCL_STALL_CYCLES),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_MURAM_READ_CYCLES),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_MURAM_READ_STALL_CYCLES),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_CYCLES),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_STALL_CYCLES),
                    FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev, e_FM_PCD_PRS_COUNTERS_FPM_COMMAND_STALL_CYCLES)
                    );

    numOfWrittenChars = ProcBuff_GetNumOfWrittenChars(h_ProcBuff);
    ProcBuff_Free(h_ProcBuff);
    local_irq_restore(flags);

    return numOfWrittenChars;
}

static int fm_port_proc_dump_stats(char *buffer, char **start, off_t offset,
                                   int length, int *eof, void *data)
{
    t_LnxWrpFmPortDev           *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)data;
    t_LnxWrpFmDev               *p_LnxWrpFmDev;
    t_Handle                    h_ProcBuff = ProcBuff_Init(buffer,start,offset,length,eof);
    unsigned long               flags;
    int                         numOfWrittenChars;

    if (!p_LnxWrpFmPortDev)
    {
        REPORT_ERROR(MINOR, E_INVALID_STATE, ("FM-Port not initialized!"));
        return 0;
    }
    p_LnxWrpFmDev = (t_LnxWrpFmDev *)p_LnxWrpFmPortDev->h_LnxWrpFmDev;
    if (!p_LnxWrpFmDev->active || !p_LnxWrpFmDev->h_Dev)
    {
        REPORT_ERROR(MINOR, E_INVALID_STATE, ("FM not initialized!"));
        return 0;
    }

    local_irq_save(flags);
    ProcBuff_Write (h_ProcBuff,
             "\tFM %d %s Port %d counters:\n"
             "e_FM_PORT_COUNTERS_CYCLE=%d\n"
             "e_FM_PORT_COUNTERS_TASK_UTIL=%d\n"
             "e_FM_PORT_COUNTERS_QUEUE_UTIL=%d\n"
             "e_FM_PORT_COUNTERS_DMA_UTIL=%d\n"
             "e_FM_PORT_COUNTERS_FIFO_UTIL=%d\n"
             "e_FM_PORT_COUNTERS_RX_PAUSE_ACTIVATION=%d\n"
             "e_FM_PORT_COUNTERS_FRAME=%d\n"
             "e_FM_PORT_COUNTERS_DISCARD_FRAME=%d\n"
             "e_FM_PORT_COUNTERS_DEALLOC_BUF=%d\n"
             "e_FM_PORT_COUNTERS_RX_BAD_FRAME=%d\n"
             "e_FM_PORT_COUNTERS_RX_LARGE_FRAME=%d\n"
             "e_FM_PORT_COUNTERS_RX_OUT_OF_BUFFERS_DISCARD=%d\n"
             "e_FM_PORT_COUNTERS_RX_FILTER_FRAME=%d\n"
             "e_FM_PORT_COUNTERS_RX_LIST_DMA_ERR=%d\n"
             "e_FM_PORT_COUNTERS_WRED_DISCARD=%d\n"
             "e_FM_PORT_COUNTERS_LENGTH_ERR=%d\n"
             "e_FM_PORT_COUNTERS_UNSUPPRTED_FORMAT=%d\n"
             "e_FM_PORT_COUNTERS_DEQ_TOTAL=%d\n"
             "e_FM_PORT_COUNTERS_ENQ_TOTAL=%d\n"
             "e_FM_PORT_COUNTERS_DEQ_FROM_DEFAULT=%d\n"
             "e_FM_PORT_COUNTERS_DEQ_CONFIRM =%d\n",
             p_LnxWrpFmDev->id,
             /*(oh ? "OH" : (rx ? "RX" : "TX"))*/"unknown",
             p_LnxWrpFmPortDev->id,
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_CYCLE),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_TASK_UTIL),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_QUEUE_UTIL),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_DMA_UTIL),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_FIFO_UTIL),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_RX_PAUSE_ACTIVATION),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_FRAME),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_DISCARD_FRAME),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_DEALLOC_BUF),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_RX_BAD_FRAME),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_RX_LARGE_FRAME),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_RX_OUT_OF_BUFFERS_DISCARD),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_RX_FILTER_FRAME),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_RX_LIST_DMA_ERR),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_WRED_DISCARD),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_LENGTH_ERR),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_UNSUPPRTED_FORMAT),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_DEQ_TOTAL),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_ENQ_TOTAL),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_DEQ_FROM_DEFAULT),
             FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_COUNTERS_DEQ_CONFIRM)
             );

    numOfWrittenChars = ProcBuff_GetNumOfWrittenChars(h_ProcBuff);
    ProcBuff_Free(h_ProcBuff);
    local_irq_restore(flags);

    return numOfWrittenChars;
}

static int fm_proc_dump_regs(char *buffer, char **start, off_t offset,
                             int length, int *eof, void *data)
{
    unsigned long   flags;

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
    t_LnxWrpFmDev   *p_LnxWrpFmDev = (t_LnxWrpFmDev*)data;
    char            *next = buffer;
    unsigned        size = length;
    int             t;

    local_irq_save(flags);
    t = scnprintf(next, size, "FM driver registers dump.\n");
    size -= t;
    next += t;

    if (!p_LnxWrpFmDev->active || !p_LnxWrpFmDev->h_Dev)
        REPORT_ERROR(MINOR, E_INVALID_STATE, ("FM not initialized!"));
    else
        FM_DumpRegs(p_LnxWrpFmDev->h_Dev);

    local_irq_restore(flags);
    *eof = 1;

    return length - size;

#else
    t_Handle    h_ProcBuff = ProcBuff_Init(buffer,start,offset,length,eof);
    int         numOfWrittenChars;

    local_irq_save(flags);
    ProcBuff_Write (h_ProcBuff, "Debug level is too low to dump registers!!!\n");
    numOfWrittenChars = ProcBuff_GetNumOfWrittenChars(h_ProcBuff);
    ProcBuff_Free(h_ProcBuff);
    local_irq_restore(flags);

    return numOfWrittenChars;
#endif /* (defined(DEBUG_ERRORS) && ... */
}

static int fm_pcd_proc_dump_regs(char *buffer, char **start, off_t offset,
                             int length, int *eof, void *data)
{
    unsigned long   flags;

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
    t_LnxWrpFmDev   *p_LnxWrpFmDev = (t_LnxWrpFmDev*)data;
    char            *next = buffer;
    unsigned        size = length;
    int             t;

    local_irq_save(flags);
    t = scnprintf(next, size, "FM driver registers dump.\n");
    size -= t;
    next += t;

    if (!p_LnxWrpFmDev->active || !p_LnxWrpFmDev->h_PcdDev)
        REPORT_ERROR(MINOR, E_INVALID_STATE, ("FM not initialized!"));
    else
        FM_PCD_DumpRegs(p_LnxWrpFmDev->h_PcdDev);

    local_irq_restore(flags);
    *eof = 1;

    return length - size;

#else
    t_Handle    h_ProcBuff = ProcBuff_Init(buffer,start,offset,length,eof);
    int         numOfWrittenChars;

    local_irq_save(flags);
    ProcBuff_Write (h_ProcBuff, "Debug level is too low to dump registers!!!\n");
    numOfWrittenChars = ProcBuff_GetNumOfWrittenChars(h_ProcBuff);
    ProcBuff_Free(h_ProcBuff);
    local_irq_restore(flags);

    return numOfWrittenChars;
#endif /* (defined(DEBUG_ERRORS) && ... */
}

static int fm_port_proc_dump_regs(char *buffer, char **start, off_t offset,
                                  int length, int *eof, void *data)
{
    unsigned long   flags;

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
    t_LnxWrpFmPortDev           *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)data;
    char            *next = buffer;
    unsigned        size = length;
    int             t;

    local_irq_save(flags);
    t = scnprintf(next, size, "FM port driver registers dump.\n");
    size -= t;
    next += t;

    if (!p_LnxWrpFmPortDev->h_Dev)
        REPORT_ERROR(MINOR, E_INVALID_STATE, ("FM port not initialized!"));
    else
        FM_PORT_DumpRegs(p_LnxWrpFmPortDev->h_Dev);

    local_irq_restore(flags);
    *eof = 1;

    return length - size;

#else
    t_Handle    h_ProcBuff = ProcBuff_Init(buffer,start,offset,length,eof);
    int         numOfWrittenChars;

    local_irq_save(flags);
    ProcBuff_Write (h_ProcBuff, "Debug level is too low to dump registers!!!\n");
    numOfWrittenChars = ProcBuff_GetNumOfWrittenChars(h_ProcBuff);
    ProcBuff_Free(h_ProcBuff);
    local_irq_restore(flags);

    return numOfWrittenChars;
#endif /* (defined(DEBUG_ERRORS) && ... */
}

static irqreturn_t fm_irq(int irq, void *_dev)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev *)_dev;

    if (!p_LnxWrpFmDev || !p_LnxWrpFmDev->h_Dev)
        return IRQ_NONE;

    FM_EventIsr(p_LnxWrpFmDev->h_Dev);

    return IRQ_HANDLED;
}

static irqreturn_t fm_err_irq(int irq, void *_dev)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev *)_dev;

    if (!p_LnxWrpFmDev || !p_LnxWrpFmDev->h_Dev)
        return IRQ_NONE;

    FM_ErrorIsr(p_LnxWrpFmDev->h_Dev);

    return IRQ_HANDLED;
}

static volatile int   hcFrmRcv = 0;

static enum qman_cb_dqrr_result qm_tx_conf_dqrr_cb(struct qman_portal          *portal,
                                                   struct qman_fq              *fq,
                                                   const struct qm_dqrr_entry  *dq)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = ((t_FmTestFq *)fq)->h_Arg;

    FM_PCD_HcTxConf(p_LnxWrpFmDev->h_PcdDev, (t_FmFD *)&dq->fd);
    hcFrmRcv--;

    return qman_cb_dqrr_consume;
}

static enum qman_cb_dqrr_result qm_tx_dqrr_cb(struct qman_portal          *portal,
                                              struct qman_fq              *fq,
                                              const struct qm_dqrr_entry  *dq)
{
    BUG();
    return qman_cb_dqrr_consume;
}

static void qm_err_cb(struct qman_portal       *portal,
                       struct qman_fq           *fq,
                       const struct qm_mr_entry *msg)
{
    BUG();
}

static struct qman_fq * FqAlloc(t_LnxWrpFmDev   *p_LnxWrpFmDev,
                                uint32_t        fqid,
                                uint32_t        flags,
                                uint16_t        channel,
                                uint8_t         wq)
{
    int                     _errno;
    struct qman_fq          *fq = NULL;
    t_FmTestFq              *p_FmtFq;
    struct qm_mcc_initfq    initfq;

    p_FmtFq = (t_FmTestFq *)XX_Malloc(sizeof(t_FmTestFq));
    if (!p_FmtFq) {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FQ obj!!!"));
        return NULL;
    }

    p_FmtFq->fq_base.cb.dqrr = (QMAN_FQ_FLAG_NO_ENQUEUE ? qm_tx_conf_dqrr_cb : qm_tx_dqrr_cb);
    p_FmtFq->fq_base.cb.ern = p_FmtFq->fq_base.cb.dc_ern = p_FmtFq->fq_base.cb.fqs = qm_err_cb;
    p_FmtFq->h_Arg = (t_Handle)p_LnxWrpFmDev;
    if (fqid == 0) {
        flags |= QMAN_FQ_FLAG_DYNAMIC_FQID;
        flags &= ~QMAN_FQ_FLAG_NO_MODIFY;
    } else {
        flags &= ~QMAN_FQ_FLAG_DYNAMIC_FQID;
    }

    if (qman_create_fq(fqid, flags, &p_FmtFq->fq_base)) {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FQ obj - qman_new_fq!!!"));
        XX_Free(p_FmtFq);
        return NULL;
    }
    fq = &p_FmtFq->fq_base;

    if (!(flags & QMAN_FQ_FLAG_NO_MODIFY)) {
        initfq.we_mask            = QM_INITFQ_WE_DESTWQ;
        initfq.fqd.dest.channel   = channel;
        initfq.fqd.dest.wq        = wq;

        _errno = qman_init_fq(fq, QMAN_INITFQ_FLAG_SCHED, &initfq);
        if (unlikely(_errno < 0)) {
            REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FQ obj - qman_init_fq!!!"));
            qman_destroy_fq(fq, 0);
            XX_Free(p_FmtFq);
            return NULL;
        }
    }

    DBG(TRACE, ("fqid %d, flags 0x%08x, channel %d, wq %d",qman_fq_fqid(fq),flags,channel,wq));

    return fq;
}

static t_Error QmEnqueueCB (t_Handle h_Arg, uint32_t fqid, void *p_Fd)
{
    t_LnxWrpFmDev   *p_LnxWrpFmDev = (t_LnxWrpFmDev*)h_Arg;
    int             _errno, timeout=1000000;

    ASSERT_COND(p_LnxWrpFmDev);
    UNUSED(fqid);

    hcFrmRcv++;
//MemDisp((uint8_t*)p_Fd,sizeof(t_FmFD));
    _errno = qman_enqueue(p_LnxWrpFmDev->hc_tx_fq, (struct qm_fd*)p_Fd, 0);
    if (_errno)
        RETURN_ERROR(MINOR, E_INVALID_STATE, NO_MSG);

    while (hcFrmRcv && --timeout)
    {
        udelay(1);
        cpu_relax();
    }
    BUG_ON(!timeout);

    return E_OK;
}

static t_LnxWrpFmDev * CreateFmDev(uint8_t  id)
{
    t_LnxWrpFmDev   *p_LnxWrpFmDev;
    int             j;

    p_LnxWrpFmDev = (t_LnxWrpFmDev *)XX_Malloc(sizeof(t_LnxWrpFmDev));
    if (!p_LnxWrpFmDev)
    {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, NO_MSG);
        return NULL;
    }

    memset(p_LnxWrpFmDev, 0, sizeof(t_LnxWrpFmDev));
    p_LnxWrpFmDev->fmDevSettings.advConfig = (t_SysObjectAdvConfigEntry*)XX_Malloc(FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry));
    memset(p_LnxWrpFmDev->fmDevSettings.advConfig, 0, (FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry)));
    p_LnxWrpFmDev->fmPcdDevSettings.advConfig = (t_SysObjectAdvConfigEntry*)XX_Malloc(FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry));
    memset(p_LnxWrpFmDev->fmPcdDevSettings.advConfig, 0, (FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry)));
    p_LnxWrpFmDev->hcPort.settings.advConfig = (t_SysObjectAdvConfigEntry*)XX_Malloc(FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry));
    memset(p_LnxWrpFmDev->hcPort.settings.advConfig, 0, (FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry)));
    for (j=0; j<FM_MAX_NUM_OF_RX_PORTS; j++)
    {
        p_LnxWrpFmDev->rxPorts[j].settings.advConfig = (t_SysObjectAdvConfigEntry*)XX_Malloc(FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry));
        memset(p_LnxWrpFmDev->rxPorts[j].settings.advConfig, 0, (FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry)));
    }
    for (j=0; j<FM_MAX_NUM_OF_TX_PORTS; j++)
    {
        p_LnxWrpFmDev->txPorts[j].settings.advConfig = (t_SysObjectAdvConfigEntry*)XX_Malloc(FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry));
        memset(p_LnxWrpFmDev->txPorts[j].settings.advConfig, 0, (FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry)));
    }
    for (j=0; j<FM_MAX_NUM_OF_OH_PORTS-1; j++)
    {
        p_LnxWrpFmDev->opPorts[j].settings.advConfig = (t_SysObjectAdvConfigEntry*)XX_Malloc(FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry));
        memset(p_LnxWrpFmDev->opPorts[j].settings.advConfig, 0, (FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry)));
    }

    return p_LnxWrpFmDev;
}

static void DistroyFmDev(t_LnxWrpFmDev *p_LnxWrpFmDev)
{
    int             j;

    for (j=0; j<FM_MAX_NUM_OF_OH_PORTS-1; j++)
        if (p_LnxWrpFmDev->opPorts[j].settings.advConfig)
            XX_Free(p_LnxWrpFmDev->opPorts[j].settings.advConfig);
    for (j=0; j<FM_MAX_NUM_OF_TX_PORTS; j++)
        if (p_LnxWrpFmDev->txPorts[j].settings.advConfig)
            XX_Free(p_LnxWrpFmDev->txPorts[j].settings.advConfig);
    for (j=0; j<FM_MAX_NUM_OF_RX_PORTS; j++)
        if (p_LnxWrpFmDev->rxPorts[j].settings.advConfig)
            XX_Free(p_LnxWrpFmDev->rxPorts[j].settings.advConfig);
    if (p_LnxWrpFmDev->hcPort.settings.advConfig)
        XX_Free(p_LnxWrpFmDev->hcPort.settings.advConfig);
    if (p_LnxWrpFmDev->fmPcdDevSettings.advConfig)
        XX_Free(p_LnxWrpFmDev->fmPcdDevSettings.advConfig);
    if (p_LnxWrpFmDev->fmDevSettings.advConfig)
        XX_Free(p_LnxWrpFmDev->fmDevSettings.advConfig);

#ifdef NO_OF_SUPPORT
    memset(p_LnxWrpFmDev, 0, sizeof(t_LnxWrpFmDev));
#else
    XX_Free(p_LnxWrpFmDev);
#endif /* NO_OF_SUPPORT */
}

static t_Error FillRestFmInfo(t_LnxWrpFmDev *p_LnxWrpFmDev)
{
#define FM_BMI_PPIDS_OFFSET                 0x00080304
#define FM_DMA_PLR_OFFSET                   0x000c2060
#define FM_FPM_IP_REV_1_OFFSET              0x000c30c4
#define DMA_HIGH_LIODN_MASK                 0x0FFF0000
#define DMA_LOW_LIODN_MASK                  0x00000FFF
#define DMA_LIODN_SHIFT                     16

typedef _Packed struct {
    uint32_t    plr[32];
} _PackedType t_Plr;

typedef _Packed struct {
   volatile uint32_t   fmbm_ppid[63];
} _PackedType t_Ppids;

    t_Plr       *p_Plr;
    t_Ppids     *p_Ppids;
    int         i;
    uint32_t    fmRev;
    uint8_t     physRxPortId[] = {0x8,0x9,0xa,0xb,0x10};
    uint8_t     physOhPortId[] = {0x1,0x2,0x3,0x4,0x5,0x6,0x7};

    fmRev = (uint32_t)(*CAST_UINT64_TO_POINTER_TYPE(uint32_t, (p_LnxWrpFmDev->fmBaseAddr+FM_FPM_IP_REV_1_OFFSET)) & 0xffff);
    p_Plr = CAST_UINT64_TO_POINTER_TYPE(t_Plr, (p_LnxWrpFmDev->fmBaseAddr+FM_DMA_PLR_OFFSET));
#ifdef MODULE
    for (i=0;i<FM_MAX_NUM_OF_PARTITIONS/2;i++)
        p_Plr->plr[i] = 0;
#endif /* MODULE */

    for (i=0; i<FM_MAX_NUM_OF_PARTITIONS; i++)
        p_LnxWrpFmDev->fmDevSettings.param.liodnPerPartition[i] =
            (uint16_t)((i%2) ?
                       (p_Plr->plr[i/2] & DMA_LOW_LIODN_MASK) :
                       ((p_Plr->plr[i/2] & DMA_HIGH_LIODN_MASK) >> DMA_LIODN_SHIFT));

    p_Ppids = CAST_UINT64_TO_POINTER_TYPE(t_Ppids, (p_LnxWrpFmDev->fmBaseAddr+FM_BMI_PPIDS_OFFSET));

    for (i=0; i<FM_MAX_NUM_OF_RX_PORTS; i++)
            p_LnxWrpFmDev->rxPorts[i].settings.param.specificParams.rxParams.rxPartitionId =
                p_Ppids->fmbm_ppid[physRxPortId[i]-1];

#ifdef FM_OP_PARTITION_ERRATA_FMAN16
    for (i=0; i<FM_MAX_NUM_OF_OH_PORTS; i++)
    {
        /* OH port #0 is host-command, don't need this workaround */
        if (i == 0)
            continue;
        if (fmRev == 0x0100)
            p_LnxWrpFmDev->opPorts[i-1].settings.param.specificParams.nonRxParams.opPartitionId =
                p_Ppids->fmbm_ppid[physOhPortId[i]-1];
    }
#endif  /* FM_OP_PARTITION_ERRATA_FMAN16 */

    return E_OK;
}

#ifndef NO_OF_SUPPORT
/* The default address for the Fman microcode in flash. Having a default
 * allows older systems to continue functioning.  0xEF000000 is the address
 * where the firmware is normally on a P4080DS.
 */
static phys_addr_t P4080_UCAddr = 0xef000000;


/**
 * FmanUcodeAddrParam - process the fman_ucode kernel command-line parameter
 *
 * This function is called when the kernel encounters a fman_ucode command-
 * line parameter.  This parameter contains the address of the Fman microcode
 * in flash.
 */
static int FmanUcodeAddrParam(char *str)
{
    unsigned long long l;
    int ret;

    ret = strict_strtoull(str, 0, &l);
    if (!ret)
        P4080_UCAddr = (phys_addr_t) l;

    return ret;
}
__setup("fman_ucode=", FmanUcodeAddrParam);

/**
 * FindFmanMicrocode - find the Fman micrcode in memory
 *
 * This function returns a pointer to the QE Firmware blob that holds
 * the Fman microcode.  We use the QE Firmware structure because Fman microcode
 * is similar to QE microcode, so there's no point in defining a new layout.
 *
 * We also never iounmap() the memory because we might reset the Fman at any
 * time.
 */
static struct qe_firmware *FindFmanMicrocode(void)
{
    static struct qe_firmware *P4080_UCPatch;

    if (!P4080_UCPatch) {
        unsigned long P4080_UCSize;
        struct qe_header *hdr;

        /* Only map enough to the get the core structure */
        P4080_UCPatch = ioremap(P4080_UCAddr, sizeof(struct qe_firmware));
        if (!P4080_UCPatch) {
            REPORT_ERROR(MAJOR, E_NULL_POINTER, ("ioremap(%llx) returned NULL", (u64) P4080_UCAddr));
            return NULL;
        }

        /* Make sure it really is a QE Firmware blob */
        hdr = &P4080_UCPatch->header;
        if (!hdr ||
            (hdr->magic[0] != 'Q') || (hdr->magic[1] != 'E') ||
            (hdr->magic[2] != 'F')) {
            REPORT_ERROR(MAJOR, E_NOT_FOUND, ("data at %llx is not a Fman microcode", (u64) P4080_UCAddr));
            return NULL;
        }

        /* Now we call ioremap again, this time to pick up the whole blob */
        P4080_UCSize = sizeof(u32) * P4080_UCPatch->microcode[0].count;
        iounmap(P4080_UCPatch);
        P4080_UCPatch = ioremap(P4080_UCAddr, P4080_UCSize);
        if (!P4080_UCPatch) {
            REPORT_ERROR(MAJOR, E_NULL_POINTER, ("ioremap(%llx) returned NULL", (u64) P4080_UCAddr));
            return NULL;
        }
    }

    return P4080_UCPatch;
}

static t_LnxWrpFmDev * ReadFmDevTreeNode (struct of_device *of_dev)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev;
    struct device_node  *fm_node, *dev_node, *dpa_node;
    struct of_device_id name;
    struct resource     res;
    const uint32_t      *uint32_prop;
    int                 _errno=0, lenp;

    fm_node = of_dev->node;

    uint32_prop = (uint32_t *)of_get_property(fm_node, "cell-index", &lenp);
    if (unlikely(uint32_prop == NULL)) {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, cell-index) failed", fm_node->full_name));
        return NULL;
    }
    BUG_ON(lenp != sizeof(uint32_t));
    if (*uint32_prop > INTG_MAX_NUM_OF_FM) {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("fm id!"));
        return NULL;
    }
    p_LnxWrpFmDev = CreateFmDev(*uint32_prop);
    if (!p_LnxWrpFmDev) {
        REPORT_ERROR(MAJOR, E_NULL_POINTER, NO_MSG);
        return NULL;
    }
    p_LnxWrpFmDev->dev = &of_dev->dev;
    p_LnxWrpFmDev->id = *uint32_prop;

    /* Get the FM interrupt */
    p_LnxWrpFmDev->irq = of_irq_to_resource(fm_node, 0, NULL);
    if (unlikely(p_LnxWrpFmDev->irq == /*NO_IRQ*/0)) {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_irq_to_resource() = %d", NO_IRQ));
        return NULL;
    }

    /* Get the FM error interrupt */
    p_LnxWrpFmDev->err_irq = of_irq_to_resource(fm_node, 1, NULL);
    /* TODO - un-comment it once there will be err_irq in the DTS */
#if 0
    if (unlikely(p_LnxWrpFmDev->err_irq == /*NO_IRQ*/0)) {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_irq_to_resource() = %d", NO_IRQ));
        return NULL;
    }
#endif /* 0 */

    /* Get the FM address */
    _errno = of_address_to_resource(fm_node, 0, &res);
    if (unlikely(_errno < 0)) {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_address_to_resource() = %d", _errno));
        return NULL;
    }

    p_LnxWrpFmDev->fmBaseAddr = res.start;
    p_LnxWrpFmDev->fmMemSize = res.end + 1 - res.start;

    uint32_prop = (uint32_t *)of_get_property(fm_node, "clock-frequency", &lenp);
    if (unlikely(uint32_prop == NULL)) {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, clock-frequency) failed", fm_node->full_name));
        return NULL;
    }
    BUG_ON(lenp != sizeof(uint32_t));
    p_LnxWrpFmDev->fmDevSettings.param.fmClkFreq = *uint32_prop;

    /* Get the MURAM base address and size */
    memset(&name, 0, sizeof(struct of_device_id));
    BUG_ON(strlen("muram") >= sizeof(name.name));
    strcpy(name.name, "muram");
    BUG_ON(strlen("fsl,fman-muram") >= sizeof(name.compatible));
    strcpy(name.compatible, "fsl,fman-muram");
    for_each_child_of_node(fm_node, dev_node) {
        if (likely(of_match_node(&name, dev_node) != NULL)) {
            _errno = of_address_to_resource(dev_node, 0, &res);
            if (unlikely(_errno < 0)) {
                REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_address_to_resource() = %d", _errno));
                return NULL;
            }

            p_LnxWrpFmDev->fmMuramBaseAddr = res.start;
            p_LnxWrpFmDev->fmMuramMemSize = res.end + 1 - res.start;
        }
    }

    /* Get all PCD nodes */
    memset(&name, 0, sizeof(struct of_device_id));
    BUG_ON(strlen("parser") >= sizeof(name.name));
    strcpy(name.name, "parser");
    BUG_ON(strlen("fsl,fman-parser") >= sizeof(name.compatible));
    strcpy(name.compatible, "fsl,fman-parser");
    for_each_child_of_node(fm_node, dev_node)
        if (likely(of_match_node(&name, dev_node) != NULL))
            p_LnxWrpFmDev->prsActive = TRUE;

    memset(&name, 0, sizeof(struct of_device_id));
    BUG_ON(strlen("keygen") >= sizeof(name.name));
    strcpy(name.name, "keygen");
    BUG_ON(strlen("fsl,fman-keygen") >= sizeof(name.compatible));
    strcpy(name.compatible, "fsl,fman-keygen");
    for_each_child_of_node(fm_node, dev_node)
        if (likely(of_match_node(&name, dev_node) != NULL))
            p_LnxWrpFmDev->kgActive = TRUE;

    memset(&name, 0, sizeof(struct of_device_id));
    BUG_ON(strlen("cc") >= sizeof(name.name));
    strcpy(name.name, "cc");
    BUG_ON(strlen("fsl,fman-cc") >= sizeof(name.compatible));
    strcpy(name.compatible, "fsl,fman-cc");
    for_each_child_of_node(fm_node, dev_node)
        if (likely(of_match_node(&name, dev_node) != NULL))
            p_LnxWrpFmDev->ccActive = TRUE;

    memset(&name, 0, sizeof(struct of_device_id));
    BUG_ON(strlen("policer") >= sizeof(name.name));
    strcpy(name.name, "policer");
    BUG_ON(strlen("fsl,fman-policer") >= sizeof(name.compatible));
    strcpy(name.compatible, "fsl,fman-policer");
    for_each_child_of_node(fm_node, dev_node)
        if (likely(of_match_node(&name, dev_node) != NULL))
            p_LnxWrpFmDev->plcrActive = TRUE;

    if (p_LnxWrpFmDev->prsActive || p_LnxWrpFmDev->kgActive ||
        p_LnxWrpFmDev->ccActive || p_LnxWrpFmDev->plcrActive)
        p_LnxWrpFmDev->pcdActive = TRUE;

    if (p_LnxWrpFmDev->pcdActive)
    {
        const char *str_prop = (char *)of_get_property(fm_node, "fsl,default-pcd", &lenp);
        if (str_prop) {
            if (strncmp(str_prop, "3-tuple", strlen("3-tuple")) == 0)
                p_LnxWrpFmDev->defPcd = e_FM_PCD_3_TUPLE;
        }
        else
            p_LnxWrpFmDev->defPcd = e_NO_PCD;
    }

    of_node_put(fm_node);

    memset(&name, 0, sizeof(struct of_device_id));
    BUG_ON(strlen("fsl,dpa-ethernet") >= sizeof(name.compatible));
    strcpy(name.compatible, "fsl,dpa-ethernet");
    for_each_matching_node(dpa_node, &name) {
        struct device_node  *mac_node;
        const phandle       *phandle_prop;

        phandle_prop = (typeof(phandle_prop))of_get_property(dpa_node, "fsl,fman-mac", &lenp);
        if (phandle_prop == NULL)
            continue;

        BUG_ON(lenp != sizeof(phandle));

        mac_node = of_find_node_by_phandle(*phandle_prop);
        if (unlikely(mac_node == NULL)) {
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_find_node_by_phandle() failed"));
            return NULL;
        }

        fm_node = of_get_parent(mac_node);
        if (unlikely(fm_node == NULL)) {
            REPORT_ERROR(MAJOR, E_NO_DEVICE, ("of_get_parent() = %d", _errno));
            return NULL;
        }
        of_node_put(mac_node);

        uint32_prop = (uint32_t *)of_get_property(fm_node, "cell-index", &lenp);
        if (unlikely(uint32_prop == NULL)) {
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, cell-index) failed", fm_node->full_name));
            return NULL;
        }
        BUG_ON(lenp != sizeof(uint32_t));

        if (*uint32_prop == p_LnxWrpFmDev->id) {
            phandle_prop = (typeof(phandle_prop))of_get_property(dpa_node, "fsl,qman-channel", &lenp);
            if (unlikely(phandle_prop == NULL)) {
                REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, fsl,qman-channel) failed", dpa_node->full_name));
                return NULL;
            }
            BUG_ON(lenp != sizeof(phandle));

            dev_node = of_find_node_by_phandle(*phandle_prop);
            if (unlikely(dev_node == NULL)) {
                REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_find_node_by_phandle() failed"));
                return NULL;
            }

            uint32_prop = (typeof(uint32_prop))of_get_property(dev_node, "fsl,qman-channel-id", &lenp);
            if (unlikely(uint32_prop == NULL)) {
                REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, fsl,qman-channel-id) failed", dev_node->full_name));
                of_node_put(dev_node);
                return NULL;
            }
            of_node_put(dev_node);
            BUG_ON(lenp != sizeof(uint32_t));
            p_LnxWrpFmDev->hcCh = *uint32_prop;
            break;
        }

        of_node_put(fm_node);
    }

    p_LnxWrpFmDev->active = TRUE;

    return p_LnxWrpFmDev;
}

static t_LnxWrpFmPortDev * ReadFmPortDevTreeNode (struct of_device *of_dev)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev;
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev;
    struct device_node  *fm_node, *port_node;
    struct resource     res;
    const uint32_t      *uint32_prop;
    int                 _errno=0, lenp;

    port_node = of_dev->node;

    /* Get the FM node */
    fm_node = of_get_parent(port_node);
    if (unlikely(fm_node == NULL)) {
        REPORT_ERROR(MAJOR, E_NO_DEVICE, ("of_get_parent() = %d", _errno));
        return NULL;
    }

    p_LnxWrpFmDev = dev_get_drvdata(&of_find_device_by_node(fm_node)->dev);
    of_node_put(fm_node);

    uint32_prop = (uint32_t *)of_get_property(port_node, "cell-index", &lenp);
    if (unlikely(uint32_prop == NULL)) {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, cell-index) failed", port_node->full_name));
        return NULL;
    }
    BUG_ON(lenp != sizeof(uint32_t));
    if (of_device_is_compatible(port_node, "fsl,fman-port-oh")) {
        if (unlikely(*uint32_prop >= FM_MAX_NUM_OF_OH_PORTS)) {
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, cell-index) failed", port_node->full_name));
            return NULL;
        }

        if (*uint32_prop == 0) {
            p_LnxWrpFmPortDev = &p_LnxWrpFmDev->hcPort;
            p_LnxWrpFmPortDev->id = 0;
            p_LnxWrpFmPortDev->settings.param.portType = e_FM_PORT_TYPE_OH_HOST_COMMAND;
        }
        else {
            p_LnxWrpFmPortDev = &p_LnxWrpFmDev->opPorts[*uint32_prop-1];
            p_LnxWrpFmPortDev->id = *uint32_prop-1;
            p_LnxWrpFmPortDev->settings.param.portType = e_FM_PORT_TYPE_OH_OFFLINE_PARSING;
        }
        p_LnxWrpFmPortDev->settings.param.portId = *uint32_prop;

        uint32_prop = (uint32_t *)of_get_property(port_node, "fsl,qman-channel-id", &lenp);
        if (uint32_prop == NULL) {
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("missing fsl,qman-channel-id"));
            return NULL;
        }
        BUG_ON(lenp != sizeof(uint32_t));
        p_LnxWrpFmPortDev->txCh = *uint32_prop;
        p_LnxWrpFmPortDev->settings.param.specificParams.nonRxParams.deqSubPortal = (p_LnxWrpFmPortDev->txCh & 0x1f);
    }
    else if (of_device_is_compatible(port_node, "fsl,fman-port-1g-tx") ||
             of_device_is_compatible(port_node, "fsl,fman-port-10g-tx")) {
        if (unlikely(*uint32_prop >= FM_MAX_NUM_OF_TX_PORTS)) {
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, cell-index) failed", port_node->full_name));
            return NULL;
        }
        if (of_device_is_compatible(port_node, "fsl,fman-port-10g-tx"))
            p_LnxWrpFmPortDev = &p_LnxWrpFmDev->txPorts[*uint32_prop+FM_MAX_NUM_OF_1G_TX_PORTS];
        else
            p_LnxWrpFmPortDev = &p_LnxWrpFmDev->txPorts[*uint32_prop];

        p_LnxWrpFmPortDev->id = *uint32_prop;
        p_LnxWrpFmPortDev->settings.param.portId = p_LnxWrpFmPortDev->id;
        if (of_device_is_compatible(port_node, "fsl,fman-port-10g-tx"))
            p_LnxWrpFmPortDev->settings.param.portType = e_FM_PORT_TYPE_TX_10G;
        else
            p_LnxWrpFmPortDev->settings.param.portType = e_FM_PORT_TYPE_TX;

        uint32_prop = (uint32_t *)of_get_property(port_node, "fsl,qman-channel-id", &lenp);
        if (uint32_prop == NULL) {
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("missing fsl,qman-channel-id"));
            return NULL;
        }
        BUG_ON(lenp != sizeof(uint32_t));
        p_LnxWrpFmPortDev->txCh = *uint32_prop;
        p_LnxWrpFmPortDev->settings.param.specificParams.nonRxParams.deqSubPortal = (p_LnxWrpFmPortDev->txCh & 0x1f);
    }
    else if (of_device_is_compatible(port_node, "fsl,fman-port-1g-rx") ||
             of_device_is_compatible(port_node, "fsl,fman-port-10g-rx")) {
        if (unlikely(*uint32_prop >= FM_MAX_NUM_OF_RX_PORTS)) {
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, cell-index) failed", port_node->full_name));
            return NULL;
        }
        if (of_device_is_compatible(port_node, "fsl,fman-port-10g-rx"))
            p_LnxWrpFmPortDev = &p_LnxWrpFmDev->rxPorts[*uint32_prop+FM_MAX_NUM_OF_1G_RX_PORTS];
        else
            p_LnxWrpFmPortDev = &p_LnxWrpFmDev->rxPorts[*uint32_prop];

        p_LnxWrpFmPortDev->id = *uint32_prop;
        p_LnxWrpFmPortDev->settings.param.portId = p_LnxWrpFmPortDev->id;
        if (of_device_is_compatible(port_node, "fsl,fman-port-10g-rx"))
            p_LnxWrpFmPortDev->settings.param.portType = e_FM_PORT_TYPE_RX_10G;
        else
            p_LnxWrpFmPortDev->settings.param.portType = e_FM_PORT_TYPE_RX;

        if (p_LnxWrpFmDev->pcdActive)
            p_LnxWrpFmPortDev->defPcd = p_LnxWrpFmDev->defPcd;
    }
    else {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("Illegal port type"));
        return NULL;
    }

    _errno = of_address_to_resource(port_node, 0, &res);
    if (unlikely(_errno < 0)) {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_address_to_resource() = %d", _errno));
        return NULL;
    }

    p_LnxWrpFmPortDev->dev = &of_dev->dev;
    p_LnxWrpFmPortDev->baseAddr = res.start;
    p_LnxWrpFmPortDev->memSize = res.end + 1 - res.start;
    p_LnxWrpFmPortDev->settings.param.h_Fm = p_LnxWrpFmDev->h_Dev;
    p_LnxWrpFmPortDev->h_LnxWrpFmDev = (t_Handle)p_LnxWrpFmDev;

    of_node_put(port_node);

    p_LnxWrpFmPortDev->active = TRUE;

    return p_LnxWrpFmPortDev;
}
#endif /* !NO_OF_SUPPORT */

static void LnxwrpFmDevExceptionsCb(t_Handle h_App, e_FmExceptions exception)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev *)h_App;

    ASSERT_COND(p_LnxWrpFmDev);

    DBG(INFO, ("got fm exception %d", exception));

    /* do nothing */
    UNUSED(exception);
}

static void LnxwrpFmDevBusErrorCb(t_Handle        h_App,
                                  e_FmPortType    portType,
                                  uint8_t         portId,
                                  uint64_t        addr,
                                  uint8_t         tnum,
                                  uint8_t         partition)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev *)h_App;

    ASSERT_COND(p_LnxWrpFmDev);

    /* do nothing */
    UNUSED(portType);UNUSED(portId);UNUSED(addr);UNUSED(tnum);UNUSED(partition);
}

static void LnxwrpFmPcdDevExceptionsCb( t_Handle h_App, e_FmPcdExceptions exception)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev *)h_App;

    ASSERT_COND(p_LnxWrpFmDev);

    DBG(INFO, ("got fm-pcd exception %d", exception));

    /* do nothing */
    UNUSED(exception);
}

static void LnxwrpFmPcdDevIndexedExceptionsCb(t_Handle          h_App,
                                              e_FmPcdExceptions exception,
                                              uint16_t          index)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev *)h_App;

    ASSERT_COND(p_LnxWrpFmDev);

    DBG(INFO, ("got fm-pcd-indexed exception %d, indx %d", exception, index));

    /* do nothing */
    UNUSED(exception);UNUSED(index);
}

static t_Error ConfigureFmDev(t_LnxWrpFmDev  *p_LnxWrpFmDev)
{
    struct resource     *dev_res;
    int                 _errno;
    uint32_t            fmPhysAddr, muramPhysAddr;

    if (!p_LnxWrpFmDev->active)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM not configured!!!"));

    _errno = can_request_irq(p_LnxWrpFmDev->irq, 0);
    if (unlikely(_errno < 0))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("can_request_irq() = %d", _errno));
    _errno = devm_request_irq(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->irq, fm_irq, 0, "fman", p_LnxWrpFmDev);
    if (unlikely(_errno < 0))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("request_irq(%d) = %d", p_LnxWrpFmDev->irq, _errno));

    if (p_LnxWrpFmDev->err_irq != 0) {
        _errno = can_request_irq(p_LnxWrpFmDev->err_irq, 0);
        if (unlikely(_errno < 0))
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("can_request_irq() = %d", _errno));
        _errno = devm_request_irq(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->err_irq, fm_err_irq, IRQF_SHARED, "fman-err", p_LnxWrpFmDev);
        if (unlikely(_errno < 0))
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("request_irq(%d) = %d", p_LnxWrpFmDev->err_irq, _errno));
    }

    fmPhysAddr = p_LnxWrpFmDev->fmBaseAddr;
    p_LnxWrpFmDev->res = devm_request_mem_region(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->fmBaseAddr, p_LnxWrpFmDev->fmMemSize, "fman");
    if (unlikely(p_LnxWrpFmDev->res == NULL))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("request_mem_region() failed"));

    p_LnxWrpFmDev->fmBaseAddr = CAST_POINTER_TO_UINT64(devm_ioremap(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->fmBaseAddr, p_LnxWrpFmDev->fmMemSize));
    if (unlikely(p_LnxWrpFmDev->fmBaseAddr == 0))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("devm_ioremap() failed"));

    muramPhysAddr = p_LnxWrpFmDev->fmMuramBaseAddr;
    dev_res = __devm_request_region(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->res, p_LnxWrpFmDev->fmMuramBaseAddr, p_LnxWrpFmDev->fmMuramMemSize, "fman-muram");
    if (unlikely(dev_res == NULL))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("__devm_request_region() failed"));

    p_LnxWrpFmDev->fmMuramBaseAddr = CAST_POINTER_TO_UINT64(devm_ioremap(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->fmMuramBaseAddr, p_LnxWrpFmDev->fmMuramMemSize));
    if (unlikely(p_LnxWrpFmDev->fmMuramBaseAddr == 0))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("devm_ioremap() failed"));

    if (SYS_RegisterIoMap((uint64_t)p_LnxWrpFmDev->fmMuramBaseAddr, (uint64_t)muramPhysAddr, p_LnxWrpFmDev->fmMuramMemSize) != E_OK)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM MURAM memory map"));

    if (SYS_RegisterIoMap((uint64_t)p_LnxWrpFmDev->fmBaseAddr, (uint64_t)fmPhysAddr, p_LnxWrpFmDev->fmMemSize) != E_OK)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM memory map"));

    p_LnxWrpFmDev->fmDevSettings.param.baseAddr     = p_LnxWrpFmDev->fmBaseAddr;
    p_LnxWrpFmDev->fmDevSettings.param.fmId         = p_LnxWrpFmDev->id;
    p_LnxWrpFmDev->fmDevSettings.param.irq          = NO_IRQ;
    p_LnxWrpFmDev->fmDevSettings.param.errIrq       = NO_IRQ;
    p_LnxWrpFmDev->fmDevSettings.param.f_Exception  = LnxwrpFmDevExceptionsCb;
    p_LnxWrpFmDev->fmDevSettings.param.f_BusError   = LnxwrpFmDevBusErrorCb;
    p_LnxWrpFmDev->fmDevSettings.param.h_App        = p_LnxWrpFmDev;

    return FillRestFmInfo(p_LnxWrpFmDev);
}

static t_Error ConfigureFmPortDev(t_LnxWrpFmPortDev *p_LnxWrpFmPortDev)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev *)p_LnxWrpFmPortDev->h_LnxWrpFmDev;
    struct resource     *dev_res;

    if (!p_LnxWrpFmPortDev->active)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM port not configured!!!"));

    dev_res = __devm_request_region(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->res, p_LnxWrpFmPortDev->baseAddr, p_LnxWrpFmPortDev->memSize, "fman-port-hc");
    if (unlikely(dev_res == NULL))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("__devm_request_region() failed"));
    p_LnxWrpFmPortDev->baseAddr = CAST_POINTER_TO_UINT64(devm_ioremap(p_LnxWrpFmDev->dev, p_LnxWrpFmPortDev->baseAddr, p_LnxWrpFmPortDev->memSize));
    if (unlikely(p_LnxWrpFmPortDev->baseAddr == 0))
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("devm_ioremap() failed"));

    p_LnxWrpFmPortDev->settings.param.baseAddr = p_LnxWrpFmPortDev->baseAddr;

    return E_OK;
}

static t_Error InitFmPcdDev(t_LnxWrpFmDev  *p_LnxWrpFmDev)
{
    if (p_LnxWrpFmDev->pcdActive)
    {
        t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = &p_LnxWrpFmDev->hcPort;
        t_FmPcdParams       fmPcdParams;
        t_Error             err;

        memset(&fmPcdParams, 0, sizeof(fmPcdParams));
        fmPcdParams.h_Fm        = p_LnxWrpFmDev->h_Dev;
        fmPcdParams.h_FmMuram   = p_LnxWrpFmDev->h_MuramDev;
        fmPcdParams.prsSupport  = p_LnxWrpFmDev->prsActive;
        fmPcdParams.kgSupport   = p_LnxWrpFmDev->kgActive;
        fmPcdParams.plcrSupport = p_LnxWrpFmDev->plcrActive;
        fmPcdParams.ccSupport   = p_LnxWrpFmDev->ccActive;

#ifndef CONFIG_GUEST_PARTITION
        fmPcdParams.f_Exception   = LnxwrpFmPcdDevExceptionsCb;
        if (fmPcdParams.kgSupport)
            fmPcdParams.f_ExceptionId  = LnxwrpFmPcdDevIndexedExceptionsCb;
        fmPcdParams.h_App              = p_LnxWrpFmDev;
#endif /* !CONFIG_GUEST_PARTITION */

#ifdef CONFIG_MULTI_PARTITION_SUPPORT
        fmPcdParams.numOfSchemes = 0;
        fmPcdParams.numOfClsPlanEntries = 0;
        fmPcdParams.partitionId = 0;
#endif  /* CONFIG_MULTI_PARTITION_SUPPORT */
        fmPcdParams.useHostCommand = TRUE;
        p_LnxWrpFmDev->hc_tx_fq  =
            FqAlloc(p_LnxWrpFmDev,
                    0,
                    QMAN_FQ_FLAG_TO_DCPORTAL,
                    p_LnxWrpFmPortDev->txCh,
                    0);
        p_LnxWrpFmDev->hc_tx_conf_fq  =
            FqAlloc(p_LnxWrpFmDev,
                    0,
                    QMAN_FQ_FLAG_NO_ENQUEUE,
                    p_LnxWrpFmDev->hcCh,
                    7);
        p_LnxWrpFmDev->hc_tx_err_fq  =
            FqAlloc(p_LnxWrpFmDev,
                    0,
                    QMAN_FQ_FLAG_NO_ENQUEUE,
                    p_LnxWrpFmDev->hcCh,
                    7);

        fmPcdParams.hc.portBaseAddr = p_LnxWrpFmPortDev->baseAddr;
        fmPcdParams.hc.portId = p_LnxWrpFmPortDev->id;
        fmPcdParams.hc.errFqid = qman_fq_fqid(p_LnxWrpFmDev->hc_tx_err_fq);
        fmPcdParams.hc.confFqid = qman_fq_fqid(p_LnxWrpFmDev->hc_tx_conf_fq);
        fmPcdParams.hc.deqSubPortal = (p_LnxWrpFmPortDev->txCh & 0x1f);
        fmPcdParams.hc.enqFqid = qman_fq_fqid(p_LnxWrpFmDev->hc_tx_fq);
        fmPcdParams.hc.f_QmEnqueue = QmEnqueueCB;
        fmPcdParams.hc.h_QmArg = (t_Handle)p_LnxWrpFmDev;

        p_LnxWrpFmDev->h_PcdDev = FM_PCD_Config(&fmPcdParams);
        if(!p_LnxWrpFmDev->h_PcdDev)
            RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("FM PCD!"));

        if((err = FM_PCD_ConfigPlcrNumOfSharedProfiles(p_LnxWrpFmDev->h_PcdDev,
                                                       LNXWRP_FM_NUM_OF_SHARED_PROFILES))!= E_OK)
            RETURN_ERROR(MAJOR, err, NO_MSG);

        if (p_LnxWrpFmDev->err_irq != 0) {
            FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,e_FM_PCD_KG_EXCEPTION_DOUBLE_ECC,FALSE);
            FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,e_FM_PCD_KG_EXCEPTION_KEYSIZE_OVERFLOW,FALSE);
            FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,e_FM_PCD_PLCR_EXCEPTION_INIT_ENTRY_ERROR,FALSE);
            FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,e_FM_PCD_PLCR_EXCEPTION_DOUBLE_ECC,FALSE);
            FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,e_FM_PCD_PRS_EXCEPTION_DOUBLE_ECC,FALSE);
            FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,e_FM_PCD_PRS_EXCEPTION_ILLEGAL_ACCESS,FALSE);
            FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,e_FM_PCD_PRS_EXCEPTION_PORT_ILLEGAL_ACCESS,FALSE);
        }

        if((err = FM_PCD_Init(p_LnxWrpFmDev->h_PcdDev))!= E_OK)
            RETURN_ERROR(MAJOR, err, NO_MSG);
    }

    return E_OK;
}

static t_Error InitFmDev(t_LnxWrpFmDev  *p_LnxWrpFmDev)
{
    struct qe_firmware *fw;

    if (!p_LnxWrpFmDev->active)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM not configured!!!"));

    if ((p_LnxWrpFmDev->h_MuramDev = FM_MURAM_ConfigAndInit(p_LnxWrpFmDev->fmMuramBaseAddr, p_LnxWrpFmDev->fmMuramMemSize)) == NULL)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("FM-MURAM!"));

    /* Loading the fman-controller code */
    fw = FindFmanMicrocode();
    if (!fw)
        /* We already reported an error, so just return NULL*/
        return ERROR_CODE(E_NULL_POINTER);

    p_LnxWrpFmDev->fmDevSettings.param.firmware.p_Code =
        (void *) fw + fw->microcode[0].code_offset;
    p_LnxWrpFmDev->fmDevSettings.param.firmware.size =
        sizeof(u32) * fw->microcode[0].count;
    DBG(INFO, ("Loading fman-controller code version %d.%d.%d",
               fw->microcode[0].major,
               fw->microcode[0].minor,
               fw->microcode[0].revision));

    p_LnxWrpFmDev->fmDevSettings.param.h_FmMuram = p_LnxWrpFmDev->h_MuramDev;

    if ((p_LnxWrpFmDev->h_Dev = FM_Config(&p_LnxWrpFmDev->fmDevSettings.param)) == NULL)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("FM"));

    if (FM_ConfigResetOnInit(p_LnxWrpFmDev->h_Dev, TRUE) != E_OK)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM"));

    if (p_LnxWrpFmDev->err_irq != 0) {
        FM_SetException(p_LnxWrpFmDev->h_Dev, e_FM_EX_DMA_BUS_ERROR,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_DMA_READ_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_DMA_SYSTEM_WRITE_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_DMA_FM_WRITE_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_FPM_STALL_ON_TASKS , FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_FPM_SINGLE_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_FPM_DOUBLE_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_IRAM_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_MURAM_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_QMI_DOUBLE_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_QMI_DEQ_FROM_UNKNOWN_PORTID,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_BMI_LIST_RAM_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_BMI_PIPELINE_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_BMI_STATISTICS_RAM_ECC, FALSE);
    }

    if (FM_Init(p_LnxWrpFmDev->h_Dev) != E_OK)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM"));

//    return InitFmPcdDev(p_LnxWrpFmDev);
    return E_OK;
}

static t_Error InitFmPort3TupleDefPcd(t_LnxWrpFmPortDev *p_LnxWrpFmPortDev)
{
    t_LnxWrpFmDev                       *p_LnxWrpFmDev = (t_LnxWrpFmDev *)p_LnxWrpFmPortDev->h_LnxWrpFmDev;
    t_FmPcdNetEnvParams                 netEnvParam;
    t_FmPcdKgSchemeParams               schemeParam;
    t_FmPortPcdParams                   pcdParam;
    t_FmPortPcdPrsParams                prsParam;
    t_FmPortPcdKgParams                 kgParam;
    uint8_t                             i, j;

    if (!p_LnxWrpFmDev->kgActive)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("keygen must be enabled for 3-tuple PCD!"));

    if (!p_LnxWrpFmDev->prsActive)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("parser must be enabled for 3-tuple PCD!"));

    if (p_LnxWrpFmPortDev->pcdNumOfQs < 9)
        RETURN_ERROR(MINOR, E_INVALID_VALUE, ("Need to save at least 18 queues for 3-tuple PCD!!!"));

    p_LnxWrpFmPortDev->totalNumOfSchemes = p_LnxWrpFmPortDev->numOfSchemesUsed = 2;

    if (AllocSchemesForPort(p_LnxWrpFmDev, p_LnxWrpFmPortDev->totalNumOfSchemes, &p_LnxWrpFmPortDev->schemesBase) != E_OK)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("No schemes for Rx or OP port for 3-tuple PCD!!!"));

    /* set netEnv */
    memset(&netEnvParam, 0, sizeof(t_FmPcdNetEnvParams));
    netEnvParam.numOfDistinctionUnits = 2;
    netEnvParam.units[0].hdrs[0].hdr = HEADER_TYPE_IPv4; /* no special options */
    netEnvParam.units[1].hdrs[0].hdr = HEADER_TYPE_ETH;
    p_LnxWrpFmPortDev->h_DefNetEnv = FM_PCD_SetNetEnvCharacteristics(p_LnxWrpFmDev->h_PcdDev, &netEnvParam);
    if(!p_LnxWrpFmPortDev->h_DefNetEnv)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("FM PCD!"));

    for(i=0; i<p_LnxWrpFmPortDev->numOfSchemesUsed; i++)
    {
        memset(&schemeParam, 0, sizeof(schemeParam));
        schemeParam.modify = FALSE;
        schemeParam.id.relativeSchemeId = i+p_LnxWrpFmPortDev->schemesBase;
        schemeParam.alwaysDirect = FALSE;
        schemeParam.netEnvParams.h_NetEnv = p_LnxWrpFmPortDev->h_DefNetEnv;
        schemeParam.schemeCounter.update = TRUE;
        schemeParam.schemeCounter.value = 0;

        switch (i)
        {
            case (0): /* catch IPv4 */
                schemeParam.netEnvParams.numOfDistinctionUnits = 1;
                schemeParam.netEnvParams.unitIds[0] = 0;
                schemeParam.baseFqid = p_LnxWrpFmPortDev->pcdBaseQ;
                schemeParam.nextEngine = e_FM_PCD_DONE;
                schemeParam.numOfUsedFqidMasks = 0;
                schemeParam.useHash = TRUE;
                schemeParam.keyExtractAndHashParams.numOfUsedExtracts = 3;
                for(j=0; j<schemeParam.keyExtractAndHashParams.numOfUsedExtracts; j++)
                {
                    schemeParam.keyExtractAndHashParams.extractArray[j].type = e_FM_PCD_EXTRACT_BY_HDR;
                    schemeParam.keyExtractAndHashParams.extractArray[j].extractParams.extractByHdr.hdr = HEADER_TYPE_IPv4;
                    schemeParam.keyExtractAndHashParams.extractArray[j].extractParams.extractByHdr.ignoreProtocolValidation = FALSE;
                    schemeParam.keyExtractAndHashParams.extractArray[j].extractParams.extractByHdr.type = e_FM_PCD_EXTRACT_FULL_FIELD;
                }
                schemeParam.keyExtractAndHashParams.extractArray[0].extractParams.extractByHdr.extractByHdrType.fullField.ipv4 = NET_HEADER_FIELD_IPv4_PROTO;
                schemeParam.keyExtractAndHashParams.extractArray[1].extractParams.extractByHdr.extractByHdrType.fullField.ipv4 = NET_HEADER_FIELD_IPv4_SRC_IP;
                schemeParam.keyExtractAndHashParams.extractArray[2].extractParams.extractByHdr.extractByHdrType.fullField.ipv4 = NET_HEADER_FIELD_IPv4_DST_IP;

                if(schemeParam.useHash)
                {
                    schemeParam.keyExtractAndHashParams.privateDflt0 = 0x01020304;
                    schemeParam.keyExtractAndHashParams.privateDflt1 = 0x11121314;
                    schemeParam.keyExtractAndHashParams.numOfUsedDflts = FM_PCD_KG_NUM_OF_DEFAULT_GROUPS;
                    for(j=0; j<FM_PCD_KG_NUM_OF_DEFAULT_GROUPS; j++)
                    {
                        schemeParam.keyExtractAndHashParams.dflts[j].type = (e_FmPcdKgKnownFieldsDfltTypes)j; /* all types */
                        schemeParam.keyExtractAndHashParams.dflts[j].dfltSelect = e_FM_PCD_KG_DFLT_GBL_0;
                    }
                    schemeParam.keyExtractAndHashParams.numOfUsedMasks = 0;
                    schemeParam.keyExtractAndHashParams.hashShift = 0;
                    schemeParam.keyExtractAndHashParams.hashDistributionNumOfFqids = 8;
                }
                break;

            case (1): /* Garbage collector */
                schemeParam.netEnvParams.numOfDistinctionUnits = 0;
                schemeParam.baseFqid = p_LnxWrpFmPortDev->pcdBaseQ+8;
                break;

            default:
                break;
        }

        p_LnxWrpFmPortDev->h_Schemes[i] = FM_PCD_KgSetScheme(p_LnxWrpFmDev->h_PcdDev, &schemeParam);
        if(!p_LnxWrpFmPortDev->h_Schemes[i])
            RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("FM_PCD_KgSetScheme failed"));
    }

    /* initialize PCD parameters */
    memset(&pcdParam, 0, sizeof( t_FmPortPcdParams));
    pcdParam.h_NetEnv   = p_LnxWrpFmPortDev->h_DefNetEnv;
    pcdParam.pcdSupport = e_FM_PORT_PCD_SUPPORT_PRS_AND_KG;

    /* initialize Keygen parameters */
    memset(&prsParam, 0, sizeof( t_FmPortPcdPrsParams));

    prsParam.parsingOffset = 0;
    prsParam.firstPrsHdr = HEADER_TYPE_ETH;
    pcdParam.p_PrsParams = &prsParam;

    /* initialize Parser parameters */
    memset(&kgParam, 0, sizeof( t_FmPortPcdKgParams));
    kgParam.numOfSchemes = p_LnxWrpFmPortDev->numOfSchemesUsed;
    for(i=0;i<kgParam.numOfSchemes;i++)
        kgParam.h_Schemes[i] = p_LnxWrpFmPortDev->h_Schemes[i];

    pcdParam.p_KgParams = &kgParam;

    return FM_PORT_SetPCD(p_LnxWrpFmPortDev->h_Dev, &pcdParam);
}

static t_Error InitFmPortDev(t_LnxWrpFmPortDev *p_LnxWrpFmPortDev)
{
#define MY_ADV_CONFIG_CHECK_END                                 \
            RETURN_ERROR(MAJOR, E_INVALID_SELECTION,            \
                         ("Advanced configuration routine"));   \
        if (errCode != E_OK)                                    \
            RETURN_ERROR(MAJOR, errCode, NO_MSG);               \
    }

    int                 i = 0;

    if (!p_LnxWrpFmPortDev->active || p_LnxWrpFmPortDev->initialized)
        return E_INVALID_STATE;

    if ((p_LnxWrpFmPortDev->h_Dev = FM_PORT_Config(&p_LnxWrpFmPortDev->settings.param)) == NULL)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("FM-port"));

    if ((p_LnxWrpFmPortDev->settings.param.portType == e_FM_PORT_TYPE_TX_10G) ||
             (p_LnxWrpFmPortDev->settings.param.portType == e_FM_PORT_TYPE_TX))
    {
        t_Error         errCode;
        if ((errCode = FM_PORT_ConfigDeqHighPriority(p_LnxWrpFmPortDev->h_Dev, TRUE)) != E_OK)
             RETURN_ERROR(MAJOR, errCode, NO_MSG);
        if ((errCode = FM_PORT_ConfigDeqPrefetchOption(p_LnxWrpFmPortDev->h_Dev, e_FM_PORT_DEQ_FULL_PREFETCH)) != E_OK)
             RETURN_ERROR(MAJOR, errCode, NO_MSG);
    }

    if (((t_LnxWrpFmDev *)p_LnxWrpFmPortDev->h_LnxWrpFmDev)->err_irq != 0) {
        FM_MAC_SetException(p_LnxWrpFmPortDev->h_Dev, e_FM_MAC_EX_10G_MDIO_SCAN_EVENTMDIO,FALSE);
        FM_MAC_SetException(p_LnxWrpFmPortDev->h_Dev, e_FM_MAC_EX_10G_MDIO_CMD_CMPL,FALSE);
        FM_MAC_SetException(p_LnxWrpFmPortDev->h_Dev, e_FM_MAC_EX_10G_REM_FAULT,FALSE);
        FM_MAC_SetException(p_LnxWrpFmPortDev->h_Dev, e_FM_MAC_EX_10G_LOC_FAULT,FALSE);
        FM_MAC_SetException(p_LnxWrpFmPortDev->h_Dev, e_FM_MAC_EX_10G_1TX_ECC_ER,FALSE);
        FM_MAC_SetException(p_LnxWrpFmPortDev->h_Dev, e_FM_MAC_EX_10G_TX_FIFO_UNFL,FALSE);
        FM_MAC_SetException(p_LnxWrpFmPortDev->h_Dev, e_FM_MAC_EX_10G_TX_FIFO_OVFL,FALSE);
        FM_MAC_SetException(p_LnxWrpFmPortDev->h_Dev, e_FM_MAC_EX_10G_TX_ER,FALSE);
        FM_MAC_SetException(p_LnxWrpFmPortDev->h_Dev, e_FM_MAC_EX_10G_RX_FIFO_OVFL,FALSE);
        FM_MAC_SetException(p_LnxWrpFmPortDev->h_Dev, e_FM_MAC_EX_10G_RX_ECC_ER,FALSE);
        FM_MAC_SetException(p_LnxWrpFmPortDev->h_Dev, e_FM_MAC_EX_10G_RX_JAB_FRM,FALSE);
        FM_MAC_SetException(p_LnxWrpFmPortDev->h_Dev, e_FM_MAC_EX_10G_RX_OVRSZ_FRM,FALSE);
    }

    /* Call the driver's advanced configuration routines, if requested:
       Compare the function pointer of each entry to the available routines,
       and invoke the matching routine with proper casting of arguments. */
    while (p_LnxWrpFmPortDev->settings.advConfig[i].p_Function)
    {
        ADV_CONFIG_CHECK_START(&(p_LnxWrpFmPortDev->settings.advConfig[i]))

        ADV_CONFIG_CHECK(p_LnxWrpFmPortDev->h_Dev, FM_PORT_ConfigBufferPrefixContent,   PARAMS(1, (t_FmPortBufferPrefixContent*)))

        MY_ADV_CONFIG_CHECK_END

        /* Advance to next advanced configuration entry */
        i++;
    }

    if (FM_PORT_Init(p_LnxWrpFmPortDev->h_Dev) != E_OK)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, NO_MSG);

    FM_PORT_Disable(p_LnxWrpFmPortDev->h_Dev);

    if ((p_LnxWrpFmPortDev->defPcd != e_NO_PCD) &&
        (InitFmPort3TupleDefPcd(p_LnxWrpFmPortDev) != E_OK))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, NO_MSG);

    p_LnxWrpFmPortDev->initialized = TRUE;

    return E_OK;
}

static void FreeFmDev(t_LnxWrpFmDev  *p_LnxWrpFmDev)
{
    if (!p_LnxWrpFmDev->active)
        return;

    if (p_LnxWrpFmDev->h_PcdDev)
        FM_PCD_Free(p_LnxWrpFmDev->h_PcdDev);

    if (p_LnxWrpFmDev->h_Dev)
        FM_Free(p_LnxWrpFmDev->h_Dev);

    if (p_LnxWrpFmDev->h_MuramDev)
        FM_MURAM_Free(p_LnxWrpFmDev->h_MuramDev);

    SYS_UnregisterIoMap((uint64_t)p_LnxWrpFmDev->fmMuramBaseAddr);
    devm_iounmap(p_LnxWrpFmDev->dev, CAST_UINT64_TO_POINTER(p_LnxWrpFmDev->fmMuramBaseAddr));
    __devm_release_region(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->res, p_LnxWrpFmDev->fmMuramBaseAddr, p_LnxWrpFmDev->fmMuramMemSize);
    SYS_UnregisterIoMap((uint64_t)p_LnxWrpFmDev->fmBaseAddr);
    devm_iounmap(p_LnxWrpFmDev->dev, CAST_UINT64_TO_POINTER(p_LnxWrpFmDev->fmBaseAddr));
    release_mem_region(p_LnxWrpFmDev->fmBaseAddr, p_LnxWrpFmDev->fmMemSize);
//    devm_release_mem_region(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->fmBaseAddr, p_LnxWrpFmDev->fmMemSize);
}

static void FreeFmPortDev(t_LnxWrpFmPortDev *p_LnxWrpFmPortDev)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev *)p_LnxWrpFmPortDev->h_LnxWrpFmDev;

    if (!p_LnxWrpFmPortDev->active)
        return;

    if (p_LnxWrpFmPortDev->h_Dev)
        FM_PORT_Free(p_LnxWrpFmPortDev->h_Dev);
    devm_iounmap(p_LnxWrpFmDev->dev, CAST_UINT64_TO_POINTER(p_LnxWrpFmPortDev->baseAddr));
    __devm_release_region(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->res, p_LnxWrpFmPortDev->baseAddr, p_LnxWrpFmPortDev->memSize);
}


/*****************************************************************************/
/*               API routines for the FM Linux Device                        */
/*****************************************************************************/

static int fm_open(struct inode *inode, struct file *file)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = NULL;
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = NULL;
    unsigned int        major = imajor(inode);
    unsigned int        minor = iminor(inode);

    DBG(TRACE, ("Opening minor - %d - ", minor));

    if (file->private_data != NULL)
        return 0;

#ifdef NO_OF_SUPPORT
{
    int                 i;
    for (i=0; i<INTG_MAX_NUM_OF_FM; i++)
        if (lnxWrpFm.p_FmDevs[i]->major == major)
            p_LnxWrpFmDev = lnxWrpFm.p_FmDevs[i];
}
#else
{
    struct of_device_id name;
    struct device_node  *fm_node;

    /* Get all the FM nodes */
    memset(&name, 0, sizeof(struct of_device_id));
    BUG_ON(strlen("fsl,fman") >= sizeof(name.compatible));
    strcpy(name.compatible, "fsl,fman");
    for_each_matching_node(fm_node, &name) {
        struct of_device    *of_dev;

        of_dev = of_find_device_by_node(fm_node);
        if (unlikely(of_dev == NULL)) {
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("fm id!"));
            return -ENXIO;
        }

        p_LnxWrpFmDev = (t_LnxWrpFmDev *)fm_bind(&of_dev->dev);
        if (p_LnxWrpFmDev->major == major)
            break;
        fm_unbind((struct fm *)p_LnxWrpFmDev);
        p_LnxWrpFmDev = NULL;
    }
}
#endif /* NO_OF_SUPPORT */

    if (!p_LnxWrpFmDev)
        return -ENODEV;

    if (minor == DEV_FM_MINOR_BASE)
        file->private_data = p_LnxWrpFmDev;
    else if (minor == DEV_FM_PCD_MINOR_BASE)
        file->private_data = p_LnxWrpFmDev;
    else {
        if (minor == DEV_FM_OH_PORTS_MINOR_BASE)
            p_LnxWrpFmPortDev = &p_LnxWrpFmDev->hcPort;
        else if ((minor > DEV_FM_OH_PORTS_MINOR_BASE) && (minor < DEV_FM_RX_PORTS_MINOR_BASE))
            p_LnxWrpFmPortDev = &p_LnxWrpFmDev->opPorts[minor-DEV_FM_OH_PORTS_MINOR_BASE-1];
        else if ((minor >= DEV_FM_RX_PORTS_MINOR_BASE) && (minor < DEV_FM_TX_PORTS_MINOR_BASE))
            p_LnxWrpFmPortDev = &p_LnxWrpFmDev->rxPorts[minor-DEV_FM_RX_PORTS_MINOR_BASE];
        else if ((minor >= DEV_FM_TX_PORTS_MINOR_BASE) && (minor < DEV_FM_MAX_MINORS))
            p_LnxWrpFmPortDev = &p_LnxWrpFmDev->txPorts[minor-DEV_FM_TX_PORTS_MINOR_BASE];
        else
            return -EINVAL;

        /* if trying to open port, check if it initialized */
        if (!p_LnxWrpFmPortDev->initialized)
            return -ENODEV;

        p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev *)fm_port_bind(p_LnxWrpFmPortDev->dev);
        file->private_data = p_LnxWrpFmPortDev;
        fm_unbind((struct fm *)p_LnxWrpFmDev);
    }

    if (file->private_data == NULL)
         return -ENXIO;

    return 0;
}

static int fm_close(struct inode *inode, struct file *file)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev;
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev;
    unsigned int        minor = iminor(inode);
    int                 err = 0;

    DBG(TRACE, ("Closing minor - %d - ", minor));

    if ((minor == DEV_FM_MINOR_BASE) ||
        (minor == DEV_FM_PCD_MINOR_BASE))
    {
        p_LnxWrpFmDev = (t_LnxWrpFmDev*)file->private_data;
        if (!p_LnxWrpFmDev)
            return -ENODEV;
        fm_unbind((struct fm *)p_LnxWrpFmDev);
    }
    else if (((minor >= DEV_FM_OH_PORTS_MINOR_BASE) && (minor < DEV_FM_RX_PORTS_MINOR_BASE)) ||
             ((minor >= DEV_FM_RX_PORTS_MINOR_BASE) && (minor < DEV_FM_TX_PORTS_MINOR_BASE)) ||
             ((minor >= DEV_FM_TX_PORTS_MINOR_BASE) && (minor < DEV_FM_MAX_MINORS)))
    {
        p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)file->private_data;
        if (!p_LnxWrpFmPortDev)
            return -ENODEV;
        fm_port_unbind((struct fm_port *)p_LnxWrpFmPortDev);
    }

    return err;
}

static int fm_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int        minor = iminor(inode);

    DBG(TRACE, ("IOCTL minor - %d, cmd - 0x%08x, arg - 0x%08x", minor, cmd, arg));

    if ((minor == DEV_FM_MINOR_BASE) ||
        (minor == DEV_FM_PCD_MINOR_BASE))
    {
        t_LnxWrpFmDev *p_LnxWrpFmDev = ((t_LnxWrpFmDev*)file->private_data);
        if (!p_LnxWrpFmDev)
            return -ENODEV;
        if (LnxwrpFmIOCTL(p_LnxWrpFmDev, cmd, arg))
            return -EFAULT;
    }
    else if (((minor >= DEV_FM_OH_PORTS_MINOR_BASE) && (minor < DEV_FM_RX_PORTS_MINOR_BASE)) ||
             ((minor >= DEV_FM_RX_PORTS_MINOR_BASE) && (minor < DEV_FM_TX_PORTS_MINOR_BASE)) ||
             ((minor >= DEV_FM_TX_PORTS_MINOR_BASE) && (minor < DEV_FM_MAX_MINORS)))
    {
        t_LnxWrpFmPortDev *p_LnxWrpFmPortDev = ((t_LnxWrpFmPortDev*)file->private_data);
        if (!p_LnxWrpFmPortDev)
            return -ENODEV;
        if (LnxwrpFmPortIOCTL(p_LnxWrpFmPortDev, cmd, arg))
            return -EFAULT;
    }
    else
    {
        REPORT_ERROR(MINOR, E_INVALID_VALUE, ("minor"));
        return -ENODEV;
    }

    return 0;
}

/* Globals for FM character device */
static struct file_operations fm_fops =
{
    owner:      THIS_MODULE,
    ioctl:      fm_ioctl,
    open:       fm_open,
    release:    fm_close,
};


#ifndef NO_OF_SUPPORT
static int /*__devinit*/ fm_probe(struct of_device *of_dev, const struct of_device_id *match)
{
    t_LnxWrpFmDev   *p_LnxWrpFmDev;

    if ((p_LnxWrpFmDev = ReadFmDevTreeNode(of_dev)) == NULL)
        return -EIO;
    if (ConfigureFmDev(p_LnxWrpFmDev) != E_OK)
        return -EIO;
    if (InitFmDev(p_LnxWrpFmDev) != E_OK)
        return -EIO;

    Sprint (p_LnxWrpFmDev->name, "%s%d", DEV_FM_NAME, p_LnxWrpFmDev->id);

    /* Register to the /dev for IOCTL API */
    /* Register dynamically a new major number for the character device: */
    if ((p_LnxWrpFmDev->major = register_chrdev(0, p_LnxWrpFmDev->name, &fm_fops)) <= 0) {
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("Failed to allocate a major number for device \"%s\"", p_LnxWrpFmDev->name));
        return -EIO;
    }

    /* Creating classes for FM */
    DBG(TRACE ,("class_create fm_class"));
    p_LnxWrpFmDev->fm_class = class_create(THIS_MODULE, p_LnxWrpFmDev->name);
    if (IS_ERR(p_LnxWrpFmDev->fm_class)) {
        unregister_chrdev(p_LnxWrpFmDev->major, p_LnxWrpFmDev->name);
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("class_create error fm_class"));
        return -EIO;
    }

    device_create(p_LnxWrpFmDev->fm_class, NULL, MKDEV(p_LnxWrpFmDev->major, DEV_FM_MINOR_BASE), NULL,
                  "fm%d", p_LnxWrpFmDev->id);
    device_create(p_LnxWrpFmDev->fm_class, NULL, MKDEV(p_LnxWrpFmDev->major, DEV_FM_PCD_MINOR_BASE), NULL,
                  "fm%d-pcd", p_LnxWrpFmDev->id);

    /* Register to the /proc for debug and statistics API */
    if (((p_LnxWrpFmDev->proc_fm = proc_mkdir(p_LnxWrpFmDev->name, NULL)) == NULL) ||
        ((p_LnxWrpFmDev->proc_fm_regs = create_proc_read_entry("regs", 0, p_LnxWrpFmDev->proc_fm, fm_proc_dump_regs, p_LnxWrpFmDev)) == NULL) ||
        ((p_LnxWrpFmDev->proc_fm_stats = create_proc_read_entry("stats", 0, p_LnxWrpFmDev->proc_fm, fm_proc_dump_stats, p_LnxWrpFmDev)) == NULL) ||
        ((p_LnxWrpFmDev->proc_fm_pcd = proc_mkdir("fm-pcd", p_LnxWrpFmDev->proc_fm)) == NULL) ||
        ((p_LnxWrpFmDev->proc_fm_pcd_regs = create_proc_read_entry("regs", 0, p_LnxWrpFmDev->proc_fm_pcd, fm_pcd_proc_dump_regs, p_LnxWrpFmDev)) == NULL) ||
        ((p_LnxWrpFmDev->proc_fm_pcd_stats = create_proc_read_entry("stats", 0, p_LnxWrpFmDev->proc_fm_pcd, fm_pcd_proc_dump_stats, p_LnxWrpFmDev)) == NULL)
        )
    {
        FreeFmDev(p_LnxWrpFmDev);
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("Unable to create proc entry - fm!!!"));
        return -EIO;
    }

    dev_set_drvdata(p_LnxWrpFmDev->dev, p_LnxWrpFmDev);

    DBG(TRACE, ("FM%d probed", p_LnxWrpFmDev->id));

    return 0;
}

static int __devexit fm_remove(struct of_device *of_dev)
{
    t_LnxWrpFmDev   *p_LnxWrpFmDev;
    struct device   *dev;

    dev = &of_dev->dev;
    p_LnxWrpFmDev = dev_get_drvdata(dev);

    remove_proc_entry("stats", p_LnxWrpFmDev->proc_fm_pcd);
    remove_proc_entry("regs", p_LnxWrpFmDev->proc_fm_pcd);
    remove_proc_entry("fm-pcd", p_LnxWrpFmDev->proc_fm);
    remove_proc_entry("stats", p_LnxWrpFmDev->proc_fm);
    remove_proc_entry("regs", p_LnxWrpFmDev->proc_fm);
    remove_proc_entry(p_LnxWrpFmDev->name, NULL);

    DBG(TRACE, ("destroy fm_class"));
    device_destroy(p_LnxWrpFmDev->fm_class, MKDEV(p_LnxWrpFmDev->major, DEV_FM_MINOR_BASE));
    device_destroy(p_LnxWrpFmDev->fm_class, MKDEV(p_LnxWrpFmDev->major, DEV_FM_PCD_MINOR_BASE));
    class_destroy(p_LnxWrpFmDev->fm_class);

    /* Destroy chardev */
    unregister_chrdev(p_LnxWrpFmDev->major, p_LnxWrpFmDev->name);

    FreeFmDev(p_LnxWrpFmDev);

    DistroyFmDev(p_LnxWrpFmDev);

    dev_set_drvdata(dev, NULL);

    return 0;
}

static const struct of_device_id fm_match[] __devinitconst = {
    {
        .compatible    = "fsl,fman"
    },
    {}
};
#ifndef MODULE
MODULE_DEVICE_TABLE(of, fm_match);
#endif /* !MODULE */

static struct of_platform_driver fm_driver = {
    .name           = "fsl-fman",
    .match_table    = fm_match,
    .owner          = THIS_MODULE,
    .probe          = fm_probe,
    .remove         = __devexit_p(fm_remove)
};

static int /*__devinit*/ fm_port_probe(struct of_device *of_dev, const struct of_device_id *match)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev;
    t_LnxWrpFmDev       *p_LnxWrpFmDev;
    struct device       *dev;

    dev = &of_dev->dev;

    if ((p_LnxWrpFmPortDev = ReadFmPortDevTreeNode(of_dev)) == NULL)
        return -EIO;

    if (ConfigureFmPortDev(p_LnxWrpFmPortDev) != E_OK)
        return -EIO;

#if 0
    if ((p_LnxWrpFmPortDev->settings.param.portType != e_FM_PORT_TYPE_RX_10G) &&
        (p_LnxWrpFmPortDev->settings.param.portType != e_FM_PORT_TYPE_RX) &&
        (p_LnxWrpFmPortDev->settings.param.portType != e_FM_PORT_TYPE_TX_10G) &&
        (p_LnxWrpFmPortDev->settings.param.portType != e_FM_PORT_TYPE_TX) &&
        (InitFmPortDev(p_LnxWrpFmPortDev) != E_OK))
        return -EIO;
#endif /* 0 */

    dev_set_drvdata(dev, p_LnxWrpFmPortDev);

    if ((p_LnxWrpFmPortDev->settings.param.portType == e_FM_PORT_TYPE_OH_HOST_COMMAND) &&
        (InitFmPcdDev((t_LnxWrpFmDev *)p_LnxWrpFmPortDev->h_LnxWrpFmDev) != E_OK))
        return -EIO;

    p_LnxWrpFmDev = (t_LnxWrpFmDev *)p_LnxWrpFmPortDev->h_LnxWrpFmDev;

    if (p_LnxWrpFmPortDev->settings.param.portType == e_FM_PORT_TYPE_RX)
    {
        Sprint (p_LnxWrpFmPortDev->name, "%s-port-rx%d", p_LnxWrpFmDev->name, p_LnxWrpFmPortDev->id);
        p_LnxWrpFmPortDev->minor = p_LnxWrpFmPortDev->id + DEV_FM_RX_PORTS_MINOR_BASE;
    }
    else if (p_LnxWrpFmPortDev->settings.param.portType == e_FM_PORT_TYPE_RX_10G)
    {
        Sprint (p_LnxWrpFmPortDev->name, "%s-port-rx%d", p_LnxWrpFmDev->name, p_LnxWrpFmPortDev->id+FM_MAX_NUM_OF_1G_RX_PORTS);
        p_LnxWrpFmPortDev->minor = p_LnxWrpFmPortDev->id + FM_MAX_NUM_OF_1G_RX_PORTS + DEV_FM_RX_PORTS_MINOR_BASE;
    }
    else if (p_LnxWrpFmPortDev->settings.param.portType == e_FM_PORT_TYPE_TX)
    {
        Sprint (p_LnxWrpFmPortDev->name, "%s-port-tx%d", p_LnxWrpFmDev->name, p_LnxWrpFmPortDev->id);
        p_LnxWrpFmPortDev->minor = p_LnxWrpFmPortDev->id + DEV_FM_TX_PORTS_MINOR_BASE;
    }
    else if (p_LnxWrpFmPortDev->settings.param.portType == e_FM_PORT_TYPE_TX_10G)
    {
        Sprint (p_LnxWrpFmPortDev->name, "%s-port-tx%d", p_LnxWrpFmDev->name, p_LnxWrpFmPortDev->id+FM_MAX_NUM_OF_1G_TX_PORTS);
        p_LnxWrpFmPortDev->minor = p_LnxWrpFmPortDev->id + FM_MAX_NUM_OF_1G_TX_PORTS + DEV_FM_TX_PORTS_MINOR_BASE;
    }
    else if (p_LnxWrpFmPortDev->settings.param.portType == e_FM_PORT_TYPE_OH_HOST_COMMAND)
    {
        Sprint (p_LnxWrpFmPortDev->name, "%s-port-oh%d", p_LnxWrpFmDev->name, p_LnxWrpFmPortDev->id);
        p_LnxWrpFmPortDev->minor = p_LnxWrpFmPortDev->id + DEV_FM_OH_PORTS_MINOR_BASE;
    }
    else if (p_LnxWrpFmPortDev->settings.param.portType == e_FM_PORT_TYPE_OH_OFFLINE_PARSING)
    {
        Sprint (p_LnxWrpFmPortDev->name, "%s-port-oh%d", p_LnxWrpFmDev->name, p_LnxWrpFmPortDev->id+1);
        p_LnxWrpFmPortDev->minor = p_LnxWrpFmPortDev->id + 1 + DEV_FM_OH_PORTS_MINOR_BASE;
    }

    device_create(p_LnxWrpFmDev->fm_class, NULL, MKDEV(p_LnxWrpFmDev->major, p_LnxWrpFmPortDev->minor), NULL, p_LnxWrpFmPortDev->name);

    /* Register to the /proc for debug and statistics API */
    if (((p_LnxWrpFmPortDev->proc = proc_mkdir(p_LnxWrpFmPortDev->name, p_LnxWrpFmDev->proc_fm)) == NULL) ||
        ((p_LnxWrpFmPortDev->proc_regs = create_proc_read_entry("regs", 0, p_LnxWrpFmPortDev->proc, fm_port_proc_dump_regs, p_LnxWrpFmPortDev)) == NULL) ||
        ((p_LnxWrpFmPortDev->proc_stats = create_proc_read_entry("stats", 0, p_LnxWrpFmPortDev->proc, fm_port_proc_dump_stats, p_LnxWrpFmPortDev)) == NULL)
        )
    {
        FreeFmDev(p_LnxWrpFmDev);
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("Unable to create proc entry - fm!!!"));
        return -EIO;
    }

    DBG(TRACE, ("%s probed", p_LnxWrpFmPortDev->name));

    return 0;
}

static int __devexit fm_port_remove(struct of_device *of_dev)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev;
    t_LnxWrpFmDev       *p_LnxWrpFmDev;
    struct device       *dev;

    dev = &of_dev->dev;
    p_LnxWrpFmPortDev = dev_get_drvdata(dev);

    remove_proc_entry("stats", p_LnxWrpFmPortDev->proc_stats);
    remove_proc_entry("regs", p_LnxWrpFmPortDev->proc_regs);
    remove_proc_entry(p_LnxWrpFmPortDev->name, p_LnxWrpFmPortDev->proc);

    p_LnxWrpFmDev = (t_LnxWrpFmDev *)p_LnxWrpFmPortDev->h_LnxWrpFmDev;
    device_destroy(p_LnxWrpFmDev->fm_class, MKDEV(p_LnxWrpFmDev->major, p_LnxWrpFmPortDev->minor));

    FreeFmPortDev(p_LnxWrpFmPortDev);

    dev_set_drvdata(dev, NULL);

    return 0;
}

static const struct of_device_id fm_port_match[] __devinitconst = {
    {
        .compatible    = "fsl,fman-port-oh"
    },
    {
        .compatible    = "fsl,fman-port-1g-rx"
    },
    {
        .compatible    = "fsl,fman-port-10g-rx"
    },
    {
        .compatible    = "fsl,fman-port-1g-tx"
    },
    {
        .compatible    = "fsl,fman-port-10g-tx"
    },
    {}
};
#ifndef MODULE
MODULE_DEVICE_TABLE(of, fm_port_match);
#endif /* !MODULE */

static struct of_platform_driver fm_port_driver = {
    .name           = "fsl-fman-port",
    .match_table    = fm_port_match,
    .owner          = THIS_MODULE,
    .probe          = fm_port_probe,
    .remove         = __devexit_p(fm_port_remove)
};
#endif /* !NO_OF_SUPPORT */


t_Handle LNXWRP_FM_Init(void)
{
#ifdef NO_OF_SUPPORT
    t_LnxWrpFmDev   *p_LnxWrpFmDev;
    int             i, j;
#endif /* NO_OF_SUPPORT */

    memset(&lnxWrpFm, 0, sizeof(lnxWrpFm));

#ifdef NO_OF_SUPPORT
    for (i=0; i<INTG_MAX_NUM_OF_FM; i++)
    {
        p_LnxWrpFmDev = CreateFmDev(i);
        if (!p_LnxWrpFmDev)
        {
            REPORT_ERROR(MAJOR, E_INVALID_HANDLE, ("FM dev obj!"));
            return NULL;
        }
        lnxWrpFm.p_FmDevs[i] = p_LnxWrpFmDev;

        if (i==1)
        {
            ConfigureFmDev(p_LnxWrpFmDev);
            if (InitFmDev(p_LnxWrpFmDev) != E_OK)
                return NULL;

            Sprint (p_LnxWrpFmDev->name, "%s%d", DEV_FM_NAME, p_LnxWrpFmDev->id);

            /* Register to the /dev for IOCTL API */
            /* Register dynamically a new major number for the character device: */
            if ((p_LnxWrpFmDev->major = register_chrdev(0, p_LnxWrpFmDev->name, &fm_fops)) <= 0)
            {
                REPORT_ERROR(MAJOR, E_INVALID_STATE, ("Failed to allocate a major number for device \"%s\"", p_LnxWrpFmDev->name));
                return NULL;
            }

            /* Register to the /proc for debug and statistics API */
            if (((p_LnxWrpFmDev->proc_fm = proc_mkdir(p_LnxWrpFmDev->name, NULL)) == NULL) ||
                ((p_LnxWrpFmDev->proc_fm_regs = create_proc_read_entry("regs", 0, p_LnxWrpFmDev->proc_fm, fm_proc_dump_regs, p_LnxWrpFmDev)) == NULL) ||
                ((p_LnxWrpFmDev->proc_fm_stats = create_proc_read_entry("stats", 0, p_LnxWrpFmDev->proc_fm, fm_proc_dump_stats, p_LnxWrpFmDev)) == NULL))
            {
                REPORT_ERROR(MAJOR, E_INVALID_STATE, ("Unable to create proc entry - fm!!!"));
                return NULL;
            }
        }

//        lnxWrpFm.h_Mod = p_TdmLnxWrpParam->h_Mod;
//        lnxWrpFm.f_GetObject = p_TdmLnxWrpParam->f_GetObject;
    }

#else
    /* Register to the DTB for basic FM API */
    of_register_platform_driver(&fm_driver);
    /* Register to the DTB for basic FM port API */
    of_register_platform_driver(&fm_port_driver);
#endif /* !NO_OF_SUPPORT */

#ifdef CONFIG_FSL_FMAN_TEST
    /* Seed the QMan allocator so we'll have enough queues to run PCD with
       dinamically fqid-range allocation */
    qman_release_fqid_range(0x100, 0x100);
#endif /* CONFIG_FSL_FMAN_TEST */

    return &lnxWrpFm;
}

t_Error LNXWRP_FM_Free(t_Handle h_LnxWrpFm)
{
#ifdef NO_OF_SUPPORT
    t_LnxWrpFm          *p_LnxWrpFm = (t_LnxWrpFm *)h_LnxWrpFm;
    t_LnxWrpFmDev       *p_LnxWrpFmDev;
    int                 i, j;

    for (i=0; i<INTG_MAX_NUM_OF_FM; i++)
    {
        p_LnxWrpFmDev = p_LnxWrpFm->p_FmDevs[i];

        remove_proc_entry("stats", p_LnxWrpFmDev->proc_fm);
        remove_proc_entry("regs", p_LnxWrpFmDev->proc_fm);
        remove_proc_entry(p_LnxWrpFmDev->name, NULL);

        /* Destroy chardev */
        unregister_chrdev(p_LnxWrpFmDev->major, p_LnxWrpFmDev->name);

        FreeFmDev(p_LnxWrpFmDev);

        DistroyFmDev(p_LnxWrpFmDev);
    }

#else
        of_unregister_platform_driver(&fm_port_driver);
        of_unregister_platform_driver(&fm_driver);
#endif /* NO_OF_SUPPORT */

    return E_OK;
}


struct fm * fm_bind (struct device *fm_dev)
{
    return (struct fm *)(dev_get_drvdata(get_device(fm_dev)));
}
EXPORT_SYMBOL(fm_bind);

void fm_unbind(struct fm *fm)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev*)fm;

    put_device(p_LnxWrpFmDev->dev);
}
EXPORT_SYMBOL(fm_unbind);

struct resource * fm_get_mem_region(struct fm *fm)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev*)fm;

    return p_LnxWrpFmDev->res;
}
EXPORT_SYMBOL(fm_get_mem_region);

void * fm_get_handle(struct fm *fm)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev*)fm;

    return (void *)p_LnxWrpFmDev->h_Dev;
}
EXPORT_SYMBOL(fm_get_handle);

struct fm_port * fm_port_bind (struct device *fm_port_dev)
{
    return (struct fm_port *)(dev_get_drvdata(get_device(fm_port_dev)));
}
EXPORT_SYMBOL(fm_port_bind);

void fm_port_unbind(struct fm_port *port)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)port;

    put_device(p_LnxWrpFmPortDev->dev);
}
EXPORT_SYMBOL(fm_port_unbind);

void * fm_port_get_handle(struct fm_port *port)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)port;

    return (void *)p_LnxWrpFmPortDev->h_Dev;
}
EXPORT_SYMBOL(fm_port_get_handle);

void fm_set_rx_port_params(struct fm_port *port, struct fm_port_rx_params *params)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)port;
    int                 i;

    p_LnxWrpFmPortDev->settings.param.specificParams.rxParams.errFqid  = params->errq;
    p_LnxWrpFmPortDev->settings.param.specificParams.rxParams.dfltFqid = params->defq;

    p_LnxWrpFmPortDev->settings.param.specificParams.rxParams.rxExtBufPools.numOfPoolsUsed = params->num_pools;
    for (i=0; i<params->num_pools; i++)
    {
        p_LnxWrpFmPortDev->settings.param.specificParams.rxParams.rxExtBufPools.rxExtBufPool[i].id =
            params->pool_param[i].id;
        p_LnxWrpFmPortDev->settings.param.specificParams.rxParams.rxExtBufPools.rxExtBufPool[i].size =
            params->pool_param[i].size;
    }

    p_LnxWrpFmPortDev->buffPrefixContent.privDataSize     = params->priv_data_size;
    p_LnxWrpFmPortDev->buffPrefixContent.passPrsResult    = params->parse_results;
    p_LnxWrpFmPortDev->buffPrefixContent.passHashResult   = params->hash_results;

    ADD_ADV_CONFIG_START(p_LnxWrpFmPortDev->settings.advConfig, FM_MAX_NUM_OF_ADV_SETTINGS)

    ADD_ADV_CONFIG_NO_RET(FM_PORT_ConfigBufferPrefixContent,   ARGS(1, (&p_LnxWrpFmPortDev->buffPrefixContent)));

    ADD_ADV_CONFIG_END

    InitFmPortDev(p_LnxWrpFmPortDev);
}
EXPORT_SYMBOL(fm_set_rx_port_params);

void fm_port_pcd_bind (struct fm_port *port, struct fm_port_pcd_param *params)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)port;

    p_LnxWrpFmPortDev->pcd_owner_params.cb      = params->cb;
    p_LnxWrpFmPortDev->pcd_owner_params.dev     = params->dev;
}
EXPORT_SYMBOL(fm_port_pcd_bind);

void fm_set_tx_port_params(struct fm_port *port, struct fm_port_non_rx_params *params)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)port;

    p_LnxWrpFmPortDev->settings.param.specificParams.nonRxParams.errFqid  = params->errq;
    p_LnxWrpFmPortDev->settings.param.specificParams.nonRxParams.dfltFqid = params->defq;

    p_LnxWrpFmPortDev->buffPrefixContent.privDataSize     = params->priv_data_size;
    p_LnxWrpFmPortDev->buffPrefixContent.passPrsResult    = params->parse_results;
    p_LnxWrpFmPortDev->buffPrefixContent.passHashResult   = params->hash_results;

    ADD_ADV_CONFIG_START(p_LnxWrpFmPortDev->settings.advConfig, FM_MAX_NUM_OF_ADV_SETTINGS)

    ADD_ADV_CONFIG_NO_RET(FM_PORT_ConfigBufferPrefixContent,   ARGS(1, (&p_LnxWrpFmPortDev->buffPrefixContent)));

    ADD_ADV_CONFIG_END

    InitFmPortDev(p_LnxWrpFmPortDev);
}
EXPORT_SYMBOL(fm_set_tx_port_params);

int fm_get_tx_port_channel(struct fm_port *port)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)port;

    return p_LnxWrpFmPortDev->txCh;
}
EXPORT_SYMBOL(fm_get_tx_port_channel);

int fm_port_enable (struct fm_port *port)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)port;

    FM_PORT_Enable(p_LnxWrpFmPortDev->h_Dev);

    return 0;
}
EXPORT_SYMBOL(fm_port_enable);

void fm_port_disable(struct fm_port *port)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)port;

    FM_PORT_Disable(p_LnxWrpFmPortDev->h_Dev);
}
EXPORT_SYMBOL(fm_port_disable);
