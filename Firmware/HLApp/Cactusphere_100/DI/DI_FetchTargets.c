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

#include "DI_FetchTargets.h"

#include "DI_FetchItem.h"

// DI_FetchTargets data members
struct DI_FetchTargets {
    vector	mTargets;
};

// Initialization and cleanup
DI_FetchTargets*
DI_FetchTargets_New(void)
{
    DI_FetchTargets*	newObj =
        (DI_FetchTargets*)malloc(sizeof(DI_FetchTargets));

    if (NULL != newObj) {
        newObj->mTargets = vector_init(sizeof(DI_FetchItem*));
        if (NULL == newObj->mTargets) {
            free(newObj);
            return NULL;
        }
    }

    return newObj;
}

void
DI_FetchTargets_Destroy(DI_FetchTargets* me)
{
    DI_FetchTargets_Clear(me);
    vector_destroy(me->mTargets);
    free(me);
}

// Get current acquisition targets
vector
DI_FetchTargets_GetFetchItems(DI_FetchTargets* me)
{
    return me->mTargets;
}

// Manage acquisition targets
void
DI_FetchTargets_Add(DI_FetchTargets* me, const DI_FetchItem* target)
{
    vector_add_last(me->mTargets, (void*)&target);
}

void
DI_FetchTargets_Clear(DI_FetchTargets* me)
{
    vector_clear(me->mTargets);
}
