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

#include "LibModbusTcp.h"

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "ModbusTcpDev.h"

#include "vector.h"

const char ModbusTcpConfigKey[] = "ModbusTcpConfig";
extern const char PortKey[];

static vector sModbusTcpVec = NULL;

// Add ModbusTcpDev
static void LibmodbusTcp_AddModbusDev(char* ip, int port) {
    ModbusTcpDev* modbusDev;
    modbusDev = ModbusTcpDev_NewModbusTCP(ip, port);
    vector_add_last(sModbusTcpVec, modbusDev);
}

// Initialization
void LibmodbusTcp_ModbusDevInitialize(void) {
    sModbusTcpVec = ModbusTcpDev_Initialize();
}

// Destroy
void LibmodbusTcp_ModbusDevDestroy(void) {
    if (sModbusTcpVec != NULL) {
        LibmodbusTcp_ModbusDevClear();
        ModbusTcpDev_Destroy(sModbusTcpVec);
        vector_destroy(sModbusTcpVec);
    }
}

// Clear
void LibmodbusTcp_ModbusDevClear(void) {
    if (sModbusTcpVec != NULL) {
        vector_clear(sModbusTcpVec);
    }
}

// Regist
bool LibmodbusTcp_LoadFromJSON(const json_value* json) {
    json_value* configJson = NULL;

    configJson = json_GetKeyJson((unsigned char*)ModbusTcpConfigKey, (json_value*)json);

    if (configJson == NULL) {
        return false;
    }

    for (unsigned int i = 0, n = configJson->u.object.length; i < n; ++i) {
        char ip[16];
        int port;
        char* e;
        json_value* configItem = configJson->u.object.values[i].value;

        memcpy(ip, configJson->u.object.values[i].name, sizeof(ip));

        if (strlen((const char*)ip) == 0) {
            return false;
        }

        for (unsigned int p = 0, q = configItem->u.object.length; p < q; ++p) {
            if (0 == strcmp(configItem->u.object.values[p].name, PortKey)) { 
                json_value* item = configItem->u.object.values[p].value;

                if (item->type == json_integer) {
                    port = (int)item->u.integer;
                }
                else if (item->type == json_string) {
                    port = strtol(item->u.string.ptr, &e, 16);
                }
                break;
            }
        }
        if (port == 0) {
            return false;
        }
        LibmodbusTcp_AddModbusDev(ip, port);
    }

    return true;
}

ModbusTcpDev* LibmodbusTcp_GetAndConnectLib(char* id) {
    ModbusTcpDev* modbusDevP = ModbusTcpDev_GetModbusDev(id, sModbusTcpVec);

    if (modbusDevP == NULL) {
        return NULL;
    }

    if (!ModbusTcpDev_Connect(modbusDevP)) {
        return NULL;
    }

    return modbusDevP;
}

void 
LibmodbusTcp_Disconnect(ModbusTcpDev* me)
{
    ModbusTcpDev_Disconnect(me);
}

bool LibmodbusTcp_ReadRegister(ModbusTcpDev* me, int unitId, int regAddr, unsigned short* dst) {
    return ModbusTcpDev_ReadSingleRegister(me, unitId, regAddr, dst);
}
bool LibmodbusTcp_WriteRegister(ModbusTcpDev* me, int unitId, int regAddr, unsigned short* data) {
    return ModbusTcpDev_WriteSingleRegister(me, unitId, regAddr, *data);
}
