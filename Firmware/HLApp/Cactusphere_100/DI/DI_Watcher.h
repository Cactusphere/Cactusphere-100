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

#ifndef _DI_WATCHER_H_
#define _DI_WATCHER_H_

#ifndef _STDBOOL_H
#include <stdbool.h>
#endif

#ifndef CONTAINERS_VECTOR_H
#include <vector.h>
#endif

typedef struct DI_WatchItem	DI_WatchItem;
typedef struct DI_Watcher	DI_Watcher;

// status of contact input monitoring target
typedef struct DI_WatchItemStat {
    const DI_WatchItem*	watchItem;  // watching specification
    unsigned long	prevPulseCount; // previous counter value
    unsigned long	currPulseCount; // last counter value
} DI_WatchItemStat;

// Initialization and cleanup
extern DI_Watcher*	DI_Watcher_New(void);
extern void	DI_Watcher_Init(DI_Watcher* me, vector watchItems);
extern void	DI_Watcher_Destroy(DI_Watcher* me);

// Check update
extern bool	DI_Watcher_DoWatch(DI_Watcher* me);
extern const vector	DI_Watcher_GetLastChanges(DI_Watcher* me);

#endif  // _DI_WATCHER_H_
