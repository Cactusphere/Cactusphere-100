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

#include "DI_FetchConfig.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "json.h"
#include "DI_FetchItem.h"
#include "TelemetryItems.h"
#include "PropertyItems.h"

struct DI_FetchConfig {
    vector	mFetchItems;    // vector of DI pulse conter configuration
    vector	mFetchItemPtrs;	// vector of pointer which points mFetchItem's elem
    char	version[32];	// version string (not using)
};

// key Items in JSON
const char CounterDIKey[]          = "Counter_DI";
const char CntIsPulseHighDIKey[]   = "cntIsPulseHigh_DI";
const char CntIntervalDIKey[]      = "cntInterval_DI";
const char CntMinPulseWidthDIKey[] = "cntMinPulseWidth_DI";
const char CntMaxPulseCountDIKey[] = "cntMaxPulseCount_DI";
const char PollIsActiveHighKey[]   = "pollIsActiveHigh_DI";
const char PollIntervalDIKey[]     = "pollInterval_DI";

#define DI_FETCH_PORT_OFFSET 1

// Initialization and cleanup
DI_FetchConfig*
DI_FetchConfig_New(void)
{
    DI_FetchConfig*	newObj =
        (DI_FetchConfig*)malloc(sizeof(DI_FetchConfig));

    if (NULL != newObj) {
        newObj->mFetchItems = vector_init(sizeof(DI_FetchItem));
        if (NULL == newObj->mFetchItems) {
            free(newObj);
            return NULL;
        }
        newObj->mFetchItemPtrs = vector_init(sizeof(DI_FetchItem*));
        if (NULL == newObj->mFetchItemPtrs) {
            vector_destroy(newObj->mFetchItems);
            free(newObj);
            return NULL;
        }
        memset(newObj->version, 0, sizeof(newObj->version));
    }

    return newObj;
}

void
DI_FetchConfig_Destroy(DI_FetchConfig* me)
{
    vector_destroy(me->mFetchItemPtrs);
    vector_destroy(me->mFetchItems);
    free(me);
}

// Load DI pulse conter configuration from JSON
bool
DI_FetchConfig_LoadFromJSON(DI_FetchConfig* me,
    const json_value* json, bool desire, vector propertyItem, const char* version)
{
    DI_FetchItem config[NUM_DI] = {
        // telemetryName, intervalSec, pinID, isPulseCounter, isCountClear, isPulseHigh, isPollingActiveHigh, minPulseWidth, maxPulseCount
        {"", 1, 0, false, false, false, false, 200, 0x7FFFFFFF},
        {"", 1, 1, false, false, false, false, 200, 0x7FFFFFFF},
        {"", 1, 2, false, false, false, false, 200, 0x7FFFFFFF},
        {"", 1, 3, false, false, false, false, 200, 0x7FFFFFFF}
    };
    bool overWrite[NUM_DI] = {false};
    bool ret = true;

    const size_t cntIsPulseHighDiLen   = strlen(CntIsPulseHighDIKey);
    const size_t cntIntervalDiLen      = strlen(CntIntervalDIKey);
    const size_t cntMinPulseWidthDiLen = strlen(CntMinPulseWidthDIKey);
    const size_t cntMaxPulseCountDiLen = strlen(CntMaxPulseCountDIKey);
    const size_t pollIsActiveHighDiLen = strlen(PollIsActiveHighKey);
    const size_t pollIntervalDiLen     = strlen(PollIntervalDIKey);

    char diCounterStr[PROPERTY_NAME_MAX_LEN];
    char diPollingStr[PROPERTY_NAME_MAX_LEN];

    if (! json) {
        return false;
    }

    if (! vector_is_empty(me->mFetchItems)) {
        DI_FetchItem* tmp = (DI_FetchItem*)vector_get_data(me->mFetchItems);
        for (int i = 0; i < vector_size(me->mFetchItems); i++) {
            memcpy(&config[tmp->pinID], tmp, sizeof(DI_FetchItem));
            overWrite[tmp->pinID] = true;
            tmp ++;
        }
    }

    if (0 != vector_size(me->mFetchItems)) {
        DI_FetchItem*	curs = (DI_FetchItem*)vector_get_data(me->mFetchItems);

        for (int i = 0, n = vector_size(me->mFetchItems); i < n; ++i) {
            TelemetryItems_RemoveDictionaryElem(curs->telemetryName);
        }
        vector_clear(me->mFetchItemPtrs);
        vector_clear(me->mFetchItems);
    }

    // Check if the feature has changed.
    for (int i = 0; i < NUM_DI; i++) {
        int countVal = -1;
        int pollVal = -1;

        sprintf(diCounterStr, "Counter_DI%d", i + DI_FETCH_PORT_OFFSET);
        sprintf(diPollingStr, "Polling_DI%d", i + DI_FETCH_PORT_OFFSET);

        json_value* countObj = json_GetKeyJson(diCounterStr, json);
        json_value* pollObj = json_GetKeyJson(diPollingStr, json);

        if (countObj) {
            bool value;
            if (json_GetBoolValue(countObj, &value)) {
                countVal = (int)value;
                PropertyItems_AddItem(propertyItem, diCounterStr, TYPE_BOOL, (bool)countVal);
            }
        }
        if (pollObj) {
            bool value;
            if (json_GetBoolValue(pollObj, &value)) {
                pollVal = (int)value;
                PropertyItems_AddItem(propertyItem, diPollingStr, TYPE_BOOL, (bool)pollVal);
            }
        }

        if ((countVal == 1) && (pollVal == 1)) {
            return false;
        } else if ((countVal == 1) && (pollVal != 1)) { // Polling -> PulseCounter
            overWrite[i] = true;
            if (!config[i].isPulseCounter || desire) {
                // feature has changed
                config[i].isCountClear = true;
                config[i].intervalSec = 1;
                config[i].minPulseWidth = 200; // default
                config[i].maxPulseCount = 0x7FFFFFFF; // default
            }
            config[i].isPulseCounter = true;
            sprintf(config[i].telemetryName, "DI%d_count", i + DI_FETCH_PORT_OFFSET);
        } else if ((countVal != 1) && (pollVal == 1)) { // PulseCounter -> Polling
            overWrite[i] = true;
            if (config[i].isPulseCounter || desire) {
                // feacture has changed
                config[i].isCountClear = true;
                config[i].intervalSec = 1;
                config[i].minPulseWidth = 200; // default
                config[i].maxPulseCount = 0x7FFFFFFF; // default
            }
            config[i].isPulseCounter = false;
            sprintf(config[i].telemetryName, "DI%d_PollingStatus", i + DI_FETCH_PORT_OFFSET);
        } else if ((config[i].isPulseCounter) && (countVal == 0)) { // PulseCounter ON -> OFF
            // feature has changed (ON->OFF)
            overWrite[i] = false;
            config[i].isCountClear = true;
        } else if ((!config[i].isPulseCounter) && (pollVal == 0)) { // Polling ON -> OFF
            // feature has changed (ON->OFF)
            overWrite[i] = false;
            config[i].isCountClear = true;
        }
    }

    int pinid;
    for (int i = 0; i < json->u.object.length; i++) {
        char* propertyName = json->u.object.values[i].name;
        json_value* item = json->u.object.values[i].value;

        if (0 == strncmp(propertyName, CntIsPulseHighDIKey, cntIsPulseHighDiLen)) {
            pinid = strtol(&propertyName[cntIsPulseHighDiLen], NULL, 10) - DI_FETCH_PORT_OFFSET;
            if (pinid < 0) {
                continue;
            }
            bool value;
            if (config[pinid].isPulseCounter) {
                if (json_GetBoolValue(item,&value)) {
                    if (config[pinid].isPulseHigh != value) {
                        config[pinid].isCountClear = true;
                    }
                    config[pinid].isPulseHigh = value;
                } else {
                    ret = false;
                }
            }
            PropertyItems_AddItem(propertyItem, propertyName, TYPE_BOOL, value);
        } else if (0 == strncmp(propertyName, CntIntervalDIKey, cntIntervalDiLen)) {
            pinid = strtol(&propertyName[cntIntervalDiLen], NULL, 10) - DI_FETCH_PORT_OFFSET;
            if (pinid < 0) {
                continue;
            }

            uint32_t value;
            int8_t result = json_GetIntValue(item, &value, 10);
            if (config[pinid].isPulseCounter) {
                if (result && value >= 1 && value <= 86400) {
                    if (config[pinid].intervalSec != value) {
                        config[pinid].isCountClear = true;
                    }
                    config[pinid].intervalSec = value;
                } else {
                    ret = false;
                    overWrite[pinid] = false;
                }
            }
            PropertyItems_AddItem(propertyItem, propertyName, TYPE_NUM, value);
        } else if (0 == strncmp(propertyName, CntMinPulseWidthDIKey, cntMinPulseWidthDiLen)) {
            pinid = strtol(&propertyName[cntMinPulseWidthDiLen], NULL, 10) - DI_FETCH_PORT_OFFSET;
            if (pinid < 0) {
                continue;
            }

            uint32_t value;
            int8_t result = json_GetIntValue(item, &value, 10);
            if (config[pinid].isPulseCounter) {
                if (result && value >= 1 && value <= 1000) {
                    if (config[pinid].minPulseWidth != value) {
                        config[pinid].isCountClear = true;
                    }
                    config[pinid].minPulseWidth = value;
                } else {
                    ret = false;
                    overWrite[pinid] = false;
                }
            }
            PropertyItems_AddItem(propertyItem, propertyName, TYPE_NUM, value);
        } else if (0 == strncmp(propertyName, CntMaxPulseCountDIKey, cntMaxPulseCountDiLen)) {
            pinid = strtol(&propertyName[cntMaxPulseCountDiLen], NULL, 10) - DI_FETCH_PORT_OFFSET;
            if (pinid < 0) {
                continue;
            }

            uint32_t value;
            int8_t result = json_GetIntValue(item, &value, 10);
            if (config[pinid].isPulseCounter) {
                if (result && value >= 1 && value <= 0x7FFFFFFF) {
                    if (config[pinid].maxPulseCount != value) {
                        config[pinid].isCountClear = true;
                    }
                    config[pinid].maxPulseCount = value;
                } else {
                    ret = false;
                    overWrite[pinid] = false;
                }
            }
            PropertyItems_AddItem(propertyItem, propertyName, TYPE_NUM, value);
        } else if (0 == strncmp(propertyName, PollIntervalDIKey, pollIntervalDiLen)) {
            pinid = strtol(&propertyName[pollIntervalDiLen], NULL, 10) - DI_FETCH_PORT_OFFSET;
            if (pinid < 0) {
                continue;
            }
            uint32_t value;
            int8_t result = json_GetIntValue(item, &value, 10);

            if (!config[pinid].isPulseCounter) {
                if (result && value >= 1 && value <= 86400) {
                    if (config[pinid].intervalSec != value) {
                        config[pinid].isCountClear = true;
                    }
                    config[pinid].intervalSec = value;
                } else {
                    ret = false;
                    overWrite[pinid] = false;
                }
            }
            PropertyItems_AddItem(propertyItem, propertyName, TYPE_NUM, value);
        } else if (0 == strncmp(propertyName, PollIsActiveHighKey, pollIsActiveHighDiLen)) {
            pinid = strtol(&propertyName[pollIsActiveHighDiLen], NULL, 10) - DI_FETCH_PORT_OFFSET;
            if (pinid < 0) {
                continue;
            }
            bool value;
            if (!config[pinid].isPulseCounter) {
                if (json_GetBoolValue(item,&value)) {
                    if (config[pinid].isPollingActiveHigh != value) {
                        config[pinid].isCountClear = true;
                    }
                    config[pinid].isPollingActiveHigh = value;
                } else {
                    ret = false;
                }
            }
            PropertyItems_AddItem(propertyItem, propertyName, TYPE_BOOL, value);
        }
    }

    for (int i = 0; i < NUM_DI; i++) {
        if (overWrite[i]) {
            vector_add_last(me->mFetchItems, &config[i]);
        }
    }

    if (! vector_is_empty(me->mFetchItems)) {
        // store entity's pointers
        DI_FetchItem* curs = (DI_FetchItem*)vector_get_data(me->mFetchItems);

        for (int i = 0, n = vector_size(me->mFetchItems); i < n; ++i) {
            vector_add_last(me->mFetchItemPtrs, &curs);
            TelemetryItems_AddDictionaryElem(curs->telemetryName, false);
            ++curs;
        }
    }

    return ret;
}

// Get configuration of DI pulse conters
vector
DI_FetchConfig_GetFetchItems(DI_FetchConfig* me)
{
    return me->mFetchItems;
}

vector
DI_FetchConfig_GetFetchItemPtrs(DI_FetchConfig* me)
{
    return me->mFetchItemPtrs;
}

// Get enable port number of DI pulse counter
int
DI_FetchConfig_GetFetchEnablePorts(DI_FetchConfig* me,
    bool* counterStatus, bool* pollingStatus)
{
    DI_FetchItem* tmp = (DI_FetchItem*)vector_get_data(me->mFetchItems);
    int enablePort = vector_size(me->mFetchItems);

    for (int i = 0; i < enablePort; i++){
        if (tmp->isPulseCounter) {
            counterStatus[tmp->pinID] = true;
        } else {
            pollingStatus[tmp->pinID] = true;
        }
        tmp++;
    }

    return enablePort;
}
