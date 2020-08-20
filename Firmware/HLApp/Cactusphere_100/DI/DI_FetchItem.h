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

#ifndef _DI_FETCH_ITEM_H_
#define _DI_FETCH_ITEM_H_

#ifndef _FETCH_ITEM_BASE_H_
#include <FetchItemBase.h>
#endif

#ifndef _STDBOOL_H
#include <stdbool.h>
#endif

typedef struct DI_FetchItem {
    char        telemetryName[TELEMETRY_NAME_MAX_LEN + 1];  // telemetry name
    uint32_t    intervalSec;    // periodic acquisition interval (in seconds)
    bool        isTimerReset;   // flag of reset the timer
    uint32_t    pinID;          // pin ID
    bool        isPulseCounter; // pulse counter(true) / polling(false)
    bool        isCountClear;   // whether to clear the counter
    bool        isPulseHigh;    // whether settlement as pulse when high(:1) or low(:0) level
    uint32_t    minPulseWidth;  // minimum length for settlement as pulse
    uint32_t    maxPulseCount;  // max pulse counter value
} DI_FetchItem;

#endif  // _DI_FETCH_ITEM_H
