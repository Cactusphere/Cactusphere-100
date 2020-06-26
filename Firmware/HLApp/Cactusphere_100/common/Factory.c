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

#include "Factory.h"
#include "cactusphere_eeprom.h"

#if (APP_PRODUCT_ID == PRODUCT_ATMARK_TECHNO_DIN)
#include "DI_DataFetchScheduler.h"
#include "DI_FetchTimers.h"
#define USE_DI
#endif
#if (APP_PRODUCT_ID == PRODUCT_ATMARK_TECHNO_RS485)
#include "ModbusDataFetchScheduler.h"
#include "ModbusTcpDataFetchScheduler.h"
#define USE_MODBUS
#define USE_MODBUS_TCP
#endif

DataFetchSchedulerBase*
Factory_CreateScheduler(IO_Feature feature)
{
    DataFetchSchedulerBase* newObj = NULL;

    switch (feature) {
#if defined(USE_MODBUS) || defined(USE_MODBUS_TCP)
    case MODBUS_RTU:
        newObj = ModbusDataFetchScheduler_New();
        break;
    case MODBUS_TCP:
        newObj = ModbusTcpDataFetchScheduler_New();
        break;
#endif
#ifdef USE_DI
    case DIGITAL_IN:
        newObj = DI_DataFetchScheduler_New();
        break;
#endif
    default:
        return NULL;  // unknown feature
    }

    return newObj;
}

FetchTimers*
Factory_CreateFetchTimers(IO_Feature feature,
    FetchTimerCallback cbProc, void* cbArg)
{
    FetchTimers*	newObj = NULL;

    switch (feature) {
#if defined(USE_MODBUS) || defined(USE_MODBUS_TCP)
    case MODBUS_RTU:
    case MODBUS_TCP:
        newObj = FetchTimers_New(cbProc, cbArg);
        break;
#endif
#ifdef USE_DI
    case DIGITAL_IN:
        newObj = FetchTimers_New(cbProc, cbArg);
        newObj->InitForTimer = DI_FetchTimers_InitForTimer;
        break;
#endif
    default:
        return NULL;  // unknown feature
    }

    return newObj;
}
