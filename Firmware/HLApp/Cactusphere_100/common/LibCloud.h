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

#ifndef _LIB_CLOUD_H_
#define _LIB_CLOUD_H_

#ifndef _STDBOOL
#include <stdbool.h>
#endif
#ifndef _STDINT_H
#include <stdint.h>
#endif

typedef struct TelemetryItems	TelemetryItems;

// Initialization and cleanup
extern bool IoT_CentralLib_Initialize(
    uint32_t cachBufSize, bool clearCache);
extern void IoT_CentralLib_Cleanup(void);

// Send telemetry data
extern bool	IoT_CentralLib_SendTelemetry(
    const char* jsonStr, uint32_t* outTimestamp);

// Telemetry data caching during network down
extern bool	IoT_CentralLib_CheckConnection(void);
extern bool	IoT_CentralLib_EnqueueTelemtryItemsToCache(
    const TelemetryItems* telemetryItems, uint32_t timeStamp);
extern bool	IoT_CentralLib_HasCachedTelemetryItems(void);
extern bool	IoT_CentralLib_ResendCachedTelemetryItems(void);
extern uint32_t	IoT_CentralLib_GetTmeStamp(void);

// Send property data
extern void IoT_CentralLib_SendProperty(const char* jsonStr);

#endif  // _LIB_CLOUD_H_
