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

#ifndef _MODBUS_TCP_DEV_H_
#define _MODBUS_TCP_DEV_H_

#include <stdbool.h>
#include <stdint.h>
#include "vector.h"

typedef struct ModbusTcpDev ModbusTcpDev;

// Initialization and cleanup
extern vector ModbusTcpDev_Initialize(void);
extern void ModbusTcpDev_Destroy(vector modbusDevVec);

// Create Modbus TCP 
extern ModbusTcpDev* ModbusTcpDev_NewModbusTCP(char* ip, int port);

// Get ModbusDev*
extern ModbusTcpDev* ModbusTcpDev_GetModbusDev(const char* id, vector modbusTcpDevVec);

// Connect
extern bool ModbusTcpDev_Connect(ModbusTcpDev* me);

// Disconnect
extern void ModbusTcpDev_Disconnect(ModbusTcpDev* me);

// Read 1byte holding register
extern bool ModbusTcpDev_ReadSingleRegister(ModbusTcpDev* me, int unitId, int regAddr, unsigned short* dst);

// Read 1byte input register
extern bool ModbusTcpDev_ReadSingleInputRegister(ModbusTcpDev* me, int unitId, int regAddr, unsigned short* dst);

// Write 1byte
extern bool ModbusTcpDev_WriteSingleRegister(ModbusTcpDev* me, int unitId, int regAddr, uint16_t value);
#endif  // _MODBUS_TCP_DEV_H_
