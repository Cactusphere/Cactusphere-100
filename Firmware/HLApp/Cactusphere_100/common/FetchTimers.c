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

#include "FetchTimers.h"

#include <string.h>
#include <applibs/log.h>

typedef struct FetchTimerList {
    char        telemetryName[TELEMETRY_NAME_MAX_LEN + 1];  // telemetry name
    uint32_t    downCounter;
} FetchTimerList;

// Initialization
static void
FetchTimer_Init(FetchTimer* me, FetchItemBase* fi, uint32_t downCounter)
{
    if (fi->isTimerReset) {
        me->downCounter = fi->intervalSec;
        fi->isTimerReset = false;
    } else {
        me->downCounter = downCounter;
    }
    me->fetchItem   = fi;
}

// Initialization and cleanup
FetchTimers*
FetchTimers_New(FetchTimerCallback cbProc, void* cbArg)
{
    // initialize generalized class's member
    FetchTimers*	newObj = (FetchTimers*)malloc(sizeof(FetchTimers));

    if (NULL != newObj) {
        newObj->mBody = vector_init(sizeof(FetchTimer));
        if (NULL == newObj->mBody) {
            free(newObj);
            return NULL;
        }
        newObj->mCallbackProc = cbProc;
        newObj->mCbArg        = cbArg;
        newObj->InitForTimer = FetchTimers_IntiForTimer;
    }

    return newObj;
}

void
FetchTimers_Init(FetchTimers* me, vector fetchItemPtrs)
{
    // initialize the generalized/base class's member and do 
    // specialized/derived class specific timer related initialization
    FetchItemBase**	fetchItemCurs = vector_get_data(fetchItemPtrs);
    FetchTimerList* list = NULL;
    int listsize = vector_size(me->mBody);
    
    if (listsize > 0) {
        list = malloc( sizeof(FetchTimerList) * (size_t)listsize);
        if (list) {
            return;
        }
        memset(list, 0, sizeof(FetchTimerList) * (size_t)listsize);

        FetchTimer* curs = (FetchTimer*)vector_get_data(me->mBody);
        for (int i = 0; i < listsize; i++) {
            strcpy(list[i].telemetryName, curs->fetchItem->telemetryName);
            list[i].downCounter = curs->downCounter;
            curs++;
        }
    }

    vector_clear(me->mBody);
    for (int i = 0, n = vector_size(fetchItemPtrs); i < n; ++i) {
        FetchItemBase*	fetchItem = *fetchItemCurs++;
        FetchTimer	pseudo;
        uint32_t downCounter = 0;

        if (! fetchItem->isTimerReset) {
            if (list) {
                for (int j = 0; j < listsize; j ++) {
                    if (0 == strcmp(fetchItem->telemetryName, list[j].telemetryName)) {
                        downCounter = list[j].downCounter;
                    }
                }
            }
        }

        FetchTimer_Init(&pseudo, fetchItem, downCounter);
        vector_add_last(me->mBody, &pseudo);
        me->InitForTimer(me, fetchItem);  // specialized class specific
    }

    if (list) {
        free(list);
    }
}

void
FetchTimers_IntiForTimer(FetchTimers* me, FetchItemBase* fetchItem)
{
    // do nothing
}

void
FetchTimers_Destroy(FetchTimers* me)
{
    vector_destroy(me->mBody);
    free(me);
}

// Updating timer counters for periodic expiration
void
FetchTimers_UpdateTimers(FetchTimers* me)
{
    // decrement the timer's down counter and fire when it reaches 0
    FetchTimer*	timerCurs = vector_get_data(me->mBody);

    for (int i = 0, n = vector_size(me->mBody); i < n; ++i) {
        if (0 == --timerCurs->downCounter) {
            me->mCallbackProc(me->mCbArg, timerCurs->fetchItem);
            timerCurs->downCounter = timerCurs->fetchItem->intervalSec;  // reset
        }
        ++timerCurs;
    }
}
