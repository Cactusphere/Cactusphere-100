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

#ifndef _TELEMETRYITEMS_H_
#define _TELEMETRYITEMS_H_

#ifndef _STDBOOL
#include <stdbool.h>
#endif

typedef struct TelemetryItems	TelemetryItems;
typedef struct TelemetryCacheElem	TelemetryCacheElem;

// Initialization and cleanup of the telemetry item data type dicitionary
extern void	TelemetryItems_InitDictionary(void);
extern void	TelemetryItems_CleanupDictionary(void);

// Add and remove telemetry item data type
extern void	TelemetryItems_AddDictionaryElem(
    const char* itemName, bool isFloat);
extern void	TelemetryItems_RemoveDictionaryElem(const char* itemName);

// Initialization and cleanup
extern TelemetryItems* TelemetryItems_New(void);
extern void TelemetryItems_Destroy(TelemetryItems* me);

// Attribute
extern int	TelemetryItems_Count(const TelemetryItems* me);

// Add and remove telemetry data item
extern void TelemetryItems_Add(
    TelemetryItems* me, const char* name, const char* value);
extern void TelemetryItems_Clear(TelemetryItems* me);

// Mutual conversion between cache elem
extern TelemetryCacheElem* TelemetryItems_ConvToCacheElemAt(
    const TelemetryItems* me, int index, TelemetryCacheElem* outCacheElem);
extern void	TelemetryItems_AddFromCacheElem(TelemetryItems* me,
    const TelemetryCacheElem* cacheElem);

// Convert to JSON text
extern const char* TelemetryItems_ToJson(TelemetryItems* me);

// Convert from JSON text
extern bool TelemetryItems_LoadFromJson(
    TelemetryItems* me, const char* jsonStr);

#endif  // _TELEMETRYITEMS_H_
