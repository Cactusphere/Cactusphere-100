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

#include "ModbusConfigMgr.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <applibs_versions.h>
#include <applibs/log.h>

#include "json.h"
#include "LibModbus.h"
#include "ModbusFetchConfig.h"

typedef struct ModbusConfigMgr {
    ModbusFetchConfig* fetchConfig;
} ModbusConfigMgr;

static ModbusConfigMgr sModbusConfigMgr;

// Initialization and cleanup
void
ModbusConfigMgr_Initialize(void)
{
    sModbusConfigMgr.fetchConfig = ModbusFetchConfig_New();
    Libmodbus_ModbusDevInitialize();
}

void
ModbusConfigMgr_Cleanup(void)
{
    ModbusFetchConfig_Destroy(sModbusConfigMgr.fetchConfig);
    Libmodbus_ModbusDevDestroy();
}

// Apply new configuration
void
ModbusConfigMgr_LoadAndApplyIfChanged(const unsigned char* payload,
    unsigned int payloadSize)
{
    json_value* jsonObj = json_parse(payload, payloadSize);
    json_value* desiredObj = NULL;
    json_value* modbusConfObj = NULL;
    json_value* telemetryConfObj = NULL;

    desiredObj = json_GetKeyJson("desired", jsonObj);

    if (desiredObj == NULL) {
        Log_Debug("DeviceTemplate not exists desired.\n");
        modbusConfObj = json_GetKeyJson("ModbusDevConfig", jsonObj);
        telemetryConfObj = json_GetKeyJson("ModbusTelemetryConfig", jsonObj);
    } else {
        modbusConfObj = json_GetKeyJson("ModbusDevConfig", desiredObj);
        telemetryConfObj = json_GetKeyJson("ModbusTelemetryConfig", desiredObj);
    }

    if (modbusConfObj != NULL) {
        modbusConfObj = json_GetKeyJson("value", modbusConfObj);
        modbusConfObj = json_parse(modbusConfObj->u.string.ptr, modbusConfObj->u.string.length);
        if (modbusConfObj != NULL) {
            Libmodbus_ModbusDevClear();
            if (!Libmodbus_LoadFromJSON(modbusConfObj)) {
                Log_Debug("ModbusDevConfig LoadToJsonError!\n");
            }
        }
        else {
            Log_Debug("ModbusDevConfig parse error!\n");
        }
    }

    if (telemetryConfObj != NULL) {
        telemetryConfObj = json_GetKeyJson("value", telemetryConfObj);
        telemetryConfObj = json_parse(telemetryConfObj->u.string.ptr, telemetryConfObj->u.string.length);
        if (telemetryConfObj != NULL) {
            if (!ModbusFetchConfig_LoadFromJSON(sModbusConfigMgr.fetchConfig, telemetryConfObj, "1.0")) {
                Log_Debug("ModbusTelemetryConfig LoadToJsonError!\n");
            }
        } else {
            Log_Debug("ModbusTelemetryConfig string error!\n");
        }
    }
}

// Get configuratioin
ModbusFetchConfig*
ModbusConfigMgr_GetModbusFetchConfig(void)
{
    return sModbusConfigMgr.fetchConfig;
}
