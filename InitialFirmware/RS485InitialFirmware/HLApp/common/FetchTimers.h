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

#ifndef _FETCH_TIMERS_H_
#define _FETCH_TIMERS_H_

#ifndef CONTAINERS_VECTOR_H
#include <vector.h>
#endif

#ifndef _FETCH_ITEM_BASE_H_
#include <FetchItemBase.h>
#endif

// timer for periodic data acquisition
typedef struct FetchTimer {
    const FetchItemBase* fetchItem;  // telemetry data acquisition spec
    uint32_t	downCounter;         // down counter for timer expiration
} FetchTimer;

// callback procedure for timer expiration notification
typedef void (*FetchTimerCallback)(
    void* arg, const FetchItemBase* fetchTarget);

typedef struct FetchTimers	FetchTimers;

// FetchTimers class's virtual methods and data members
struct FetchTimers {
// virtual method
    void (*InitForTimer)(FetchTimers* me, FetchItemBase* fetchItem);

// data member
    vector	mBody;                      // vector of timer
    FetchTimerCallback	mCallbackProc;  // timer expiration notifier
    void* mCbArg;                       // callback argument
};

// Initialization and cleanup
extern FetchTimers*	FetchTimers_New(FetchTimerCallback cbProc, void* cbArg);
extern void	FetchTimers_Init(FetchTimers* me, vector fetchItemPtrs);
extern void	FetchTimers_IntiForTimer(FetchTimers* me, FetchItemBase* fetchItem);
extern void	FetchTimers_Destroy(FetchTimers* me);

// Updating timer counters for periodic expiration
extern void	FetchTimers_UpdateTimers(FetchTimers* me);

#endif  // _FETCH_TIMERS_H_
