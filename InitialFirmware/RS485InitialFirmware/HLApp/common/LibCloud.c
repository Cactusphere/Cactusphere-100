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

#include "LibCloud.h"

#include <errno.h>
#include <stdio.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/networking.h>

#include <iothub_client_core_common.h>
#include <iothub_device_client_ll.h>
#include <iothub_client_options.h>
#include <iothubtransportmqtt.h>
#include <iothub.h>
#include <azure_sphere_provisioning.h>

#include "vector.h"

#include "TelemetryItemCache.h"
#include "TelemetryItems.h"

#define RESEND_MAX_NUM	10

extern IOTHUB_DEVICE_CLIENT_LL_HANDLE Get_IOTHUB_DEVICE_CLIENT_LL_HANDLE(void); // main.c

static int  IoT_CentralLib_FindWaitingMsg(IOTHUB_MESSAGE_HANDLE msgHandle);

typedef struct TelemetryMsgInfo {
    IOTHUB_MESSAGE_HANDLE   msgHandle;
    uint32_t    timeStamp;
} TelemetryMsgInfo;

static IOTHUB_DEVICE_CLIENT_LL_HANDLE sIothubClientHandle = NULL;
static TelemetryItemCache*	sTelemetryCache = NULL;
static TelemetryItems*	sTelemetryItems = NULL;
static vector   sWaitingMsgs = NULL;
static time_t	sBaseTime;

/// <summary>
///     Callback confirming message delivered to IoT Hub.
/// </summary>
/// <param name="result">Message delivery status</param>
/// <param name="context">User specified context</param>
static void
SendMessageCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *context)
{
    int theIndex =
        IoT_CentralLib_FindWaitingMsg((IOTHUB_MESSAGE_HANDLE)context);

    Log_Debug("INFO: Message received by IoT Hub. Result is: %d\n", result);
    if (0 <= theIndex) {
        if (IOTHUB_CLIENT_CONFIRMATION_OK != result) {
            const TelemetryMsgInfo*   theMsg =
                (TelemetryMsgInfo*)vector_get_data(sWaitingMsgs) + theIndex;
            const char* jsonStr = IoTHubMessage_GetString(theMsg->msgHandle);

            if (TelemetryItems_LoadFromJson(sTelemetryItems, jsonStr)) {
                (void)TelemetryItemCache_EnqueueItems(
                    sTelemetryCache, sTelemetryItems, theMsg->timeStamp);
            }
        }
        vector_remove_at(sWaitingMsgs, theIndex);
    } else {
        Log_Debug("WARN: Unknown essage on  SendMessageCallback().\n");
    }
    IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)context);
}

static uint32_t
GetTimestamp(void)
{
    time_t	currTime = time(NULL);

    return (uint32_t)(currTime - sBaseTime);
}

static void
MakeDateTimeStr(char* strBuf, size_t bufSize, uint32_t timeStamp)
{
    time_t	theTime = sBaseTime + (time_t)timeStamp;
    struct tm*	tmVal;

    tmVal = gmtime(&theTime);
    theTime -= tmVal->__tm_gmtoff;
    tmVal = localtime(&theTime);
    strftime(strBuf, bufSize, "%Y-%m-%dT%H:%M:%S", tmVal);
    sprintf(strBuf + strlen(strBuf), ".%07ldZ", 0);
//
// NOTE: Since the timestamp is a 32-bit integer, time values less than 
//       seconds (timespec::tv_nsec) that can be obtained by timespec_get() 
//       are ignored, and time values less than seconds are set to 0 
//       in the character string returned as an output argument.
}

static int
IoT_CentralLib_FindWaitingMsg(IOTHUB_MESSAGE_HANDLE msgHandle)
{
    TelemetryMsgInfo*   curs =
        (TelemetryMsgInfo*)vector_get_data(sWaitingMsgs);

    for (int i = 0, n = vector_size(sWaitingMsgs); i < n; ++i) {
        if ((curs++)->msgHandle == msgHandle) {
            return i;
        }
    }

    return -1;
}

static bool
IoT_CentralLib_DoSendTelemetry(const char* jsonStr, uint32_t timeStamp)
{
    // send telemetry data message to IoT Central with timestamp property
    bool	isOK = true;
    char	strBuf[64];
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString(jsonStr);
    TelemetryMsgInfo    msgInfo;

    if (messageHandle == 0) {
        Log_Debug("WARNING: unable to create a new IoTHubMessage\n");
        return false;
    }
    MakeDateTimeStr(strBuf, sizeof(strBuf), timeStamp);
    IoTHubMessage_SetProperty(messageHandle, "iothub-creation-time-utc", strBuf);
    msgInfo.msgHandle = messageHandle;
    msgInfo.timeStamp = timeStamp;
    vector_add_last(sWaitingMsgs, &msgInfo);
    if (IoTHubDeviceClient_LL_SendEventAsync(
            sIothubClientHandle, messageHandle, SendMessageCallback, messageHandle)
        != IOTHUB_CLIENT_OK) {
        int theIndex = IoT_CentralLib_FindWaitingMsg(messageHandle);

        isOK = false;
        if (0 <= theIndex) {
            (void)vector_remove_at(sWaitingMsgs, theIndex);
            IoTHubMessage_Destroy(messageHandle);
        }
        Log_Debug("WARNING: failed to hand over the message to IoTHubClient\n");
    } else {
        Log_Debug("INFO: IoTHubClient accepted the message for delivery\n");
    }

    return isOK;
}

// Initialization and cleanup
bool
IoT_CentralLib_Initialize(
    uint32_t cachBufSize, bool clearCache)
{
    if (clearCache && NULL != sTelemetryCache) {
        TelemetryItemCache_Destroy(sTelemetryCache);
        sTelemetryCache = NULL;
    }
    if (NULL == sTelemetryCache) {
        sTelemetryCache = TelemetryItemCache_New();
        if (NULL != sTelemetryCache) {
            TelemetryItemCache_Init(sTelemetryCache,
                NULL, cachBufSize);
        }
        sBaseTime = time(NULL);
    }

    if (NULL == sTelemetryItems) {
        sTelemetryItems = TelemetryItems_New();
        if (NULL == sTelemetryItems) {
            return false;
        }
    }

    if (NULL == sWaitingMsgs) {
        sWaitingMsgs = vector_init(sizeof(TelemetryMsgInfo));
    } else if (0 != vector_size(sWaitingMsgs)) {
        TelemetryMsgInfo* curs =
            (TelemetryMsgInfo*)vector_get_data(sWaitingMsgs);

        for (int i = 0, n = vector_size(sWaitingMsgs); i < n; ++i) {
            IoTHubMessage_Destroy(curs->msgHandle);
            ++curs;
        }
    }
    sIothubClientHandle = Get_IOTHUB_DEVICE_CLIENT_LL_HANDLE();

    return (sIothubClientHandle != NULL);
}

void
IoT_CentralLib_Cleanup(void)
{
    if (NULL != sWaitingMsgs) {
        TelemetryMsgInfo*   curs =
            (TelemetryMsgInfo*)vector_get_data(sWaitingMsgs);

        for (int i = 0, n = vector_size(sWaitingMsgs); i < n; ++i) {
            IoTHubMessage_Destroy(curs->msgHandle);
            ++curs;
        }
        vector_destroy(sWaitingMsgs);
        sWaitingMsgs = NULL;
    }
    if (NULL != sTelemetryCache) {
        TelemetryItemCache_Destroy(sTelemetryCache);
        sTelemetryCache = NULL;
    }
    if (NULL != sTelemetryItems) {
        TelemetryItems_Destroy(sTelemetryItems);
        sTelemetryItems = NULL;
    }
}

// Send telemetry data
bool
IoT_CentralLib_SendTelemetry(const char* jsonStr, uint32_t* outTimestamp)
{
    uint32_t	timeStamp = GetTimestamp();

    *outTimestamp = timeStamp;

    return IoT_CentralLib_DoSendTelemetry(jsonStr, timeStamp);
}

// Telemetry data caching during network down
bool
IoT_CentralLib_CheckConnection(void)
{
    bool	isOK = false;

    (void)Networking_IsNetworkingReady(&isOK);

    return isOK;
}

bool
IoT_CentralLib_EnqueueTelemtryItemsToCache(
    const TelemetryItems* telemetryItems, uint32_t timeStamp)
{
    return TelemetryItemCache_EnqueueItems(sTelemetryCache,
        telemetryItems, timeStamp);
}

bool
IoT_CentralLib_HasCachedTelemetryItems(void)
{
    return (! TelemetryItemCache_IsEmpty(sTelemetryCache));
}

bool
IoT_CentralLib_ResendCachedTelemetryItems(void)
{
    // send cached telemetry data up to RESEND_MAX_NUM at a time
    if (TelemetryItemCache_IsEmpty(sTelemetryCache)) {
        return true;
    }

    for (int i = 0; i < RESEND_MAX_NUM; ++i) {
        uint32_t	timeStamp;
        const char* jsonStr;

        (void)TelemetryItemCache_DequeueItemsTo(
            sTelemetryCache, sTelemetryItems, &timeStamp);
        jsonStr = TelemetryItems_ToJson(sTelemetryItems);
        if (! IoT_CentralLib_DoSendTelemetry(jsonStr, timeStamp)) {
            return false;  // error
        }
        TelemetryItems_Clear(sTelemetryItems);

        if (TelemetryItemCache_IsEmpty(sTelemetryCache)) {
            break;
        }
    }

    return true;
}

uint32_t
IoT_CentralLib_GetTmeStamp(void)
{
    return GetTimestamp();
}

void IoT_CentralLib_SendProperty(const char* jsonStr)
{
    Log_Debug("Sending IoT Hub Message: %s\n", jsonStr);

    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString(jsonStr);

    if (messageHandle == 0) {
        Log_Debug("WARNING: unable to create a new IoTHubMessage\n");
        return;
    }

    if (IoTHubDeviceClient_LL_SendReportedState(sIothubClientHandle, jsonStr, strlen(jsonStr), NULL, 0) != IOTHUB_CLIENT_OK) {
        Log_Debug("WARNING: failed to hand over the message to IoTHubClient\n");
    }
    else {
        //Log_Debug("INFO: IoTHubClient accepted the message for delivery\n");
    }

    IoTHubMessage_Destroy(messageHandle);
}
