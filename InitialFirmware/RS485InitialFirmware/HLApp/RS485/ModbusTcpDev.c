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

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "ModbusTcpDev.h"
#include "ModbusTCP.h"
#include "vector.h"

// ModbusTcpDev structure
typedef struct ModbusTcpDev {
    ModbusTcpCtx* ctx;
    char id[21]; // 255.255.255.255:1111
}ModbusTcpDev;

// Initialization and cleanup
vector 
ModbusTcpDev_Initialize() {
    return vector_init(sizeof(ModbusTcpDev));
}

void
ModbusTcpDev_Destroy(vector modbusDevVec) {
    ModbusTcpDev* modbusDev = vector_get_data(modbusDevVec);
    for (int i = 0, n = vector_size(modbusDevVec); i < n; ++i) {
        ModbusTCP_Destroy(modbusDev->ctx);
        free(modbusDev);
        modbusDev++;
    }
}

// Create Modbus TCP
ModbusTcpDev* 
ModbusTcpDev_NewModbusTCP(char* ip, int port) {
    ModbusTcpDev* newObj;
    
    newObj = (ModbusTcpDev*)malloc(sizeof(ModbusTcpDev));
    newObj->ctx = ModbusTCP_Initialize(ip, port);

    sprintf(newObj->id, "%s:%d", ip, port);

    return newObj;
}

// Get ModubsTcpDev* 
ModbusTcpDev* 
ModbusTcpDev_GetModbusDev(const char* id, vector modbusDevVec) {
    ModbusTcpDev* modbusDev = vector_get_data(modbusDevVec);
    for (int i = 0, n = vector_size(modbusDevVec); i < n; ++i) {
        if (0 == strcmp(modbusDev->id, id)) {
            return modbusDev;
        }
        modbusDev++;
    }
    return NULL;
}

// Connect
bool 
ModbusTcpDev_Connect(ModbusTcpDev* me) {
    return ModbusTCP_Connect(me->ctx);
}

// Disconnect
void 
ModbusTcpDev_Disconnect(ModbusTcpDev* me) {
    ModbusTCP_Disconnect(me->ctx);
}

// Read single register
bool 
ModbusTcpDev_ReadSingleRegister(ModbusTcpDev* me, int unitId, int regAddr, unsigned short* dst) {
    return ModbusTCP_ReadSingleRegister(me->ctx, unitId, regAddr, dst);
}

bool
ModbusTcpDev_ReadSingleInputRegister(ModbusTcpDev* me, int unitId, int regAddr, unsigned short* dst) {
    return ModbusTCP_ReadSingleInputRegister(me->ctx, unitId, regAddr, dst);
}

// Write single register
bool
ModbusTcpDev_WriteSingleRegister(ModbusTcpDev* me, int unitId, int regAddr, uint16_t value) {
    return ModbusTCP_WriteSingleRegister(me->ctx, unitId, regAddr, value);
}
