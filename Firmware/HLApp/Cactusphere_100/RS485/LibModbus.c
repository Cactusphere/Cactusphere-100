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

#include "LibModbus.h"

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "ModbusDev.h"

#include "vector.h"

const char ModbusDevConfigKey[] = "ModbusDevConfig";
const char BaudrateKey[] = "baudrate";

#define MIN_BAUDRATE 1200
#define MAX_BAUDRATE 125200

static vector sModbusVec = NULL;

// Add ModbusDev 
static void Libmodbus_AddModbusDev(int devID, int boud) {
    ModbusDev* modbusDev;
    modbusDev = ModbusDev_NewModbusRTU(devID, boud);
    vector_add_last(sModbusVec, modbusDev);
}

// Initialization
void Libmodbus_ModbusDevInitialize(void) {
    sModbusVec = ModbusDev_Initialize();
}

// Destroy
void Libmodbus_ModbusDevDestroy(void) {
    if (sModbusVec != NULL) {
        Libmodbus_ModbusDevClear();
        ModbusDev_Destroy(sModbusVec);
        vector_destroy(sModbusVec);
    }
}

// Clear
void Libmodbus_ModbusDevClear(void) {
    if (sModbusVec != NULL) {
        vector_clear(sModbusVec);
    }
}

// Regist
bool Libmodbus_LoadFromJSON(const json_value* json) {
    json_value* configJson = NULL;

    for (unsigned int i = 0, n = json->u.object.length; i < n; ++i) {
        if (0 == strcmp(ModbusDevConfigKey, json->u.object.values[i].name)) {
            configJson = json->u.object.values[i].value;
            break;
        }
    }

    if (configJson == NULL) {
        return false;
    }

    for (unsigned int i = 0, n = configJson->u.object.length; i < n; ++i) {
        int devId;
        int baudrate = 0;
        char *e;
        json_value* configItem = configJson->u.object.values[i].value;

        devId = strtol(configJson->u.object.values[i].name, &e, 16);

        if (devId == 0) {
            return false;
        }

        for (unsigned int p = 0, q = configItem->u.object.length; p < q; ++p) {
            if (0 == strcmp(configItem->u.object.values[p].name, BaudrateKey)) {
                json_value* item = configItem->u.object.values[p].value;
                uint32_t value;

                if (json_GetNumericValue(item, &value, 16)) {
                    baudrate = (int)value;
                }

                break;
            }
        }
        if (baudrate < MIN_BAUDRATE || baudrate > MAX_BAUDRATE) {
            continue;
        }
        Libmodbus_AddModbusDev(devId, baudrate);
    }

    return true;
}

ModbusDev* Libmodbus_GetAndConnectLib(int devID) {
    ModbusDev* modbusDevP = ModbusDev_GetModbusDev(devID, sModbusVec);

    if (modbusDevP == NULL) {
        return NULL;
    }

    if (!ModbusDev_Connect(modbusDevP)) {
        return NULL;
    }

    return modbusDevP;
}

bool Libmodbus_ReadRegister(ModbusDev* me, int regAddr, int funcCode, unsigned short* dst, int regCount) {
    return ModbusDev_ReadRegister(me, regAddr, funcCode, dst, regCount);
}
bool Libmodbus_WriteRegister(ModbusDev* me, int regAddr, unsigned short* data) {
    return ModbusDev_WriteSingleRegister(me, regAddr, *data);
}

// Get RTApp Version
bool Libmodbus_GetRTAppVersion(char* rtAppVersion) {
    return ModbusDev_GetRTAppVersion(rtAppVersion);
}
