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

#include "DI_DataFetchScheduler.h"

#include "DI_FetchItem.h"
#include "DI_FetchTargets.h"
#include "DI_Watcher.h"
#include "DI_WatchItem.h"
#include "LibCloud.h"
#include "LibDI.h"
#include "StringBuf.h"
#include "TelemetryItems.h"

typedef struct DI_DataFetchScheduler {
    DataFetchSchedulerBase	Super;

// data member
    DI_FetchTargets*    mFetchTargets;  // acquisition targets of pulse conter
    DI_Watcher*         mWatcher;       // contact input watch targets
} DI_DataFetchScheduler;

//
// DI_DataFetchScheduler's private procedure/method
//
// Callback procedure of FetchTimers
static void
DI_FetchTimerCallback(void* arg, const FetchItemBase* fetchTarget)
{
    // This procedure called against the acquisition target which  
    // timer expired
    DI_DataFetchScheduler* scheduler = (DI_DataFetchScheduler*)arg;

    DI_FetchTargets_Add(
        scheduler->mFetchTargets, (const DI_FetchItem*)fetchTarget);
}

// Virtual method
static void
DI_DataFetchScheduler_DoDestroy(DataFetchSchedulerBase* me)
{
    // cleanup own member
    DI_DataFetchScheduler* self = (DI_DataFetchScheduler*)me;

    DI_FetchTargets_Destroy(self->mFetchTargets);
    DI_Watcher_Destroy(self->mWatcher);
}

static void
DI_DataFetchScheduler_ClearFetchTargets(DataFetchSchedulerBase* me)
{
    DI_DataFetchScheduler* self = (DI_DataFetchScheduler*)me;

    DI_FetchTargets_Clear(self->mFetchTargets);
}

static void
DI_DataFetchScheduler_DoSchedule(DataFetchSchedulerBase* me)
{
    // acquire telemetry value from the pulse conter which timer expired and
    // the contact input which input signal changed
    DI_DataFetchScheduler* self = (DI_DataFetchScheduler*)me;
    vector	items;

    // pulse conters & polling
    items = DI_FetchTargets_GetFetchItems(self->mFetchTargets);
    if (! vector_is_empty(items)) {
        const DI_FetchItem** itemsCurs = (const DI_FetchItem**)vector_get_data(items);

        for (int i = 0, n = vector_size(items); i < n; i++) {
            const DI_FetchItem* item = *itemsCurs++;

            if (item->isPulseCounter) {
                unsigned long pulseCount = 0;

                if (! DI_Lib_ReadPulseCount(item->pinID, &pulseCount)) {
                    continue;
                };
                StringBuf_AppendByPrintf(me->mStringBuf, "%lu", pulseCount);
            } else {
                unsigned int currentStatus = 0;

                if (! DI_Lib_ReadPinLevel(item->pinID, &currentStatus)) {
                    continue;
                };
                StringBuf_AppendByPrintf(me->mStringBuf, "%ld", currentStatus);
            }

            TelemetryItems_Add(me->mTelemetryItems,
                item->telemetryName, StringBuf_GetStr(me->mStringBuf));
            StringBuf_Clear(me->mStringBuf);
        }
    }

    // contact inputs
    if (DI_Watcher_DoWatch(self->mWatcher)) {
        const vector	lastChanges = DI_Watcher_GetLastChanges(self->mWatcher);

        for (int i = 0, n = vector_size(lastChanges); i < n; ++i) {
            DI_WatchItemStat* wiStat;

            vector_get_at(&wiStat, lastChanges, i);

            StringBuf_AppendByPrintf(me->mStringBuf, "%ld", 1);
            TelemetryItems_Add(me->mTelemetryItems,
                wiStat->watchItem->telemetryName, StringBuf_GetStr(me->mStringBuf));
            StringBuf_Clear(me->mStringBuf);
        }
    }
}

DataFetchScheduler*
DI_DataFetchScheduler_New(void)
{
    // initialize own menber and setup virtual method
    DI_DataFetchScheduler* newObj =
        (DI_DataFetchScheduler*)malloc(sizeof(DI_DataFetchScheduler));
    DataFetchSchedulerBase* super;

    if (NULL == newObj) {
        goto err_malloc;
    }
    
    super = &newObj->Super;
    if (NULL == DataFetchScheduler_InitOnNew(
        super, DI_FetchTimerCallback, DIGITAL_IN)) {
        goto err;
    }
    newObj->mFetchTargets = DI_FetchTargets_New();
    if (NULL == newObj->mFetchTargets) {
        goto err_delete_super;
    }
    newObj->mWatcher = DI_Watcher_New();
    if (NULL == newObj->mWatcher) {
        goto err_delete_fetchTargets;
    }

    super->DoDestroy = DI_DataFetchScheduler_DoDestroy;
//	super->DoInit    = DI_DataFetchScheduler_DoInit;  // don't override
    super->ClearFetchTargets = DI_DataFetchScheduler_ClearFetchTargets;
    super->DoSchedule        = DI_DataFetchScheduler_DoSchedule;

    return super;
err_delete_fetchTargets:
    DI_FetchTargets_Destroy(newObj->mFetchTargets);
err_delete_super:
    DataFetchScheduler_Destroy(super);
err:
    free(newObj);
err_malloc:
    return NULL;
}

void
DI_DataFetchScheduler_Init(DataFetchScheduler* me,
    vector fetchItemPtrs, vector watchItems)
{
    // reinitialize pulse count acquisition and contact inpput monitoring targes
    DI_DataFetchScheduler* self = (DI_DataFetchScheduler*)me;

    DataFetchScheduler_Init(me, fetchItemPtrs);
    DI_Watcher_Init(self->mWatcher, watchItems);
}
