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

#ifndef _DATA_FETCH_SCHEDULER_H_
#define _DATA_FETCH_SCHEDULER_H_

#ifndef CONTAINERS_VECTOR_H
#include <vector.h>
#endif

#ifndef _FACTORY_H_
#include <Factory.h>
#endif

// forward declaration
typedef struct DataFetchSchedulerBase	DataFetchSchedulerBase;
typedef struct FetchTimers	FetchTimers;
typedef struct StringBuf	StringBuf;
typedef struct TelemetryItems	TelemetryItems;

// DataFetchSchedulerBase class's virtual methods and data mebers
struct DataFetchSchedulerBase {
// virtual method
    void	(*DoDestroy)(DataFetchSchedulerBase* me);
    void	(*DoInit)(DataFetchSchedulerBase* me, vector fetchItemPtrs);
    void	(*ClearFetchTargets)(DataFetchSchedulerBase* me);
    void	(*DoSchedule)(DataFetchSchedulerBase* me);

// data member
    FetchTimers*    mFetchTimers;       // timers for data acquistion
    TelemetryItems* mTelemetryItems;    // vector of telemetry item
    StringBuf*      mStringBuf;         // for string processing
};

// alias type
typedef DataFetchSchedulerBase	DataFetchScheduler;

// Initialization and cleanup
extern void	DataFetchScheduler_Init(
    DataFetchScheduler* me, vector fetchItemPtrs);
extern void	DataFetchScheduler_Destroy(DataFetchScheduler* me);

// Deriodic operation (per 1[sec])
extern void	DataFetchScheduler_Schedule(DataFetchScheduler* me);

// For specialized class
extern DataFetchSchedulerBase*	DataFetchScheduler_InitOnNew(
    DataFetchSchedulerBase* me,
    FetchTimerCallback ftCallback, IO_Feature feature);

#endif  // _DATA_FETCH_SCHEDULER_H_
