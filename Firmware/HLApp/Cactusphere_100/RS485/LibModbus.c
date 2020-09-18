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
#include "ModbusDevConfig.h"

#include "vector.h"

const char ModbusDevConfigKey[] = "ModbusDevConfig";
const char BaudrateKey[] = "baudrate";
const char ParityBitKey[] = "parity";
const char StopBitKey[] = "stop";

#define MIN_BAUDRATE 1200
#define MAX_BAUDRATE 125200

static const char ModbusParityKey[PARITY_NUM][5] = {
    "None", "Odd", "Even"
};

static vector sModbusVec = NULL;

// Add ModbusDev 
static void Libmodbus_AddModbusDev(int devID, int boud, uint8_t parity, uint8_t stop) {
    ModbusDev* modbusDev;
    modbusDev = ModbusDev_NewModbusRTU(devID, boud, parity, stop);
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
    bool ret = true;

    for (unsigned int i = 0, n = json->u.object.length; i < n; ++i) {
        if (0 == strcmp(ModbusDevConfigKey, json->u.object.values[i].name)) {
            configJson = json->u.object.values[i].value;
            break;
        }
    }

    if (configJson == NULL) {
        ret = false;
        goto end;
    }

    for (unsigned int i = 0, n = configJson->u.object.length; i < n; ++i) {
        int devId;
        int baudrate = 0;
        uint8_t parity = 0;
        uint8_t stop = 1;
        char *e;
        json_value* configItem = configJson->u.object.values[i].value;

        devId = strtol(configJson->u.object.values[i].name, &e, 16);

        if (devId == 0) {
            ret = false;
            continue;
        }

        for (unsigned int p = 0, q = configItem->u.object.length; p < q; ++p) {
            if (0 == strcmp(configItem->u.object.values[p].name, BaudrateKey)) {
                json_value* item = configItem->u.object.values[p].value;
                uint32_t value;

                if (json_GetNumericValue(item, &value, 16)) {
                    baudrate = (int)value;
                }
            } else if (0 == strcmp(configItem->u.object.values[p].name, ParityBitKey)) {
                json_value* item = configItem->u.object.values[p].value;

                for (uint8_t j = 0; j < PARITY_NUM; j++) {
                    if (0 == strcmp(item->u.string.ptr, ModbusParityKey[j])) {
                        parity = j;
                        break;
                    }
                }
            } else if (0 == strcmp(configItem->u.object.values[p].name, StopBitKey)) {
                json_value* item = configItem->u.object.values[p].value;
                uint32_t value;

                if (json_GetNumericValue(item, &value, 16)) {
                    if (value == STOPBITS_ONE || value == STOPBITS_TWO) {
                        stop = (uint8_t)value;
                    }
                }
            }
        }

        if (baudrate < MIN_BAUDRATE || baudrate > MAX_BAUDRATE) {
            ret = false;
        } else {
            Libmodbus_AddModbusDev(devId, baudrate, parity, stop);
        }
    }

end:
    return ret;
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
bool Libmodbus_WriteRegister(ModbusDev* me, int regAddr, int funcCode, unsigned short* data) {
    return ModbusDev_WriteRegister(me, regAddr, funcCode, *data);
}

// Get RTApp Version
bool Libmodbus_GetRTAppVersion(char* rtAppVersion) {
    return ModbusDev_GetRTAppVersion(rtAppVersion);
}
