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

#ifndef _MODBUS_FETCH_TARGETS_H_
#define _MODBUS_FETCH_TARGETS_H_

#ifndef CONTAINERS_VECTOR_H
#include "vector.h"
#endif

typedef struct ModbusFetchTargets	ModbusFetchTargets;
typedef struct ModbusFetchItem	ModbusFetchItem;

// Initialization and cleanup
extern ModbusFetchTargets*	ModbusFetchTargets_New(void);
extern void	ModbusFetchTargets_Destroy(ModbusFetchTargets* me);

// Get current acquisition targets
extern vector	ModbusFetchTargets_GetDevIDs(ModbusFetchTargets* me);
extern vector	ModbusFetchTargets_GetFetchItems(
    ModbusFetchTargets* me, unsigned long devID);

// Manage acquisition targets
extern void	ModbusFetchTargets_Add(
    ModbusFetchTargets* me, const ModbusFetchItem* target);
extern void	ModbusFetchTargets_Clear(ModbusFetchTargets* me);

#endif  // _MODBUS_FETCH_TARGETS_H_
