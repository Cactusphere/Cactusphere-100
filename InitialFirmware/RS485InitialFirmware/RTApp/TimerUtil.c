/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Atmark Techno, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "TimerUtil.h"

#include "mt3620-timer.h"

static uint32_t	sTickCount = 0;

static void
TimerCallback()
{
    sTickCount += 10;  // 10[ms/intr]
    Gpt_LaunchTimerMs(TimerGpt0, 10, TimerCallback);
}

// Initialization
bool
TimerUtil_Initialize()
{
    // interrupt setting at 10 [ms] cycle
    Gpt_Init();
    Gpt_LaunchTimerMs(TimerGpt0, 10, TimerCallback);

    return true;
}

// Sleep
void
TimerUtil_SleepUntilIntr()
{
    __asm__("wfi");
}

// tick count (count/msec)
uint32_t
TimerUtil_GetTickCount()
{
    return sTickCount;
}
