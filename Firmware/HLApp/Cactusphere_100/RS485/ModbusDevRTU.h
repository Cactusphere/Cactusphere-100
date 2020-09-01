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

#ifndef _MODBUS_DEV_RTU_H_
#define _MODBUS_DEV_RTU_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct ModbusCtx ModbusCtx;

// Initialization and cleanup
extern ModbusCtx* ModbusDevRTU_Initialize(int devId, int baud, uint8_t parity, uint8_t stop);
extern void ModbusDevRTU_Destroy(ModbusCtx* me);

// Connect
extern bool ModbusDevRTU_Connect(ModbusCtx* me);

// Read status/register
extern bool ModbusDevRTU_ReadRegister(ModbusCtx* me, int regAddr, int function, unsigned short* dst, int length);

// Write 2byte
extern bool ModbusDevRTU_WriteRegister(ModbusCtx* me, int regAddr, int funcCode, unsigned short value);

// Get RTApp Version
extern bool ModbusDevRTU_GetRTAppVersion(char* rtAppVersion);
#endif  // _MODBUS_DEV_RTU_H_
