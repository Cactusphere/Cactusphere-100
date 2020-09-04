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

#include "ModbusTcpDataFetchScheduler.h"

#include <string.h>
#include <applibs/log.h>
#include "LibModbusTcp.h"
#include "ModbusTcpDev.h"
#include "ModbusTcpFetchItem.h"
#include "ModbusTcpFetchTargets.h"
#include "StringBuf.h"
#include "TelemetryItems.h"

typedef struct ModbusTcpDataFetchScheduler {
    DataFetchSchedulerBase	Super;

    // data member
    ModbusTcpFetchTargets*	mFetchTargets;  // acquisition targets of Modbus TCP
} ModbusTcpDataFetchScheduler;

//
// DataTcpDataFetchScheduler's private procedure/method
//
// Callback procedure of FetchTimers
static void
ModbusTcpFetchTimerCallback(void* arg, const FetchItemBase* fetchTarget)
{
    ModbusTcpDataFetchScheduler* scheduler = (ModbusTcpDataFetchScheduler*)arg;

    ModbusTcpFetchTargets_Add(
        scheduler->mFetchTargets, (const ModbusTcpFetchItem*)fetchTarget);
}

// Virtual method
static void
ModbusTcpDataFetchScheduler_DoDestroy(DataFetchSchedulerBase* me)
{
    ModbusTcpDataFetchScheduler*	self = (ModbusTcpDataFetchScheduler*)me;

    ModbusTcpFetchTargets_Destroy(self->mFetchTargets);
}

static void
ModbusTcpDataFetchScheduler_ClearFetchTargets(DataFetchSchedulerBase* me)
{
    ModbusTcpDataFetchScheduler* self = (ModbusTcpDataFetchScheduler*)me;

    ModbusTcpFetchTargets_Clear(self->mFetchTargets);
}

static void
ModbusTcpDataFetchScheduler_DoSchedule(DataFetchSchedulerBase* me)
{
    ModbusTcpDataFetchScheduler* self = (ModbusTcpDataFetchScheduler*)me;
    vector	IDs;
    IDs = ModbusTcpFetchTargets_GetDevIDs(self->mFetchTargets);
    Log_Debug("TEST\n");
    if (!vector_is_empty(IDs)) {
        char* IDCurs = (char*)vector_get_data(IDs);
        Log_Debug("TEST00\n");
        for (int i = 0, n = vector_size(IDs); i < n; i++) {

            vector	fetchItems = ModbusTcpFetchTargets_GetFetchItems(
                self->mFetchTargets, IDCurs);
            const ModbusTcpFetchItem** fiCurs =
                (const ModbusTcpFetchItem**)vector_get_data(fetchItems);

            ModbusTcpDev* modbusdev = LibmodbusTcp_GetAndConnectLib(IDCurs);

            IDCurs += 21; // MODBUS_TCP_ID_SIZE
            Log_Debug("TEST01\n");
            if (modbusdev == NULL) {
                continue;
            }
            Log_Debug("TEST02\n");

            for (int j = 0, m = vector_size(fetchItems); j < m; ++j) {
                const ModbusTcpFetchItem* item = *fiCurs++;
                unsigned short value;
                Log_Debug("TEST03\n");
                if (!LibmodbusTcp_ReadRegister(modbusdev, (int)item->unitID, (int)item->regAddr, &value)) {
                    // error!
                    Log_Debug("ERROR\n");
                    continue;
                }

                if (item->asFloat)
                {
                    double fVal = value;

                    fVal += item->offset;
                    if (item->multiplier != 0) {
                        fVal *= item->multiplier;
                    }
                    if (item->devider != 0) {
                        fVal /= item->devider;
                    }

                    StringBuf_AppendByPrintf(me->mStringBuf, "%f", fVal);
                }
                else
                {
                    unsigned long ulVal = value;

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
            LibmodbusTcp_Disconnect(modbusdev);
        }
    }
}

DataFetchScheduler*
ModbusTcpDataFetchScheduler_New(void)
{
    ModbusTcpDataFetchScheduler* newObj =
        (ModbusTcpDataFetchScheduler*)malloc(sizeof(ModbusTcpDataFetchScheduler));
    DataFetchSchedulerBase* super;

    if (NULL != newObj) {
        super = &newObj->Super;
        if (NULL == DataFetchScheduler_InitOnNew(
            super, ModbusTcpFetchTimerCallback, MODBUS_TCP)) {
            goto err;
        }
        newObj->mFetchTargets = ModbusTcpFetchTargets_New();
        if (NULL == newObj->mFetchTargets) {
            goto err_delete_super;
        }
    }

    super->DoDestroy = ModbusTcpDataFetchScheduler_DoDestroy;
//	super->DoInit    = ModbusTcpDataFetchScheduler_DoInit;  // don't override
    super->ClearFetchTargets = ModbusTcpDataFetchScheduler_ClearFetchTargets;
    super->DoSchedule        = ModbusTcpDataFetchScheduler_DoSchedule;

    return super;
err_delete_super:
    DataFetchScheduler_Destroy(super);
err:
    free(newObj);
    return NULL;
}
