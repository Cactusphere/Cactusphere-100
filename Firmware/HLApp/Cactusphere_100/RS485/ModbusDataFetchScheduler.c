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

#include "ModbusDataFetchScheduler.h"

#include "LibModbus.h"
#include "ModbusFetchItem.h"
#include "ModbusFetchTargets.h"
#include "StringBuf.h"
#include "TelemetryItems.h"

typedef struct ModbusDataFetchScheduler {
    DataFetchSchedulerBase	Super;

    // data member
    ModbusFetchTargets*	mFetchTargets;  // acquisition targets of Modbus RTU
} ModbusDataFetchScheduler;

//
// DataFetchScheduler's private procedure/method
//
// Callback procedure of FetchTimers
static void
ModbusFetchTimerCallback(void* arg, const FetchItemBase* fetchTarget)
{
    ModbusDataFetchScheduler* scheduler = (ModbusDataFetchScheduler*)arg;

    ModbusFetchTargets_Add(
        scheduler->mFetchTargets, (const ModbusFetchItem*)fetchTarget);
}

// Virtual method
static void
ModbusDataFetchScheduler_DoDestroy(DataFetchSchedulerBase* me)
{
    ModbusDataFetchScheduler*	self = (ModbusDataFetchScheduler*)me;

    ModbusFetchTargets_Destroy(self->mFetchTargets);
}

static void
ModbusDataFetchScheduler_ClearFetchTargets(DataFetchSchedulerBase* me)
{
    ModbusDataFetchScheduler* self = (ModbusDataFetchScheduler*)me;

    ModbusFetchTargets_Clear(self->mFetchTargets);
}

static void
ModbusDataFetchScheduler_DoSchedule(DataFetchSchedulerBase* me)
{
    ModbusDataFetchScheduler* self = (ModbusDataFetchScheduler*)me;
    vector	devIDs;

    devIDs = ModbusFetchTargets_GetDevIDs(self->mFetchTargets);
    if (!vector_is_empty(devIDs)) {
        unsigned long* devIDCurs = (unsigned long*)vector_get_data(devIDs);

        for (int i = 0, n = vector_size(devIDs); i < n; i++) {
            unsigned long	devID = *devIDCurs++;
            vector	fetchItems = ModbusFetchTargets_GetFetchItems(
                self->mFetchTargets, devID);
            const ModbusFetchItem** fiCurs =
                (const ModbusFetchItem**)vector_get_data(fetchItems);

            ModbusDev* modbusdev = Libmodbus_GetAndConnectLib((int)devID);

            if (modbusdev == NULL) {
                continue;
            }

            for (int j = 0, m = vector_size(fetchItems); j < m; ++j) {
                const ModbusFetchItem* item = *fiCurs++;
                unsigned short readVal[2] = { 0 };

                if (!Libmodbus_ReadRegister(modbusdev, (int)item->regAddr, (int)item->funcCode, readVal, (int)item->regCount)) {
                    // error!
                    continue;
                }

                unsigned long tmpVal  = 0;
                if (item->regCount == 2) {
                    tmpVal = (unsigned long)((readVal[0] << 16) + readVal[1]);
                } else {
                    tmpVal = readVal[0];
                }

                if (item->asFloat) {
                    double fVal = tmpVal;

                    fVal += item->offset;
                    if (item->multiplier != 0) {
                        fVal *= item->multiplier;
                    }
                    if (item->devider != 0) {
                        fVal /= item->devider;
                    }
                    StringBuf_AppendByPrintf(me->mStringBuf, "%f", fVal);
                } else {
                    unsigned long ulVal = tmpVal;

                    ulVal += item->offset;
                    if (item->multiplier != 0) {
                        ulVal *= item->multiplier;
                    }
                    if (item->devider != 0) {
                        ulVal /= item->devider;
                    }

                    StringBuf_AppendByPrintf(me->mStringBuf, "%ld", ulVal);
                }

                TelemetryItems_Add(me->mTelemetryItems,
                    item->telemetryName, StringBuf_GetStr(me->mStringBuf));
                StringBuf_Clear(me->mStringBuf);
            }
        }
    }
}

DataFetchScheduler*
ModbusDataFetchScheduler_New(void)
{
    ModbusDataFetchScheduler* newObj =
        (ModbusDataFetchScheduler*)malloc(sizeof(ModbusDataFetchScheduler));
    DataFetchSchedulerBase* super;

    if (NULL != newObj) {
        super = &newObj->Super;
        if (NULL == DataFetchScheduler_InitOnNew(
            super, ModbusFetchTimerCallback, MODBUS_RTU)) {
            goto err;
        }
        newObj->mFetchTargets = ModbusFetchTargets_New();
        if (NULL == newObj->mFetchTargets) {
            goto err_delete_super;
        }
    }

    super->DoDestroy = ModbusDataFetchScheduler_DoDestroy;
//	super->DoInit    = ModbusDataFetchScheduler_DoInit;  // don't override
    super->ClearFetchTargets = ModbusDataFetchScheduler_ClearFetchTargets;
    super->DoSchedule        = ModbusDataFetchScheduler_DoSchedule;

    return super;
err_delete_super:
    DataFetchScheduler_Destroy(super);
err:
    free(newObj);
    return NULL;
}
