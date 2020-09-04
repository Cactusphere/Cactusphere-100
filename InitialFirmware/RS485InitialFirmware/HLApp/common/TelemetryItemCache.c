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

#include "TelemetryItemCache.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "TelemetryItems.h"

const char	MARKER_NAME[] = "___-___";

typedef struct TelemetryItemCache {
    TelemetryCacheElem* mRingBuf;	// ring buffer area
    unsigned char*      mOwnBuf;    // self allocated buffer area
    uint32_t	mBufSize;	// ring buffer size in cache element size unit
    uint32_t	mWritePos;	// write position index
    uint32_t	mReadPos;	// read position index
    uint32_t	mIndexMax;	// max value of index
} TelemetryItemCache;

static void
TelemetryItemCache_DiscardOldestCache(TelemetryItemCache* me)
{
    // skip until the second marker appears or reaches end
    const TelemetryCacheElem* cacheElem =
        me->mRingBuf + (me->mReadPos % me->mBufSize);

    if (0 == strcmp(cacheElem->itemName, MARKER_NAME)) {
        if (++(me->mReadPos) > me->mIndexMax) {
            me->mReadPos = 0;
        }
        cacheElem = me->mRingBuf + (me->mReadPos % me->mBufSize);
    }
    while (! TelemetryItemCache_IsEmpty(me)) {
        if (0 == strcmp(cacheElem->itemName, MARKER_NAME)) {
            break;
        }
        if (++(me->mReadPos) > me->mIndexMax) {
            me->mReadPos = 0;
        }
        cacheElem = me->mRingBuf + (me->mReadPos % me->mBufSize);
    }
}

static void
TelemetryItemCache_DestroyRingBuf(TelemetryItemCache* me)
{
    free(me->mOwnBuf);
    me->mOwnBuf = NULL;
}

// Initialization and cleanup
TelemetryItemCache*
TelemetryItemCache_New(void)
{
    TelemetryItemCache* newObj =
        (TelemetryItemCache*)malloc(sizeof(TelemetryItemCache));

    if (NULL != newObj) {
        newObj->mRingBuf    = NULL;
        newObj->mOwnBuf     = NULL;
        newObj->mBufSize    = 0;
        newObj->mWritePos   = newObj->mReadPos = 0;
        newObj->mIndexMax   = ULONG_MAX;
    }

    return newObj;
}

bool
TelemetryItemCache_Init(TelemetryItemCache* me,
    unsigned char* cacheBuf, uint32_t bufSize)
{
    // Setting up ring buffer with passed buffer area.
    // If buffer isn't passed (passed pointer is NULL), then allocate it.
    if (NULL != me->mOwnBuf) {
        TelemetryItemCache_DestroyRingBuf(me);
    }
    if (bufSize < sizeof(TelemetryCacheElem) * 10) {
        return false;  // invalid argument
    }
    if (NULL == cacheBuf) {
        cacheBuf = malloc(bufSize);
        if (NULL == cacheBuf) {
            return false;  // cannot alloc the buf
        }
        me->mOwnBuf = cacheBuf;
    }

    if (0 != ((uint32_t)cacheBuf & 0x3)) {
        // align if the passed area is not aligned on a 4 byte boundary
        uint32_t	mod = (uint32_t)cacheBuf & 0x3;

        cacheBuf += (4 - mod);
        bufSize  -= (4 - mod);
        if (bufSize < sizeof(TelemetryCacheElem) * 10) {
            if (NULL != me->mOwnBuf) {
                TelemetryItemCache_DestroyRingBuf(me);
            }
            return false;  // invalid argument (too small buf)
        }
    }
    me->mRingBuf  = (TelemetryCacheElem*)cacheBuf;
    me->mBufSize  = bufSize / sizeof(TelemetryCacheElem);
    me->mWritePos = me->mReadPos = 0;
    me->mIndexMax = ULONG_MAX - (ULONG_MAX % me->mBufSize);

    return true;
//
// NOTE: Here, it is assumed that the CPU is a 32-bit CPU, that is, 
//       the pointer size is 32 bits.
}

void
TelemetryItemCache_Destroy(TelemetryItemCache* me)
{
    if (NULL != me->mOwnBuf) {
        free(me->mOwnBuf);
    }
    free(me);
}

// Attribute
uint32_t
TelemetryItemCache_CountAvailItems(const TelemetryItemCache* me)
{
    uint32_t	numSpace = 0;

    if (ULONG_MAX == me->mIndexMax) {
        numSpace = (me->mWritePos - me->mReadPos);
    } else {
        if (me->mWritePos == me->mReadPos) {  // empty
            numSpace = me->mBufSize;
        } else if (me->mWritePos > me->mReadPos) {
            numSpace = (me->mWritePos - me->mReadPos);
        } else {
            numSpace = ((me->mIndexMax - me->mReadPos) + me->mWritePos);
        }
    }

    return (0 < numSpace ? (numSpace - 1) : 0);
//
// NOET: Subtract 1 for time stamp / separator markers
}

bool
TelemetryItemCache_IsEmpty(const TelemetryItemCache* me)
{
    return (me->mWritePos == me->mReadPos);
}

// Add and remove chace elem
bool
TelemetryItemCache_EnqueueItems(TelemetryItemCache* me,
    const TelemetryItems* items, uint32_t timeStamp)
{
    // Puts all passed telemetry data items into the ring buffer 
    // with a timestamp/separation marker at the beginning.
    // When there is no enough space left, discard old caches.
    TelemetryCacheElem* curs;

    while (TelemetryItemCache_CountAvailItems(me) < TelemetryItems_Count(items)) {
        if (TelemetryItemCache_IsEmpty(me)) {
            return false;  // too large items
        }
        TelemetryItemCache_DiscardOldestCache(me);
    }

    curs = me->mRingBuf + (me->mWritePos % me->mBufSize);
    curs->itemName = MARKER_NAME;
    curs->value.ul = timeStamp;
    if (++(me->mWritePos) > me->mIndexMax) {
        me->mWritePos = 0;
    }

    for (int i = 0, n = TelemetryItems_Count(items); i < n; ++i) {
        curs = me->mRingBuf + (me->mWritePos % me->mBufSize);
        TelemetryItems_ConvToCacheElemAt(items, i, curs);

        if (++(me->mWritePos) > me->mIndexMax) {
            me->mWritePos = 0;
        }
    }

    return true;
}

bool
TelemetryItemCache_DequeueItemsTo(TelemetryItemCache* me,
    TelemetryItems* outItems, uint32_t* outTimeStamp)
{
    // Retrieve a set of telemetry data items from the cache. Extract 
    // from the first time stamp / separation marker to the next one.
    const TelemetryCacheElem*	cacheElem;

    if (TelemetryItemCache_IsEmpty(me)) {
        return false;
    }

    TelemetryItems_Clear(outItems);
    *outTimeStamp = 0;

    cacheElem = me->mRingBuf + (me->mReadPos % me->mBufSize);
    if (0 != strcmp(cacheElem->itemName, MARKER_NAME)) {
        // skip until time stamp / separation marker appears
        do {
            if (++(me->mReadPos) > me->mIndexMax) {
                me->mReadPos = 0;
            }
            if (TelemetryItemCache_IsEmpty(me)) {
                return false;  // reached to end
            }
            cacheElem = me->mRingBuf + (me->mReadPos % me->mBufSize);
        } while (0 != strcmp(cacheElem->itemName, MARKER_NAME));
    }
    *outTimeStamp = cacheElem->value.ul;
    if (++(me->mReadPos) > me->mIndexMax) {
        me->mReadPos = 0;
    }

    while (! TelemetryItemCache_IsEmpty(me)) {
        cacheElem = me->mRingBuf + (me->mReadPos % me->mBufSize);
        if (0 == strcmp(cacheElem->itemName, MARKER_NAME)) {
            break;
        }
        TelemetryItems_AddFromCacheElem(outItems, cacheElem);
        if (++(me->mReadPos) > me->mIndexMax) {
            me->mReadPos = 0;
        }
    }

    return true;
}
