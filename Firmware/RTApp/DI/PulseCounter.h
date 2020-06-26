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

#ifndef _PULSE_COUNTER_H_
#define _PULSE_COUNTER_H_

#ifndef _STDBOOL_H
#include <stdbool.h>
#endif
#ifndef _STDINT_H
#include <stdint.h>
#endif

typedef struct PulseCounter {
    int         pinId;             // DIn pin number
    int         pulseCounter;      // pulse counter value
    int         pulseOnTime;       // time integration of pulse [msec]
    int         pulseOnTimeS;      // time integration of pulse [sec]
    int         pulseElapsedTime;  // current pulse's continuation length
    uint32_t    minPulseSetTime;   // minimum length for settlement as pulse
    uint32_t    maxPulseCounter;   // max pulse counter value
    bool        isCountHight;      // whether settlement as pulse when high(:1) or low(:0) level
    bool        prevState;         // previous state of the DIn pin
    bool        currentState;      // state of the DIn pin (After chattering control)
    bool        isSetPulse;        // is settlement have done
    bool        isRising;          // is DIn level rised
    bool        isStart;           // is this counter running
} PulseCounter;

// Initialization
extern void PulseCounter_Initialize(PulseCounter* me, int pin);

// Attribute
extern int  PulseCounter_GetPinId(PulseCounter* me);

// Pulse counter driver operation
extern void PulseCounter_SetConfigCounter(PulseCounter* me,
    bool isCountHigh, int minPulse, int maxPulse);
extern void PulseCounter_Clear(PulseCounter* me, int initValue);
extern int  PulseCounter_GetPulseCount(PulseCounter* me);
extern int  PulseCounter_GetPulseOnTime(PulseCounter* me);
extern bool PulseCounter_GetLevel(PulseCounter* me);
extern bool PulseCounter_GetPinLevel(PulseCounter* me);

// Handle polling based pulse counting task
extern void PulseCounter_Counter(PulseCounter* me);

#endif  // _PULSE_COUNTER_H_
