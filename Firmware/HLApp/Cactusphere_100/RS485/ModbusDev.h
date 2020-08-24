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

#ifndef _MODBUS_DEV_H_
#define _MODBUS_DEV_H_

#include <stdbool.h>

#include "json.h"
#include "vector.h"

typedef struct ModbusDev ModbusDev;

// Initialization and cleanup
extern vector ModbusDev_Initialize(void);
extern void ModbusDev_Destroy(vector modbusDevVec);

// Create Modbus RTU
extern ModbusDev* ModbusDev_NewModbusRTU(int devId, int baud);

// Get ModbusDev*
extern ModbusDev* ModbusDev_GetModbusDev(int devID, vector modbusDevVec);

// Connect
extern bool ModbusDev_Connect(ModbusDev* me);

// Read status/register
extern bool ModbusDev_ReadRegister(ModbusDev* me, int regAddr, int funcCode, unsigned short* dst, int regCount);

// Write 2byte
extern bool ModbusDev_WriteRegister(ModbusDev* me, int regAddr, int funcCode, uint16_t value);

// Get RTApp Version
extern bool ModbusDev_GetRTAppVersion(char* rtAppVersion);
#endif  // _MODBUS_DEV_H_
