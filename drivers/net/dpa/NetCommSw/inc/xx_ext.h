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
 @File          xx_ext.h

 @Description   Prototypes, externals and typedefs for system-supplied
                (external) routines
*//***************************************************************************/

#ifndef __XX_EXT_H
#define __XX_EXT_H

#include "std_ext.h"
#include "part_ext.h"

#if defined(__MWERKS__) && defined(OPTIMIZED_FOR_SPEED)
#include "xx_integration_ext.h"
#endif /* defined(__MWERKS__) && defined(OPTIMIZED_FOR_SPEED) */

/**************************************************************************//**
 @Group         xx_id  XX Interface (System call hooks)

 @Description   Prototypes, externals and typedefs for system-supplied
                (external) routines

 @{
*//***************************************************************************/

#if (defined(REPORT_EVENTS) && (REPORT_EVENTS > 0))
/**************************************************************************//**
 @Function      XX_EventById

 @Description   Event reporting routine - executed only when REPORT_EVENTS=1.

 @Param[in]     event - Event code (e_Event).
 @Param[in]     appId - Application identifier.
 @Param[in]     flags - Event flags.
 @Param[in]     msg   - Event message.

 @Return        None
*//***************************************************************************/
void XX_EventById(uint32_t event, t_Handle appId, uint16_t flags, char *msg);

#else  /* not REPORT_EVENTS */
#define XX_EventById(event, appId, flags, msg)
#endif /* REPORT_EVENTS */



#ifdef DEBUG_XX_MALLOC

void * XX_MallocDebug(uint32_t size, char *fname, int line);

void * XX_MallocSmartDebug(uint32_t size,
                           int      memPartitionId,
                           uint32_t align,
                           char     *fname,
                           int      line);

#define XX_Malloc(sz) \
    XX_MallocDebug((sz), __FILE__, __LINE__)

#define XX_MallocSmart(sz, memt, al) \
    XX_MallocSmartDebug((sz), (memt), (al), __FILE__, __LINE__)

#else /* not DEBUG_XX_MALLOC */

/**************************************************************************//**
 @Function      XX_Malloc

 @Description   allocates contiguous block of memory.

 @Param[in]     size - Number of bytes to allocate.

 @Return        The address of the newly allocated block on success, NULL on failure.
*//***************************************************************************/
void * XX_Malloc(uint32_t size);

/**************************************************************************//**
 @Function      XX_MallocSmart

 @Description   Allocates contiguous block of memory in a specified
                alignment and from the specified segment.

 @Param[in]     size            - Number of bytes to allocate.
 @Param[in]     memPartitionId  - Memory partition ID; The value zero must
                                  be mapped to the default heap partition.
 @Param[in]     alignment       - Required memory alignment.

 @Return        The address of the newly allocated block on success, NULL on failure.
*//***************************************************************************/
void * XX_MallocSmart(uint32_t size, int memPartitionId, uint32_t alignment);

#endif /* not DEBUG_XX_MALLOC */

/**************************************************************************//**
 @Function      XX_FreeSmart

 @Description   Frees the memory block pointed to by "p".
                Only for memory allocated by XX_MallocSmart

 @Param[in]     p_Memory - pointer to the memory block.

 @Return        None.
*//***************************************************************************/
void XX_FreeSmart(void *p_Memory);

/**************************************************************************//**
 @Function      XX_Free

 @Description   frees the memory block pointed to by "p".

 @Param[in]     p_Memory - pointer to the memory block.

 @Return        None.
*//***************************************************************************/
void XX_Free(void *p_Memory);


/**************************************************************************//**
 @Function      XX_GetPartitionBase

 @Description   This routine gets the address of a memory segment according to
                the memory type.

 @Param[in]     memPartitionId  - Memory partition ID; The value zero must
                                  be mapped to the default heap partition.

 @Return        The address of the required memory type.
*//***************************************************************************/
void * XX_GetMemPartitionBase(int memPartitionId);

/**************************************************************************//**
 @Function      XX_Print

 @Description   print a string.

 @Param[in]     str - string to print.

 @Return        None.
*//***************************************************************************/
void    XX_Print(char *str, ...);

/**************************************************************************//**
 @Function      XX_GetChar

 @Description   Get character from console.

 @Return        Character is returned on success. Zero is returned otherwise.
*//***************************************************************************/
char    XX_GetChar(void);

/**************************************************************************//**
 @Function      XX_SetIntr

 @Description   Set an interupt service routine for a specific interrupt source.

 @Param[in]     irq     - Interrupt ID (system-specific number).
 @Param[in]     f_Isr   - Callback routine that will be called when the interupt occurs.
 @Param[in]     handle  - The argument for the user callback routine.

 @Return        E_OK on success; error code otherwise..
*//***************************************************************************/
t_Error XX_SetIntr(int irq, t_Isr *f_Isr, t_Handle handle);

/**************************************************************************//**
 @Function      XX_FreeIntr

 @Description   Free a specific interrupt and a specific callback routine.

 @Param[in]     irq - Interrupt ID (system-specific number).

 @Return        E_OK on success; error code otherwise..
*//***************************************************************************/
t_Error XX_FreeIntr(int irq);

/**************************************************************************//**
 @Function      XX_EnableIntr

 @Description   Enable a specific interrupt.

 @Param[in]     irq - Interrupt ID (system-specific number).

 @Return        E_OK on success; error code otherwise..
*//***************************************************************************/
t_Error XX_EnableIntr(int irq);

/**************************************************************************//**
 @Function      XX_DisableIntr

 @Description   Disable a specific interrupt.

 @Param[in]     irq - Interrupt ID (system-specific number).

 @Return        E_OK on success; error code otherwise..
*//***************************************************************************/
t_Error XX_DisableIntr(int irq);

#if !(defined(__MWERKS__) && defined(OPTIMIZED_FOR_SPEED))
/**************************************************************************//**
 @Function      XX_DisableAllIntr

 @Description   Disable interrupts by writing to MSR register at the CPU.

 @Return        intMASK a value that represent the interurupt mask before operation
*//***************************************************************************/
uint32_t XX_DisableAllIntr(void);

/**************************************************************************//**
 @Function      XX_RestoreAllIntr

 @Description   Enable interrupts by writing to MSR register at the CPU.

 @Param[in]     flags           - intMASK, mask of previos level function will set the mask based on it.

 @Return        None.
*//***************************************************************************/
void XX_RestoreAllIntr(uint32_t flags);
#endif /* !(defined(__MWERKS__) && defined(OPTIMIZED_FOR_SPEED)) */

/**************************************************************************//**
 @Function      XX_Call

 @Description   Call a service in another task.

                Activate the routine “f” via the queue identified by “IntrManagerId”. The
                parameter to “f” is Id - the handle of the destination object

 @Param[in]     intrManagerId   - Queue ID.
 @Param[in]     f               - routine pointer.
 @Param[in]     Id              - the parameter to be passed to f().
 @Param[in]     h_App           - Application handle.
 @Param[in]     flags           - Unused,

 @Return        E_OK is returned on success. E_FAIL is returned otherwise (usually an operating system level failure).
*//***************************************************************************/
t_Error XX_Call( uint32_t intrManagerId,
                 t_Error (* f)(t_Handle),
                 t_Handle Id,
                 t_Handle h_App,
                 uint16_t flags );

#ifndef BOOT_SEQ
/**************************************************************************//**
 @Function      XX_Exit

 @Description   Stop execution and report status (where it is applicable)

 @Param[in]     status - exit status
*//***************************************************************************/
void    XX_Exit(int status);
#endif /* BOOT_SEQ */

/*****************************************************************************/
/*                        Tasklet Service Routines                           */
/*****************************************************************************/
typedef t_Handle t_TaskletHandle;

/**************************************************************************//**
 @Function      XX_InitTasklet

 @Description   Create and initialize a tasklet object.

 @Param[in]     routine - A routine to be ran as a tasklet.
 @Param[in]     data    - An argument to pass to the tasklet.

 @Return        Tasklet handle is returned on success. NULL is returned otherwise.
*//***************************************************************************/
t_TaskletHandle XX_InitTasklet (void (*routine)(void *), void *data);

/**************************************************************************//**
 @Function      XX_FreeTasklet

 @Description   Free a tasklet object.

 @Param[in]     h_Tasklet - A handle to a tasklet to be free.

 @Return        None.
*//***************************************************************************/
void XX_FreeTasklet (t_TaskletHandle h_Tasklet);

/**************************************************************************//**
 @Function      XX_ScheduleTask

 @Description   Schedule a tasklet object.

 @Param[in]     h_Tasklet - A handle to a tasklet to be schedulded.
 @Param[in]     immediate - Indicate whether to schedule this tasklet on
                            the immediate queue or on the delayed one.

 @Return        0 - on success. Error code - otherwise.
*//***************************************************************************/
int XX_ScheduleTask(t_TaskletHandle h_Tasklet, int immediate);

/**************************************************************************//**
 @Function      XX_FlushScheduledTasks

 @Description   Flush all tasks there are in the scheduled tasks queue.

 @Return        None.
*//***************************************************************************/
void XX_FlushScheduledTasks(void);

/**************************************************************************//**
 @Function      XX_TaskletIsQueued

 @Description   Check if task is queued.

 @Param[in]     h_Tasklet - A handle to a tasklet to be schedulded.

 @Return        1 - task is queued. 0 - otherwise.
*//***************************************************************************/
int XX_TaskletIsQueued(t_TaskletHandle h_Tasklet);

/**************************************************************************//**
 @Function      XX_SetTaskletData

 @Description   Set data to a scheduleded task. Used to change data of allready
                scheduled task.

 @Param[in]     h_Tasklet - A handle to a tasklet to be schedulded.
 @Param[in]     data      - Data to be set.
*//***************************************************************************/
void XX_SetTaskletData(t_TaskletHandle h_Tasklet, t_Handle data);

/**************************************************************************//**
 @Function      XX_GetTaskletData

 @Description   Get the data of scheduled task.

 @Param[in]     h_Tasklet - A handle to a tasklet to be schedulded.

 @Return        handle to the data of the task.
*//***************************************************************************/
t_Handle XX_GetTaskletData(t_TaskletHandle h_Tasklet);

/**************************************************************************//**
 @Function      XX_BottomHalf

 @Description   Bottom half implementation, invoked by the interrupt handler.

                This routine handles all bottom-half tasklets with interrupts
                enabled.

 @Return        None.
*//***************************************************************************/
void XX_BottomHalf(void);


/*****************************************************************************/
/*                        Semaphore Service Routines                         */
/*****************************************************************************/
typedef t_Handle t_SemaphoreHandle;
typedef t_Handle t_MutexHandle;

/**************************************************************************//**
 @Function      XX_InitSemaphore

 @Description   Creates a counting semaphore.

 @Param[in]     initialCount - initial counter value.

 @Return        Semaphore handle is returned on success. NULL is returned
                otherwise.
*//***************************************************************************/
t_SemaphoreHandle XX_InitSemaphore(int initialCount);

/**************************************************************************//**
 @Function      XX_InitBinSemaphore

 @Description   creates a binary semaphore, which is a semaphore with maximum
                count of 1.

 @Param[in]     initialCount - initial counter value, should be of course
                               either 0 or 1.

 @Return        Semaphore handle is returned on success. NULL is returned
                otherwise.
*//***************************************************************************/
t_SemaphoreHandle XX_InitBinSemaphore(int initialCount);

/**************************************************************************//**
 @Function      XX_FreeSemaphore

 @Description   Frees the memory allocated for the semaphore creation.

 @Param[in]     h_Semaphore - A handle to a semaphore.

 @Return        None.
*//***************************************************************************/
void XX_FreeSemaphore(t_SemaphoreHandle h_Semaphore);

/**************************************************************************//**
 @Function      XX_InitMutex

 @Description   Creates a mutex.

 @Return        Mutex handle is returned on success. NULL is returned
                otherwise.
*//***************************************************************************/
t_MutexHandle XX_InitMutex(void);

/**************************************************************************//**
 @Function      XX_InitMutexLocked

 @Description   Creates a mutex and locks it.

 @Return        Mutex handle is returned on success; NULL otherwise.
*//***************************************************************************/
t_MutexHandle XX_InitMutexLocked(void);

/**************************************************************************//**
 @Function      XX_FreeMutex

 @Description   Frees the memory allocated for the mutex creation.

 @Param[in]     h_Mutex - A handle to a mutex.

 @Return        None.
*//***************************************************************************/
void XX_FreeMutex(t_MutexHandle h_Mutex);

/**************************************************************************//**
 @Function      XX_DownSemaphore

 @Description   Decrements the counter of the semaphore by 1; When counter
                reaches 0, the semaphore is locked.

 @Param[in]     h_Semaphore - A handle to a semaphore.

 @Return        None.
*//***************************************************************************/
void XX_DownSemaphore(t_SemaphoreHandle h_Semaphore);

/**************************************************************************//**
 @Function      XX_UpSemaphore

 @Description   Increments the counter of the semaphore by 1; When counter
                passes 0, the semaphore is unlocked.

 @Param[in]     h_Semaphore - A handle to a semaphore.

 @Return        None.
*//***************************************************************************/
void XX_UpSemaphore(t_SemaphoreHandle h_Semaphore);

/**************************************************************************//**
 @Function      XX_TryLock

 @Description   Tries to lock a mutex.

 @Param[in]     h_Mutex - A handle to a mutex.

 @Return        1 - on success. 0 - otherwise.
*//***************************************************************************/
int XX_TryLock(t_MutexHandle h_Mutex);

/**************************************************************************//**
 @Function      XX_Lock

 @Description   Locks a mutex.

 @Param[in]     h_Mutex - A handle to a mutex.

 @Return        None.
*//***************************************************************************/
void XX_Lock(t_MutexHandle h_Mutex);

/**************************************************************************//**
 @Function      XX_Unlock

 @Description   Unlocks a mutex.

 @Param[in]     h_Mutex - A handle to a mutex.

 @Return        None.
*//***************************************************************************/
void XX_Unlock(t_MutexHandle h_Mutex);


/*****************************************************************************/
/*                        Spinlock Service Routines                          */
/*****************************************************************************/
typedef t_Handle t_SpinlockHandle;

/**************************************************************************//**
 @Function      XX_InitSpinlock

 @Description   Creates a spinlock.

 @Return        Spinlock handle is returned on success; NULL otherwise.
*//***************************************************************************/
t_SpinlockHandle  XX_InitSpinlock(void);

/**************************************************************************//**
 @Function      XX_FreeSpinlock

 @Description   Frees the memory allocated for the spinlock creation.

 @Param[in]     h_Spinlock - A handle to a spinlock.

 @Return        None.
*//***************************************************************************/
void XX_FreeSpinlock(t_SpinlockHandle h_Spinlock);

/**************************************************************************//**
 @Function      XX_Spinlock

 @Description   Locks a spinlock.

 @Param[in]     h_Spinlock - A handle to a spinlock.

 @Return        None.
*//***************************************************************************/
void XX_Spinlock(t_SpinlockHandle h_Spinlock);

/**************************************************************************//**
 @Function      XX_Spinunlock

 @Description   Unlocks a spinlock.

 @Param[in]     h_Spinlock - A handle to a spinlock.

 @Return        None.
*//***************************************************************************/
void XX_Spinunlock(t_SpinlockHandle h_Spinlock);

/**************************************************************************//**
 @Function      XX_IntrSpinlock

 @Description   Locks a spinlock (interrupt safe).

 @Param[in]     h_Spinlock - A handle to a spinlock.

 @Return        None.
*//***************************************************************************/
void XX_IntrSpinlock(t_SpinlockHandle h_Spinlock);

/**************************************************************************//**
 @Function      XX_IntrSpinunlock

 @Description   Unlocks a spinlock (interrupt safe).

 @Param[in]     h_Spinlock - A handle to a spinlock.

 @Return        None.
*//***************************************************************************/
void XX_IntrSpinunlock(t_SpinlockHandle h_Spinlock);


/*****************************************************************************/
/*                        Timers Service Routines                            */
/*****************************************************************************/
typedef t_Handle t_TimerHandle;

/**************************************************************************//**
 @Function      XX_CurrentTime

 @Description   Returns current system time.

 @Return        Current system time (in milliseconds).
*//***************************************************************************/
uint32_t XX_CurrentTime(void);

/**************************************************************************//**
 @Function      XX_CreateTimer

 @Description   Creates a timer.

 @Return        Timer handle is returned on success; NULL otherwise.
*//***************************************************************************/
t_TimerHandle XX_CreateTimer(void);

/**************************************************************************//**
 @Function      XX_FreeTimer

 @Description   Frees the memory allocated for the timer creation.

 @Param[in]     h_Timer - A handle to a timer.

 @Return        None.
*//***************************************************************************/
void XX_FreeTimer(t_TimerHandle h_Timer);

/**************************************************************************//**
 @Function      XX_StartTimer

 @Description   Starts a timer.

                The user can select to start the timer as periodic timer or as
                one-shot timer. The user should provide a callback routine that
                will be called when the timer expires.

 @Param[in]     h_Timer         - A handle to a timer.
 @Param[in]     msecs           - Timer expiration period (in milliseconds).
 @Param[in]     periodic        - TRUE for a periodic timer;
                                  FALSE for a one-shot timer..
 @Param[in]     f_TimerExpired  - A callback routine to be called when the
                                  timer expires.
 @Param[in]     h_Arg           - The argument to pass in the timer-expired
                                  callback routine.

 @Return        None.
*//***************************************************************************/
void XX_StartTimer(t_TimerHandle    h_Timer,
                   uint32_t         msecs,
                   bool             periodic,
                   void             (*f_TimerExpired)(t_Handle h_Arg),
                   t_Handle         h_Arg);

/**************************************************************************//**
 @Function      XX_StopTimer

 @Description   Frees the memory allocated for the timer creation.

 @Param[in]     h_Timer - A handle to a timer.

 @Return        None.
*//***************************************************************************/
void XX_StopTimer(t_TimerHandle h_Timer);

/**************************************************************************//**
 @Function      XX_GetExpirationTime

 @Description   Returns the time (in milliseconds) remaining until the
                expiration of a timer.

 @Param[in]     h_Timer - A handle to a timer.

 @Return        The time left until the timer expires.
*//***************************************************************************/
uint32_t XX_GetExpirationTime(t_TimerHandle h_Timer);

/**************************************************************************//**
 @Function      XX_ModTimer

 @Description   Updates the expiration time of a timer.

                This routine adds the given time to the current system time,
                and sets this value as the new expiration time of the timer.

 @Param[in]     h_Timer - A handle to a timer.
 @Param[in]     msecs   - The new interval until timer expiration
                          (in milliseconds).

 @Return        None.
*//***************************************************************************/
void XX_ModTimer(t_TimerHandle h_Timer, uint32_t msecs);

/**************************************************************************//**
 @Function      XX_TimerIsActive

 @Description   Checks whether a timer is active (pending) or not.

 @Param[in]     h_Timer - A handle to a timer.

 @Return        0 - the timer is inactive; Non-zero value - the timer is active;
*//***************************************************************************/
int XX_TimerIsActive(t_TimerHandle h_Timer);

/**************************************************************************//**
 @Function      XX_Sleep

 @Description   Non-busy wait until the desired time (in milliseconds) has passed.

 @Param[in]     msecs - The requested sleep time (in milliseconds).

 @Return        None.

 @Cautions      This routine enables interrupts during its wait time.
*//***************************************************************************/
uint32_t XX_Sleep(uint32_t msecs);

/**************************************************************************//**
 @Function      XX_UDelay

 @Description   Busy-wait until the desired time (in microseconds) has passed.

 @Param[in]     usecs - The requested delay time (in microseconds).

 @Return        None.

 @Cautions      It is highly unrecommended to call this routine during interrupt
                time, because the system time may not be updated properly during
                the delay loop. The behavior of this routine during interrupt
                time is unexpected.
*//***************************************************************************/
void XX_UDelay(uint32_t usecs);


/*****************************************************************************/
/*                         Other Service Routines                            */
/*****************************************************************************/

/**************************************************************************//**
 @Function      XX_PhysToVirt

 @Description   Translates a physical address to the matching virtual address.

 @Param[in]     addr - The physical address to translate.

 @Return        Virtual address.
*//***************************************************************************/
void * XX_PhysToVirt(void *addr);

/**************************************************************************//**
 @Function      XX_VirtToPhys

 @Description   Translates a virtual address to the matching physical address.

 @Param[in]     addr - The virtual address to translate.

 @Return        Physical address.
*//***************************************************************************/
void * XX_VirtToPhys(void *addr);

#define XXX_PhysToVirt(addr)  (CAST_POINTER_TO_UINT32(XX_PhysToVirt(CAST_UINT32_TO_POINTER(addr))))
#define XXX_VirtToPhys(addr)  (CAST_POINTER_TO_UINT32(XX_VirtToPhys(CAST_UINT32_TO_POINTER(addr))))


#define MSG_BODY_SIZE       512
#ifdef CONFIG_MULTI_PARTITION_SUPPORT
/**************************************************************************//**
 @Group         xx_ipc  XX Inter-Partition-Communication API

 @Description   The following API is to be used when working with multi-core.

 @{
*//***************************************************************************/
typedef void (t_MsgCompletionCB) (t_Handle h_Arg, uint8_t msgBody[MSG_BODY_SIZE]);
typedef t_Error (t_MsgHandler) (t_Handle h_Mod, uint32_t msgId, uint8_t msgBody[MSG_BODY_SIZE]);

/**************************************************************************//**
 @Function      XX_RegisterMessageHandler

 @Description   This routine is used to register to the XX messaging mechanism.

 @Param[in]     p_Addr          - The module address.
 @Param[in]     f_MsgHandlerCB  - The module callback; It will be called when the
                                  module gets message.
 @Param[in]     h_Mod           - The module arg; It will be passed to the module
                                  whithin the callback when it gets message.

 @Return        E_OK is returned on success. error code is returned otherwise.
*//***************************************************************************/
t_Error XX_RegisterMessageHandler   (char *p_Addr, t_MsgHandler *f_MsgHandlerCB, t_Handle h_Mod);

/**************************************************************************//**
 @Function      XX_UnregisterMessageHandler

 @Description   This routine is used to unregister to the XX messaging mechanism.

 @Param[in]     p_Addr          - The module address.

 @Return        E_OK is returned on success. error code is returned otherwise.
*//***************************************************************************/
t_Error XX_UnregisterMessageHandler (char *p_Addr);

/**************************************************************************//**
 @Function      XX_SendMessage

 @Description   This routine is used to send messages to modules registered to the
                XX messaging mechanism.

 @Param[in]     p_DestAddr      - The destination module address.
 @Param[in]     msgId           - The message ID.
 @Param[in]     msgBody         - The message body (up to 64 bytes).
 @Param[in]     f_CompletionCB  - The caller callback; It will be called after the
                                  message will be handled by the destination module.
                                  NOTE - if passing here NULL, the routine will be
                                  blocking untill the destination module will
                                  handle the message.
 @Param[in]     h_CBArg         - The caller handler; This argument will be
                                  passed to whithin the completion routine.

 @Return        E_OK is returned on success. error code is returned otherwise.
*//***************************************************************************/
t_Error XX_SendMessage(char                 *p_DestAddr,
                       uint32_t             msgId,
                       uint8_t              msgBody[MSG_BODY_SIZE],
                       t_MsgCompletionCB    *f_CompletionCB,
                       t_Handle             h_CBArg);
/** @} */ /* end of xx_ic group */
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
/** @} */ /* end of xx_id group */


#endif /* __XX_EXT_H */
