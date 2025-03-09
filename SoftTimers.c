/**
 * @file SoftTimers.c
 * @author Viacheslav (slava.k@ks2corp.com)
 * @brief Contains all macro definitions and function prototypes
 * support for handling software timers.
 *
 * @version 0.1
 * @date 2022-10-08
 *
 * @copyright Vicaheslav@mcublog.ru Copyright (c) 2022
 *
 */
#include <string.h>
#include "SoftTimers.h"

#ifndef MAX_TIMERS
#define MAX_TIMERS                      (5)
#endif

#define TIMER_ACTIVE_MASK               (0x8000)
#define TIMER_CREATED_MASK              (0x4000)
#define TIMER_CREATED_AND_ACTIVE_AMSK   (0xC000)
#define TIMER_TIMEOUT_MASK              (0x3FFF)

static stimer_t TimerTable[MAX_TIMERS] = {0};
static stimer_init_ctx_t TimerInitCtx = {0};

void Timer_Init(const stimer_init_ctx_t *init_ctx)
{
    memset(&TimerTable, 0, sizeof(TimerTable));
    memcpy(&TimerInitCtx, init_ctx, sizeof(stimer_init_ctx_t));
}

/**
 * @fn uint8_t Timer_Create(uint16_t timeout, func_pnt_t FunctionPointer)
 * @brief Create a software timer.
 * @param FunctionPointer The function to call on timeout.
 * @return The timer id or the error code ERR_TIMER_NOT_AVAILABLE.
 */
uint8_t Timer_Create(timer_handler_t FunctionPointer)
{
    uint8_t idx;

    idx = 0;
    while ((idx < MAX_TIMERS) && (TimerTable[idx].TimeoutTick & TIMER_CREATED_MASK))
    {
        idx++;
    }
    if (idx == MAX_TIMERS)
    {
        idx = ERR_TIMER_NOT_AVAILABLE;
    }
    else
    {
        /* Disable timer interrupt */
        TimerInitCtx.disable_irq();
        /* Initialize timer */
        TimerTable[idx].TimerHandler = FunctionPointer;
        TimerTable[idx].TimeoutTick = TIMER_CREATED_MASK;
        /* Enable timer interrupt */
        TimerInitCtx.enable_irq();
    }
    return idx;
}

/**
 * @fn void Timer_Destroy(uint8_t timerId)
 * @brief Destroy the timer passed as parameter
 * @param timerId The id of the timer to destroy.
 */
void Timer_Destroy(uint8_t timerId)
{
    if (timerId < MAX_TIMERS)
    {
        TimerTable[timerId].TimeoutTick = 0;
    }
}

/**
 * @fn void Timer_Start(uint8_t timerId)
 * @brief Start the timer.
 * @param timerId The id of the timer to start.
 * @param timeout The timeout in tens of milliseconds. Max timeout value is 16383 (14 bits available).
 */
void Timer_Start(uint8_t timerId, uint16_t timeout)
{
    if (timerId < MAX_TIMERS)
    {
        /* Disable timer interrupt */
        TimerInitCtx.disable_irq();
        /* Set the timer active flag */
        TimerTable[timerId].TimeoutTick |= (TIMER_ACTIVE_MASK + (timeout & TIMER_TIMEOUT_MASK));
        /* Enable timer interrupt */
        TimerInitCtx.enable_irq();
    }
}

/**
 * @fn void Timer_Stop(uint8_t timerId)
 * @brief Stop the timer.
 * @param timerId The id of the timer to stop.
 */
void Timer_Stop(uint8_t timerId)
{
    if (timerId < MAX_TIMERS)
    {
        /* Disable timer interrupt */
        TimerInitCtx.disable_irq();
        /* Reset the timer active flag */
        TimerTable[timerId].TimeoutTick &= ~TIMER_ACTIVE_MASK;
        /* Enable timer interrupt */
        TimerInitCtx.enable_irq();
    }
}

/**
 * @fn void Timer_Update(void)
 * @brief Update active timers. Note that it's called in interrupt contest.
 */
void Timer_Update(void)
{
    uint8_t idx;

    for (idx=0; idx<MAX_TIMERS; idx++)
    {
        if ((TimerTable[idx].TimeoutTick & TIMER_CREATED_AND_ACTIVE_AMSK) == TIMER_CREATED_AND_ACTIVE_AMSK)
        {
            TimerTable[idx].TimeoutTick--;
            if ((TimerTable[idx].TimeoutTick & TIMER_TIMEOUT_MASK) == 0)
            {
                /* Disable the timer */
                TimerTable[idx].TimeoutTick &= ~TIMER_ACTIVE_MASK;

                /* Call the timer handler */
                TimerTable[idx].TimerHandler(idx);
            }
        }
    }
}
