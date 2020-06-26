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

#include "ModbusTcpConfigMgr.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <applibs_versions.h>
#include <applibs/log.h>

#include "json.h"
#include "LibModbusTcp.h"
#include "ModbusTcpFetchConfig.h"

typedef struct ModbusTcpConfigMgr {
    ModbusTcpFetchConfig* fetchConfig;
} ModbusTcpConfigMgr;

static ModbusTcpConfigMgr sModbusTcpConfigMgr;

// Initialization and cleanup
void
ModbusTcpConfigMgr_Initialize(void)
{
    sModbusTcpConfigMgr.fetchConfig = ModbusTcpFetchConfig_New();
    LibmodbusTcp_ModbusDevInitialize();
}

void
ModbusTcpConfigMgr_Cleanup(void)
{
    ModbusTcpFetchConfig_Destroy(sModbusTcpConfigMgr.fetchConfig);
    LibmodbusTcp_ModbusDevDestroy();
}

// Apply new configuration
void
ModbusTcpConfigMgr_LoadAndApplyIfChanged(const unsigned char* payload,
    unsigned int payloadSize)
{
    json_value* jsonObj = json_parse(payload, payloadSize);
    json_value* desiredObj = NULL;
    json_value* modbusConfObj = NULL;
    json_value* telemetryConfObj = NULL;

    desiredObj = json_GetKeyJson("desired", jsonObj);

    if (desiredObj == NULL) {
        Log_Debug("DeviceTemplate not exists desired.\n");
        modbusConfObj = json_GetKeyJson("ModbusTcpConfig", jsonObj);
        telemetryConfObj = json_GetKeyJson("ModbusTcpTelemetryConfig", jsonObj);
    } else {
        modbusConfObj = json_GetKeyJson("ModbusTcpConfig", desiredObj);
        telemetryConfObj = json_GetKeyJson("ModbusTcpTelemetryConfig", desiredObj);
    }

    if (modbusConfObj != NULL) {
        modbusConfObj = json_GetKeyJson("value", modbusConfObj);
        modbusConfObj = json_parse(modbusConfObj->u.string.ptr, modbusConfObj->u.string.length);
        if (modbusConfObj != NULL) {
            LibmodbusTcp_ModbusDevClear();
            if (!LibmodbusTcp_LoadFromJSON(modbusConfObj)) {
                Log_Debug("ModbusTcpConfig LoadToJsonError!\n");
            }
        }
        else {
            Log_Debug("ModbusTcpConfig parse error!\n");
        }
    }

    if (telemetryConfObj != NULL) {
        telemetryConfObj = json_GetKeyJson("value", telemetryConfObj);
        telemetryConfObj = json_parse(telemetryConfObj->u.string.ptr, telemetryConfObj->u.string.length);
        if (telemetryConfObj != NULL) {
            if (!ModbusTcpFetchConfig_LoadFromJSON(sModbusTcpConfigMgr.fetchConfig, telemetryConfObj, "1.0")) {
                Log_Debug("ModbusTcpTelemetryConfig LoadToJsonError!\n");
            }
        } else {
            Log_Debug("ModbusTcpTelemetryConfig string error!\n");
        }
    }
}

// Get configuratioin
ModbusTcpFetchConfig*
ModbusTcpConfigMgr_GetModbusFetchConfig(void)
{
    return sModbusTcpConfigMgr.fetchConfig;
}
