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

#ifndef _MODBUS_TCP_FETCH_TARGETS_H_
#define _MODBUS_TCP_FETCH_TARGETS_H_

#ifndef CONTAINERS_VECTOR_H
#include "vector.h"
#endif

typedef struct ModbusTcpFetchTargets	ModbusTcpFetchTargets;
typedef struct ModbusTcpFetchItem	ModbusTcpFetchItem;

// Initialization and cleanup
extern ModbusTcpFetchTargets*	ModbusTcpFetchTargets_New(void);
extern void	ModbusTcpFetchTargets_Destroy(ModbusTcpFetchTargets* me);

// Get current acquisition targets
extern vector	ModbusTcpFetchTargets_GetDevIDs(ModbusTcpFetchTargets* me);
extern vector	ModbusTcpFetchTargets_GetFetchItems(
    ModbusTcpFetchTargets* me, char* id);

// Manage acquisition targets
extern void	ModbusTcpFetchTargets_Add(
    ModbusTcpFetchTargets* me, const ModbusTcpFetchItem* target);
extern void	ModbusTcpFetchTargets_Clear(ModbusTcpFetchTargets* me);

#endif  // _MODBUS_TCP_FETCH_TARGETS_H_
