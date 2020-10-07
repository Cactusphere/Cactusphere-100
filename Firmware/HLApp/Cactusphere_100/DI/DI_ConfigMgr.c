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

#include "DI_ConfigMgr.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "applibs_versions.h"
#include <applibs/log.h>

#include "json.h"
#include "DI_FetchConfig.h"
#include "DI_WatchConfig.h"
#include "PropertyItems.h"

typedef struct DI_ConfigMgr {
    DI_FetchConfig* fetchConfig;
    DI_WatchConfig* watchConfig;
} DI_ConfigMgr;

typedef enum {
    FEATURE_UNSELECT = -1,
    FEATURE_FALSE = 0,
    FEATURE_TRUE = 1
}FEATURE_SELECT;

typedef enum {
    DI_FEATURE_PULSECOUNTER = 0,
    DI_FEATURE_EDGE,
    DI_FEATURE_POLLING,
    DI_FEATURE_NUM
}FEATURE_TYPE;

static const char DIFeatureKey[DI_FEATURE_NUM][PROPERTY_NAME_MAX_LEN] = {
    "Counter_DI%d", "Edge_DI%d", "Polling_DI%d"
};

#define DI_PORT_OFFSET 1

static DI_ConfigMgr sDI_ConfigMgr;  // singleton

// Initializaition and cleanup
void
DI_ConfigMgr_Initialize(void)
{
    sDI_ConfigMgr.fetchConfig = DI_FetchConfig_New();
    sDI_ConfigMgr.watchConfig = DI_WatchConfig_New();
}

void
DI_ConfigMgr_Cleanup(void)
{
    DI_FetchConfig_Destroy(sDI_ConfigMgr.fetchConfig);
    DI_WatchConfig_Destroy(sDI_ConfigMgr.watchConfig);
}

static bool
DI_ConfigMgr_CheckStateTransition(FEATURE_SELECT* NextStatus, bool* PrevStatus)
{
    bool ret = true;

    if (NextStatus[DI_FEATURE_PULSECOUNTER] == FEATURE_TRUE) {
        // PulseCounter
        if ((PrevStatus[DI_FEATURE_EDGE] && (NextStatus[DI_FEATURE_EDGE] == FEATURE_UNSELECT)) ||
            (PrevStatus[DI_FEATURE_POLLING] && (NextStatus[DI_FEATURE_POLLING] == FEATURE_UNSELECT))) {
            ret = false;
        }
    } else if (NextStatus[DI_FEATURE_EDGE] == FEATURE_TRUE) {
        // Edge
        if ((PrevStatus[DI_FEATURE_PULSECOUNTER] && (NextStatus[DI_FEATURE_PULSECOUNTER] == FEATURE_UNSELECT)) ||
            (PrevStatus[DI_FEATURE_POLLING] && (NextStatus[DI_FEATURE_POLLING] == FEATURE_UNSELECT))) {
            ret = false;
        }
    } else if (NextStatus[DI_FEATURE_POLLING] == FEATURE_TRUE) {
        // Polling
        if ((PrevStatus[DI_FEATURE_PULSECOUNTER] && (NextStatus[DI_FEATURE_PULSECOUNTER] == FEATURE_UNSELECT)) ||
            (PrevStatus[DI_FEATURE_EDGE] && (NextStatus[DI_FEATURE_EDGE] == FEATURE_UNSELECT))) {
            ret = false;
        }
    }
    return ret;
}

static bool
DI_ConfigMgr_CheckDuplicate(json_value* json, int* enablePort)
{
    bool diPrevStatus[DI_FEATURE_NUM][NUM_DI] = {{false}};
    char diFeatureStr[PROPERTY_NAME_MAX_LEN];        
    json_value* jsonObj;

    *enablePort = DI_FetchConfig_GetFetchEnablePorts(sDI_ConfigMgr.fetchConfig, diPrevStatus[DI_FEATURE_PULSECOUNTER], diPrevStatus[DI_FEATURE_POLLING]) + 
                  DI_WatchConfig_GetWatchEnablePorts(sDI_ConfigMgr.watchConfig, diPrevStatus[DI_FEATURE_EDGE]);
    
    for (int i = 0; i < NUM_DI; i++) {
        FEATURE_SELECT selectStatus[DI_FEATURE_NUM] = {FEATURE_UNSELECT, FEATURE_UNSELECT, FEATURE_UNSELECT};
        int selectNum = 0;
        
        for (int j = 0; j < DI_FEATURE_NUM; j++) {
            bool value;
            sprintf(diFeatureStr, DIFeatureKey[j], i + DI_PORT_OFFSET);
            jsonObj = json_GetKeyJson(diFeatureStr, json);
            if (json_GetBoolValue(jsonObj, &value)) {
                selectStatus[j] = value;
                if (selectStatus[j] == FEATURE_TRUE) {
                    if (++selectNum > 1) {
                        Log_Debug("Setting value is duplicated\n");
                        return false;
                    }
                }
            }
        }

        bool prevStatus[DI_FEATURE_NUM];
        for (int j = 0; j < DI_FEATURE_NUM; j++) {
            prevStatus[j] = diPrevStatus[j][i];
        }

        if (! DI_ConfigMgr_CheckStateTransition(selectStatus,prevStatus)) {
            Log_Debug("Setting value is duplicated\n");
            return false;
        }
    }

    return true;
}

// Apply new configuration
SphereWarning
DI_ConfigMgr_LoadAndApplyIfChanged(const unsigned char* payload,
    unsigned int payloadSize, vector item)
{	
    json_value* jsonObj = json_parse(payload, payloadSize);
    json_value* desiredObj = json_GetKeyJson("desired", jsonObj);
    bool desireFlg = false;
    int enablePort = 0;

    if (desiredObj) {
        jsonObj = desiredObj;
        desireFlg = true;
    }

    if (! DI_ConfigMgr_CheckDuplicate(jsonObj, &enablePort)){
        if (desireFlg && (enablePort < 1)) { // initial desired message
            return ILLEGAL_DESIRED_PROPERTY;
        } else {
            return ILLEGAL_PROPERTY;
        }
    }

    bool initDesired = enablePort < 1 ? true : false;

    // configuration of the pulse counter function
    if (! DI_FetchConfig_LoadFromJSON(
        sDI_ConfigMgr.fetchConfig, jsonObj, initDesired , item, "1.0")) {
        Log_Debug("failed to DI_FetchConfig_LoadFromJSON().\n");
        return ILLEGAL_PROPERTY;
    }

    // configuration of the contact input function
    if (! DI_WatchConfig_LoadFromJSON(
        sDI_ConfigMgr.watchConfig, jsonObj, initDesired, item, "1.0")) {
        Log_Debug("failed to DI_FetchConfig_LoadFromJSON().\n");
        return ILLEGAL_PROPERTY;
    }

    if ( (jsonObj->u.object.length > 1) && (vector_size(item) < 1)) {
        return UNSUPPORTED_PROPERTY;
    }

    return NO_ERROR;
}

// Get configuratioin
DI_FetchConfig*
DI_ConfigMgr_GetFetchConfig()
{
    return sDI_ConfigMgr.fetchConfig;
}

DI_WatchConfig*
DI_ConfigMgr_GetWatchConfig()
{
    return sDI_ConfigMgr.watchConfig;
}
