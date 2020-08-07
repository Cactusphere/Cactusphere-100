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

#include "ModbusFetchConfig.h"

#include <string.h>
#include <stdlib.h>

#include "json.h"
#include "ModbusFetchItem.h"
#include "ModbusDevConfig.h"
#include "TelemetryItems.h"

struct ModbusFetchConfig {
    vector	mFetchItems;	// vector of Modbus RTU configuration
    vector	mFetchItemPtrs;	// vector of pointer which points mFetchItem's elem
    char	version[32];	// version string (not using)
};

// key Items
const char ModbusTelemetryConfigKey[]   = "ModbusTelemetryConfig";
const char DevIDKey[]                   = "devID";
const char RegisterAddrKey[]            = "registerAddr";
const char RegisterCountKey[]           = "registerCount";
const char FuncCodeKey[]                = "funcCode";
const char OffsetKey[]                  = "offset";
const char IntervalKey[]                = "interval";
const char MultiplylKey[]               = "multiply";
const char DeviderKey[]                 = "devider";
const char AsFloatKey[]                 = "asFloat";

// Initialization and cleanup
ModbusFetchConfig*
ModbusFetchConfig_New(void)
{
    ModbusFetchConfig*	newObj =
        (ModbusFetchConfig*)malloc(sizeof(ModbusFetchConfig));

    if (NULL != newObj) {
        newObj->mFetchItems = vector_init(sizeof(ModbusFetchItem));
        if (NULL == newObj->mFetchItems) {
            free(newObj);
            return NULL;
        }
        newObj->mFetchItemPtrs = vector_init(sizeof(ModbusFetchItem*));
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
ModbusFetchConfig_Destroy(ModbusFetchConfig* me)
{
    vector_destroy(me->mFetchItemPtrs);
    vector_destroy(me->mFetchItems);
    free(me);
}

// Load Modbus RTU configuration from JSON
bool
ModbusFetchConfig_LoadFromJSON(ModbusFetchConfig* me,
    const json_value* json, const char* version)
{
    json_value* configJson = NULL;

    // clean up old configuration and load new content
    if (0 != vector_size(me->mFetchItems)) {
        ModbusFetchItem*	curs = (ModbusFetchItem*)vector_get_data(me->mFetchItems);

        for (int i = 0, n = vector_size(me->mFetchItems); i < n; ++i) {
            TelemetryItems_RemoveDictionaryElem(curs->telemetryName);
        }
        vector_clear(me->mFetchItemPtrs);
        vector_clear(me->mFetchItems);
    }

    for (unsigned int i = 0, n = json->u.object.length; i < n; ++i) {
        if (0 == strcmp(ModbusTelemetryConfigKey, json->u.object.values[i].name)) {
            configJson = json->u.object.values[i].value;
            break;
        }
    }

    if (configJson == NULL) {
        return false;
    }

    for (unsigned int i = 0, n = configJson->u.object.length; i < n; ++i) {
        ModbusFetchItem pseudo;
        bool isAddList = true;
        json_value* configItem = configJson->u.object.values[i].value;
        size_t	strLen = strlen(configJson->u.object.values[i].name);

        if (strLen > sizeof(pseudo.telemetryName) - 1) {
            strLen = sizeof(pseudo.telemetryName) - 1;
        }
        memcpy(pseudo.telemetryName, configJson->u.object.values[i].name, strLen);
        pseudo.telemetryName[strLen] = '\0';

        pseudo.devID = 0;
        pseudo.regAddr = 0;
        pseudo.regCount = 0;
        pseudo.funcCode = 0;
        pseudo.offset = 0;
        pseudo.intervalSec = 1;
        pseudo.multiplier = 0;
        pseudo.devider = 0;
        pseudo.asFloat = false;

        for (unsigned int p = 0, q = configItem->u.object.length; p < q; ++p) {
            if (0 == strcmp(configItem->u.object.values[p].name, DevIDKey)) {
                json_value* item = configItem->u.object.values[p].value;
                uint32_t value;

                if (json_GetNumericValue(item, &value, 16)) {
                    pseudo.devID = value;
                } else {
                    isAddList = false;
                }
            }
            else if (0 == strcmp(configItem->u.object.values[p].name, RegisterAddrKey)) {
                json_value* item = configItem->u.object.values[p].value;
                uint32_t value;

                if (json_GetNumericValue(item, &value, 16)) {
                    pseudo.regAddr = value;
                } else {
                    isAddList = false;
                }
            }
            else if (0 == strcmp(configItem->u.object.values[p].name, RegisterCountKey)) {
                json_value* item = configItem->u.object.values[p].value;
                uint32_t value;

                if (json_GetNumericValue(item, &value, 16)) {
                    pseudo.regCount = value;
                    if (pseudo.regCount < 1 || pseudo.regCount > 2) {
                        isAddList = false;
                    }
                } else {
                    isAddList = false;
                }
            }
            else if (0 == strcmp(configItem->u.object.values[p].name, FuncCodeKey)) {
                json_value* item = configItem->u.object.values[p].value;
                uint32_t value;

                if (json_GetNumericValue(item, &value, 16)) {
                    pseudo.funcCode = value;
                    switch (pseudo.funcCode)
                    {
                    case FC_READ_HOLDING_REGISTER:
                    case FC_READ_INPUT_REGISTERS:
                        break;
                    default:
                        isAddList = false;
                        break;
                    }
                } else {
                    isAddList = false;
                }
            }
            else if (0 == strcmp(configItem->u.object.values[p].name, IntervalKey)) {
                json_value* item = configItem->u.object.values[p].value;
                uint32_t value;

                if (json_GetNumericValue(item, &value, 10)) {
                    pseudo.intervalSec = value;
                    if (pseudo.intervalSec <= 0) {
                        pseudo.intervalSec = 1;
                    }
                } else {
                    isAddList = false;
                }
            }
            else if (0 == strcmp(configItem->u.object.values[p].name, OffsetKey)) {
                json_value* item = configItem->u.object.values[p].value;
                uint32_t value;

                if (json_GetNumericValue(item, &value, 10)) {
                    pseudo.offset = (uint16_t)value;
                }
            }
            else if (0 == strcmp(configItem->u.object.values[p].name, MultiplylKey)) {
                json_value* item = configItem->u.object.values[p].value;
                uint32_t value;

                if (json_GetNumericValue(item, &value, 10)) {
                    pseudo.multiplier = value;
                }
            }
            else if (0 == strcmp(configItem->u.object.values[p].name, DeviderKey)) {
                json_value* item = configItem->u.object.values[p].value;
                uint32_t value;

                if (json_GetNumericValue(item, &value, 10)) {
                    pseudo.devider = value;
                }
            }
            else if (0 == strcmp(configItem->u.object.values[p].name, AsFloatKey)) {
                json_value* item = configItem->u.object.values[p].value;

                pseudo.asFloat = item->u.boolean;
            }

        }
        if (isAddList) {
            vector_add_last(me->mFetchItems, &pseudo);
        }
    }

    if (! vector_is_empty(me->mFetchItems)) {
        ModbusFetchItem* curs = (ModbusFetchItem*)vector_get_data(me->mFetchItems);

        for (int i = 0, n = vector_size(me->mFetchItems); i < n; ++i) {
            vector_add_last(me->mFetchItemPtrs, &curs);
            TelemetryItems_AddDictionaryElem(curs->telemetryName, curs->asFloat);
            ++curs;
        }

        return true;
    }
    return false;
}

// Get configuration
vector
ModbusFetchConfig_GetFetchItems(ModbusFetchConfig* me)
{
    return me->mFetchItems;
}

vector
ModbusFetchConfig_GetFetchItemPtrs(ModbusFetchConfig* me)
{
    return me->mFetchItemPtrs;
}
