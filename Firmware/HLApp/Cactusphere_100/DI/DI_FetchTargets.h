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

#ifndef _DI_FETCH_TARGETS_H_
#define _DI_FETCH_TARGETS_H_

#ifndef CONTAINERS_VECTOR_H
#include <vector.h>
#endif

typedef struct DI_FetchTargets	DI_FetchTargets;
typedef struct DI_FetchItem	DI_FetchItem;

// Initialization and cleanup
extern DI_FetchTargets*	DI_FetchTargets_New(void);
extern void	DI_FetchTargets_Destroy(DI_FetchTargets* me);

// Get current acquisition targets
extern vector	DI_FetchTargets_GetFetchItems(DI_FetchTargets* me);

// Manage acquisition targets
extern void	DI_FetchTargets_Add(
    DI_FetchTargets* me, const DI_FetchItem* target);
extern void	DI_FetchTargets_Clear(DI_FetchTargets* me);

#endif  // _DI_FETCH_TARGETS_H_
