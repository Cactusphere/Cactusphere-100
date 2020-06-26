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

#ifndef _DI_WATCHITEM_H_
#define _DI_WATCHITEM_H_

#ifndef _STDBOOL_H
#include <stdbool.h>
#endif
#ifndef _STDINT_H
#include <stdint.h>
#endif

#ifndef TELEMETRY_NAME_MAX_LEN
#define TELEMETRY_NAME_MAX_LEN	32
#endif

typedef struct DI_WatchItem {
    char        telemetryName[TELEMETRY_NAME_MAX_LEN + 1];  // telemetry name
    uint32_t    pinID;                  // pin ID
    bool        notifyChangeForHigh;   // whether the input's normal level isn't high
    bool        isCountClear;          // whether to clear the counter
} DI_WatchItem;

#endif  // _DI_WATCHITEM_H_
