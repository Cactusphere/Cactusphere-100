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

#include "ModbusFetchTargets.h"

#include "ModbusFetchItem.h"
#include "dictionary.h"

typedef struct ModbusFetchItemsPerDev {
    unsigned long	mDevID;	// slave ID
    vector	mFetchItems;	// telemetry items
} ModbusFetchItemsPerDev;

static ModbusFetchItemsPerDev*	ModbusFetchItemsPerDev_New(unsigned long devID);
static void	ModbusFetchItemsPerDev_Destroy(ModbusFetchItemsPerDev* me);
static void	ModbusFetchItemsPerDev_Add(
    ModbusFetchItemsPerDev* me, const ModbusFetchItem* fetchItem);

// ModbusFetchTargets data members
struct ModbusFetchTargets {
    dictionary	mTargetsDictByDevID;
};

static ModbusFetchItemsPerDev*
ModbusFetchItemsPerDev_New(unsigned long devID)
{
    ModbusFetchItemsPerDev*	newObj =
        (ModbusFetchItemsPerDev*)malloc(sizeof(ModbusFetchItemsPerDev));

    if (NULL != newObj) {
        newObj->mDevID = devID;
        newObj->mFetchItems = vector_init(sizeof(ModbusFetchItem*));
        if (NULL == newObj->mFetchItems) {
            free(newObj);
            return newObj;
        }
    }

    return newObj;
}

static void
ModbusFetchItemsPerDev_Destroy(ModbusFetchItemsPerDev* me)
{
    vector_destroy(me->mFetchItems);
    free(me);
}

static void
ModbusFetchItemsPerDev_Add(
    ModbusFetchItemsPerDev* me, const ModbusFetchItem* fetchItem)
{
    vector_add_last(me->mFetchItems, (void*)&fetchItem);
}

static int
DevID_Comparator(const void* const one, const void* const two)
{
    unsigned long	devID1 = *((unsigned long*)one);
    unsigned long	devID2 = *((unsigned long*)two);

    if (devID1 < devID2) {
        return -1;
    } else if (devID1 > devID2) {
        return 1;
    } else {
        return 0;
    }
}

// Initialization and cleanup
ModbusFetchTargets*
ModbusFetchTargets_New(void)
{
    ModbusFetchTargets*	newObj =
        (ModbusFetchTargets*)malloc(sizeof(ModbusFetchTargets));

    if (NULL != newObj) {
        newObj->mTargetsDictByDevID = dictionary_init(
            sizeof(unsigned long), sizeof(ModbusFetchItemsPerDev*),
            DevID_Comparator);
        if (NULL == newObj->mTargetsDictByDevID) {
            free(newObj);
            return NULL;
        }
    }

    return newObj;
}

void
ModbusFetchTargets_Destroy(ModbusFetchTargets* me)
{
    vector	devIDs = ModbusFetchTargets_GetDevIDs(me);
    ModbusFetchItemsPerDev*	theGroup = NULL;

    ModbusFetchTargets_Clear(me);

    for (int i = 0, n = vector_size(devIDs); i < n; ++i) {
        unsigned long	devID;
        vector_get_at(&devID, devIDs, i);
        if (dictionary_get(&theGroup, me->mTargetsDictByDevID, &devID)) {
            ModbusFetchItemsPerDev_Destroy(theGroup);
        }
    }

    dictionary_destroy(me->mTargetsDictByDevID);
    free(me);
}

// Get current acquisition targets
vector
ModbusFetchTargets_GetDevIDs(ModbusFetchTargets* me)
{
    return dictionary_get_keys(me->mTargetsDictByDevID);
}

vector
ModbusFetchTargets_GetFetchItems(
    ModbusFetchTargets* me, unsigned long devID)
{
    ModbusFetchItemsPerDev*	theGroup = NULL;

    if (dictionary_get(&theGroup, me->mTargetsDictByDevID, &devID)) {
        return theGroup->mFetchItems;
    } else {
        return NULL;
    }
}

// Manage acquisition targets
void
ModbusFetchTargets_Add(
    ModbusFetchTargets* me, const ModbusFetchItem* target)
{
    ModbusFetchItemsPerDev*	theGroup = NULL;

    if (! dictionary_get(&theGroup, me->mTargetsDictByDevID, (void*)&target->devID)) {
        theGroup = ModbusFetchItemsPerDev_New(target->devID);
        if (NULL == theGroup) {
            // ERROR!
            return;
        }
        (void)dictionary_put(me->mTargetsDictByDevID, (void*)&target->devID, &theGroup);
    }

    ModbusFetchItemsPerDev_Add(theGroup, target);
}

void
ModbusFetchTargets_Clear(ModbusFetchTargets* me)
{
    vector	devIDs = ModbusFetchTargets_GetDevIDs(me);
    ModbusFetchItemsPerDev*	aGroup;

    for (int i = 0, n = vector_size(devIDs); i < n; ++i) {
        unsigned long	devID;

        vector_get_at(&devID, devIDs, i);
        if (dictionary_get(&aGroup, me->mTargetsDictByDevID, &devID)) {
            ModbusFetchItemsPerDev_Destroy(aGroup);
        }
    }
    dictionary_clear(me->mTargetsDictByDevID);
}
