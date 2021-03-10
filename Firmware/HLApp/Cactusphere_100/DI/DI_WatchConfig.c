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

#include "DI_WatchConfig.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "json.h"
#include "DI_WatchItem.h"
#include "TelemetryItems.h"
#include "PropertyItems.h"

struct DI_WatchConfig {
    vector	mWatchItems;	// vector of DI contact input configuration
    char	version[32];	// version string (not using)
};

// key Items in JSON
extern const char PinIDKey[];
const char DI_WatchConfigConfigKey[] = "DIWatchConfig";
const char NotifyChangeForeHigh[]    = "notifyChangeForeHigh";

const char EdgeDIKey[]             = "Edge_DI";
const char EdgeNotifyIsHighDIKey[] = "edgeNotifyIsHigh_DI";

#define DI_WATCH_PORT_OFFSET 1

// Initialization and cleanup
DI_WatchConfig*
DI_WatchConfig_New(void)
{
    DI_WatchConfig*	newObj =
        (DI_WatchConfig*)malloc(sizeof(DI_WatchConfig));

    if (NULL != newObj) {
        newObj->mWatchItems = vector_init(sizeof(DI_WatchItem));
        if (NULL == newObj->mWatchItems) {
            free(newObj);
            return NULL;
        }
        memset(newObj->version, 0, sizeof(newObj->version));
    }

    return newObj;
}

void
DI_WatchConfig_Destroy(DI_WatchConfig* me)
{
    vector_destroy(me->mWatchItems);
    free(me);
}

// Load DI contact input watcher configuration from JSON
bool
DI_WatchConfig_LoadFromJSON(DI_WatchConfig* me,
    const json_value* json, bool desire, vector propertyItem, const char* version)
{
    DI_WatchItem config[NUM_DI] = {
        // telemetryName, pinID, notifyChangeForHigh, isCountClear
        {"", 0, false, false},
        {"", 1, false, false},
        {"", 2, false, false},
        {"", 3, false, false}
    };
    bool overWrite[NUM_DI] = {false};
    bool ret = true;

    const size_t edgeDiLen = strlen(EdgeDIKey);
    const size_t notifyHighDiLen = strlen(EdgeNotifyIsHighDIKey);

    if (! json) {
        return false;
    }

    if (! vector_is_empty(me->mWatchItems)) {
        DI_WatchItem* tmp = (DI_WatchItem*)vector_get_data(me->mWatchItems);
        for (int i = 0; i < vector_size(me->mWatchItems); i++){
            memcpy(&config[tmp->pinID], tmp, sizeof(DI_WatchItem));
            overWrite[tmp->pinID] = true;
            tmp ++;
        }
    }

    if (0 != vector_size(me->mWatchItems)) {
        DI_WatchItem*	curs = (DI_WatchItem*)vector_get_data(me->mWatchItems);

        for (int i = 0, n = vector_size(me->mWatchItems); i < n; ++i) {
            TelemetryItems_RemoveDictionaryElem(curs->telemetryName);
        }
        vector_clear(me->mWatchItems);
        memset(me->version, 0, sizeof(me->version));
    }

    int pinid;
    for (int i = 0; i < json->u.object.length; i++) {
        char* propertyName = json->u.object.values[i].name;
        json_value* item = json->u.object.values[i].value;

        if (0 == strncmp(propertyName, EdgeDIKey, edgeDiLen)) {
            if ((pinid = strtol(&propertyName[edgeDiLen], NULL, 10) - DI_WATCH_PORT_OFFSET) < 0) {
                continue;
            }
            bool value;
            if (json_GetBoolValue(item, &value)) {
                if ((overWrite[pinid] && !value) || (!overWrite[pinid] && value)) {
                    // feature has changed
                    config[pinid].isCountClear = true;
                }
                overWrite[pinid] = value;
                sprintf(config[pinid].telemetryName, "DI%d_EdgeEvent", pinid + DI_WATCH_PORT_OFFSET);
                PropertyItems_AddItem(propertyItem, propertyName, TYPE_BOOL, overWrite[pinid]);
            } else {
                ret = false;
            }
        } else if(0 == strncmp(propertyName, EdgeNotifyIsHighDIKey, notifyHighDiLen)) {
            if ((pinid = strtol(&propertyName[notifyHighDiLen], NULL, 10) - DI_WATCH_PORT_OFFSET) < 0) {
                continue;
            }
            bool value;
            if (json_GetBoolValue(item, &value)) {
                config[pinid].isCountClear = (config[pinid].notifyChangeForHigh != value) ? true : false;
                config[pinid].notifyChangeForHigh = value;
                PropertyItems_AddItem(propertyItem, propertyName, TYPE_BOOL, config[pinid].notifyChangeForHigh);
            } else {
                ret = overWrite[pinid] = false;
            }
        }
    }

    for (int i = 0; i < NUM_DI; i++) {
        if (overWrite[i]) {
            vector_add_last(me->mWatchItems, &config[i]);
        }
    }

    if (! vector_is_empty(me->mWatchItems)) {
        // store entity's pointers
        DI_WatchItem*	curs = (DI_WatchItem*)vector_get_data(me->mWatchItems);

        for (int i = 0, n = vector_size(me->mWatchItems); i < n; ++i) {
            TelemetryItems_AddDictionaryElem(curs->telemetryName, false);
            ++curs;
        }
    }

    return ret;
}

// Get configuration of DI contact input watchers
vector
DI_WatchConfig_GetFetchItems(DI_WatchConfig* me)
{
    return me->mWatchItems;
}

// Get enable port number of DI contact input
int
DI_WatchConfig_GetWatchEnablePorts(DI_WatchConfig* me, bool* status)
{
    DI_WatchItem* tmp;
    int enablePort = vector_size(me->mWatchItems);

    for (int i = 0; i < enablePort; i++) {
        tmp = (DI_WatchItem*)vector_get_data(me->mWatchItems);
        status[tmp->pinID] = true;
    }

    return enablePort;
}
