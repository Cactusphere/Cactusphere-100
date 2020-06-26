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

#include "ModbusTcpFetchTargets.h"

#include <stdio.h>
#include <string.h>

#include "ModbusTcpFetchItem.h"
#include "dictionary.h"

#define MODBUS_TCP_ID_SIZE 21

typedef struct ModbusTcpFetchItemsPerDev {
    char mId[MODBUS_TCP_ID_SIZE];		// id is "ipAddr:port"
    vector	mFetchItems;	            // telemetry items
} ModbusTcpFetchItemsPerDev;
 
static ModbusTcpFetchItemsPerDev*	ModbusTcpFetchItemsPerDev_New(char* id);
static void	ModbusTcpFetchItemsPerDev_Destroy(ModbusTcpFetchItemsPerDev* me);
static void	ModbusTcpFetchItemsPerDev_Add(
    ModbusTcpFetchItemsPerDev* me, const ModbusTcpFetchItem* fetchItem);


// ModbusTcpFetchTargets data members
struct ModbusTcpFetchTargets {
    dictionary	mTargetsDictByDevID;
};

static ModbusTcpFetchItemsPerDev*
ModbusTcpFetchItemsPerDev_New(char* id)
{
    ModbusTcpFetchItemsPerDev*	newObj =
        (ModbusTcpFetchItemsPerDev*)malloc(sizeof(ModbusTcpFetchItemsPerDev));

    if (NULL != newObj) {
        strcpy(newObj->mId, id);
        newObj->mFetchItems = vector_init(sizeof(ModbusTcpFetchItem*));
        if (NULL == newObj->mFetchItems) {
            free(newObj);
            return newObj;
        }
    }

    return newObj;
}

static void
ModbusTcpFetchItemsPerDev_Destroy(ModbusTcpFetchItemsPerDev* me)
{
    vector_destroy(me->mFetchItems);
    free(me);
}

static void
ModbusTcpFetchItemsPerDev_Add(
    ModbusTcpFetchItemsPerDev* me, const ModbusTcpFetchItem* fetchItem)
{
    vector_add_last(me->mFetchItems, (void*)&fetchItem);
}

static int
ID_Comparator(const void* const one, const void* const two)
{
    int res = strcmp(one, two);

    if (res < 0) {
        return -1;
    } else if (res > 0) {
        return 1;
    } else {
        return 0;
    }
}

// Initialization and cleanup
ModbusTcpFetchTargets*
ModbusTcpFetchTargets_New(void)
{
    ModbusTcpFetchTargets*	newObj =
        (ModbusTcpFetchTargets*)malloc(sizeof(ModbusTcpFetchTargets));

    if (NULL != newObj) {
        newObj->mTargetsDictByDevID = dictionary_init(
            MODBUS_TCP_ID_SIZE, sizeof(ModbusTcpFetchItemsPerDev*),
            ID_Comparator);
        if (NULL == newObj->mTargetsDictByDevID) {
            free(newObj);
            return NULL;
        }
    }

    return newObj;
}

void
ModbusTcpFetchTargets_Destroy(ModbusTcpFetchTargets* me)
{
    vector	IDs = ModbusTcpFetchTargets_GetDevIDs(me);
    ModbusTcpFetchItemsPerDev*	theGroup = NULL;

    ModbusTcpFetchTargets_Clear(me);

    for (int i = 0, n = vector_size(IDs); i < n; ++i) {
        char* id = NULL;
        vector_get_at(id, IDs, i);
        if (dictionary_get(&theGroup, me->mTargetsDictByDevID, id)) {
            ModbusTcpFetchItemsPerDev_Destroy(theGroup);
        }
    }

    dictionary_destroy(me->mTargetsDictByDevID);
    free(me);
}

// Get current acquisition targets
vector
ModbusTcpFetchTargets_GetDevIDs(ModbusTcpFetchTargets* me)
{
    return dictionary_get_keys(me->mTargetsDictByDevID);
}

vector
ModbusTcpFetchTargets_GetFetchItems(
    ModbusTcpFetchTargets* me, char* id)
{
    ModbusTcpFetchItemsPerDev*	theGroup = NULL;

    if (dictionary_get(&theGroup, me->mTargetsDictByDevID, id)) {
        return theGroup->mFetchItems;
    } else {
        return NULL;
    }
}

// Manage acquisition targets
void
ModbusTcpFetchTargets_Add(
    ModbusTcpFetchTargets* me, const ModbusTcpFetchItem* target)
{
    ModbusTcpFetchItemsPerDev*	theGroup = NULL;

   char id[MODBUS_TCP_ID_SIZE];
    sprintf(id, "%s:%d", target->ipAddr, target->port);

    if (! dictionary_get(&theGroup, me->mTargetsDictByDevID, &id)) {
        theGroup = ModbusTcpFetchItemsPerDev_New(id);
        if (NULL == theGroup) {
            // ERROR!
            return;
        }
        (void)dictionary_put(me->mTargetsDictByDevID, &id, &theGroup);
    }

    ModbusTcpFetchItemsPerDev_Add(theGroup, target);
}

void
ModbusTcpFetchTargets_Clear(ModbusTcpFetchTargets* me)
{
    vector	IDs = ModbusTcpFetchTargets_GetDevIDs(me);
    ModbusTcpFetchItemsPerDev*	aGroup;

    for (int i = 0, n = vector_size(IDs); i < n; ++i) {
        char id[MODBUS_TCP_ID_SIZE];

        vector_get_at(&id, IDs, i);
        if (dictionary_get(&aGroup, me->mTargetsDictByDevID, id)) {
            ModbusTcpFetchItemsPerDev_Destroy(aGroup);
        }
    }

    dictionary_clear(me->mTargetsDictByDevID);
}
