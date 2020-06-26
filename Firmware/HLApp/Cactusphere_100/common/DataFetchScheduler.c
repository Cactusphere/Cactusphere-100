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

#include "DataFetchScheduler.h"

#include <string.h>

#include "LibCloud.h"
#include "StringBuf.h"
#include "TelemetryItems.h"

extern bool	IsAuthenticationDone(void);

static DataFetchSchedulerBase*	sPrimaryScheduler = NULL;

// Default implementation of virtual method
static void
DataFetchSchedulerBase_DoDestroy(DataFetchSchedulerBase* me)
{
    // do nothing
}

static void
DataFetchSchedulerBase_DoInit(
    DataFetchSchedulerBase* me, vector fetchItemPtrs)
{
    // do nothing
}

static void
DataFetchSchedulerBase_ClearFetchTargets(DataFetchSchedulerBase* me)
{
    // do nothing
}

static void
DataFetchSchedulerBase_DoSchedule(DataFetchSchedulerBase* me)
{
    // do nothing
}

// Initialization and cleanup
void
DataFetchScheduler_Init(DataFetchScheduler* me, vector fetchItemPtrs)
{
    // initialize the generalized/base class's member and  
    // do for specialized/derived class
    FetchTimers_Init(me->mFetchTimers, fetchItemPtrs);
    TelemetryItems_Clear(me->mTelemetryItems);
    StringBuf_Clear(me->mStringBuf);

    me->DoInit((DataFetchSchedulerBase*)me, fetchItemPtrs);

    // set first instance as primary
    if (NULL == sPrimaryScheduler) {
        sPrimaryScheduler = me;
    }
}

void
DataFetchScheduler_Destroy(DataFetchScheduler* me)
{
    // cleanup member of specialized class and generalized class
    me->DoDestroy(me);

    StringBuf_Destroy(me->mStringBuf);
    TelemetryItems_Destroy(me->mTelemetryItems);
    FetchTimers_Destroy(me->mFetchTimers);

    free(me);
}

// Periodic operation (per 1[sec])
void
DataFetchScheduler_Schedule(DataFetchScheduler* me)
{
    // Do data acquisition by specialized class and send it as telemetry.
    // If nettwork is down, store the acquired data to cache and send it after recovery. 
    const char* telemtryStr;

    me->ClearFetchTargets(me);
    TelemetryItems_Clear(me->mTelemetryItems);
    StringBuf_Clear(me->mStringBuf);

    FetchTimers_UpdateTimers(me->mFetchTimers);

    me->DoSchedule(me);

    telemtryStr = TelemetryItems_ToJson(me->mTelemetryItems);
    if (0 != strcmp(telemtryStr, "{}")) {
        bool	isNetworkAlive = IoT_CentralLib_CheckConnection();
        uint32_t	timeStamp = IoT_CentralLib_GetTmeStamp();

        if (! IsAuthenticationDone()) {
            isNetworkAlive = false;
        }

        if (isNetworkAlive) {
            if (IoT_CentralLib_HasCachedTelemetryItems()) {  // send cached data first
                if (me == sPrimaryScheduler
                && !IoT_CentralLib_ResendCachedTelemetryItems()) {
                    // !!error
                }
                if (IoT_CentralLib_HasCachedTelemetryItems()) {
                    goto do_cache;   // still outstanding cache, so append new data
                }
            }

            if (! IoT_CentralLib_SendTelemetry(telemtryStr, &timeStamp)) {
                isNetworkAlive = IoT_CentralLib_CheckConnection();
                if (isNetworkAlive) {
                    // !!error
                }
            }
        }

        if (! isNetworkAlive) {
do_cache:
            if (! IoT_CentralLib_EnqueueTelemtryItemsToCache(me->mTelemetryItems,
                    timeStamp)) {
                // failed to caching; Error!
            }
        }
        TelemetryItems_Clear(me->mTelemetryItems);
    }
}

// For specialized class
DataFetchSchedulerBase*
DataFetchScheduler_InitOnNew(DataFetchSchedulerBase* me,
    FetchTimerCallback ftCallback, IO_Feature feature)
{
    // initialize generalized class's member
    me->mFetchTimers = Factory_CreateFetchTimers(feature, ftCallback, me);
    if (NULL == me->mFetchTimers) {
        goto err;
    }
    me->mTelemetryItems = TelemetryItems_New();
    if (NULL == me->mFetchTimers) {
        goto err_delete_fetchTimers;
    }
    me->mStringBuf = StringBuf_New();
    if (NULL == me->mStringBuf) {
        goto err_delete_telemetryItems;
    }
    me->DoDestroy         = DataFetchSchedulerBase_DoDestroy;
    me->DoInit            = DataFetchSchedulerBase_DoInit;
    me->ClearFetchTargets = DataFetchSchedulerBase_ClearFetchTargets;
    me->DoSchedule        = DataFetchSchedulerBase_DoSchedule;

    return me;
err_delete_telemetryItems:
    TelemetryItems_Destroy(me->mTelemetryItems);
err_delete_fetchTimers:
    FetchTimers_Destroy(me->mFetchTimers);
err:
    free(me);
    return NULL;
}
