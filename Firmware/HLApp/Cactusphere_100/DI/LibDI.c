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

#include "LibDI.h"
#include "SendRTApp.h"
#include "DIDriveMsg.h"

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

const int pinIDs[] = { 0, 1, 2, 3 };

// Initialization and cleanup
bool DI_Lib_Initialize(void)
{
    return true;
}

void DI_Lib_Cleanup(void)
{
    // do nothing
}

bool 
DI_Lib_ConfigPulseCounter(unsigned long pinId, bool isPulseHigh,
    unsigned long minPulseWidth, unsigned long maxPulseCount)
{
    unsigned char sendMessage[256];
    DI_DriverMsg* msg = (DI_DriverMsg*)sendMessage;
    int msgSize;
    int ret = 0;

    memset(msg, 0, sizeof(DI_DriverMsg));
    msg->header.requestCode = DI_SET_CONFIG_AND_START;
    msg->header.messageLen = sizeof(DI_MsgSetConfig);
    msg->body.setConfig.pinId = pinId;
    msg->body.setConfig.isPulseHigh = isPulseHigh;
    msg->body.setConfig.minPulseWidth = minPulseWidth;
    msg->body.setConfig.maxPulseCount = maxPulseCount;
    msgSize = (int)(sizeof(msg->header) + msg->header.messageLen);
    SendRTApp_SendMessageToRTCoreAndReadMessage((const unsigned char*)msg, msgSize,
        (unsigned char*)&ret, sizeof(ret));

    return ret;
}

bool 
DI_Lib_ResetPulseCount(unsigned long pinId, unsigned long initVal)
{
    unsigned char sendMessage[256];
    DI_DriverMsg* msg = (DI_DriverMsg*)sendMessage;
    int msgSize;
    int ret = 0;

    memset(msg, 0, sizeof(DI_DriverMsg));
    msg->header.requestCode = DI_PULSE_COUNT_RESET;
    msg->header.messageLen = sizeof(DI_MsgResetPulseCount);
    msg->body.resetPulseCount.pinId = pinId;
    msg->body.resetPulseCount.initVal = initVal;
    msgSize = (int)(sizeof(msg->header) + msg->header.messageLen);
    SendRTApp_SendMessageToRTCoreAndReadMessage((const unsigned char*)msg, msgSize,
        (unsigned char*)&ret, sizeof(ret));

    return ret;
}

bool
DI_Lib_ReadPulseCount(unsigned long pinId, unsigned long* outVal)
{
    unsigned char sendMessage[256];
    DI_DriverMsg* msg = (DI_DriverMsg*)sendMessage;
    unsigned char val[4] = { 0 };
    int msgSize;
    bool ret = false;

    memset(msg, 0, sizeof(DI_DriverMsg));
    msg->header.requestCode = DI_READ_PULSE_COUNT;
    msg->header.messageLen = sizeof(DI_MsgPinId);
    msg->body.pinId.pinId = pinId;
    msgSize = (int)(sizeof(msg->header) + msg->header.messageLen);
    ret = SendRTApp_SendMessageToRTCoreAndReadMessage((const unsigned char*)msg, msgSize,
        val, sizeof(val));
    memcpy(outVal, val, sizeof(int));

    return ret;
}

bool	
DI_Lib_ReadDutySumTime(unsigned long pinId, unsigned long* outSecs)
{
    unsigned char sendMessage[256];
    DI_DriverMsg* msg = (DI_DriverMsg*)sendMessage;
    unsigned char val[4] = { 0 };
    int msgSize;
    bool ret = false;

    memset(msg, 0, sizeof(DI_DriverMsg));
    msg->header.requestCode = DI_READ_DUTY_SUM_TIME;
    msg->header.messageLen = sizeof(DI_MsgPinId);
    msg->body.pinId.pinId = pinId;
    msgSize = (int)(sizeof(msg->header) + msg->header.messageLen);
    ret = SendRTApp_SendMessageToRTCoreAndReadMessage((const unsigned char*)msg, msgSize,
        val, sizeof(val));
    memcpy(outSecs, val, sizeof(int));

    return ret;
}

bool
DI_Lib_ReadLevels(int outLevels[NUM_DI])
{
    unsigned char sendMessage[256];
    unsigned char readMessage[272];
    DI_DriverMsg* msg = (DI_DriverMsg*)sendMessage;
    DI_ReturnMsg* retMsg = (DI_ReturnMsg*)readMessage;
    int msgSize;
    bool ret = false;

    memset(msg, 0, sizeof(DI_DriverMsg));
    msg->header.requestCode = DI_READ_PULSE_LEVEL;
    msg->header.messageLen = 0;
    msgSize = (int)(sizeof(msg->header) + msg->header.messageLen);
    ret = SendRTApp_SendMessageToRTCoreAndReadMessage((const unsigned char*)msg, msgSize,
        (unsigned char*)retMsg, sizeof(DI_ReturnMsg));
    for (int i = 0; i < NUM_DI; i++) {
        outLevels[i] = retMsg->message.levels[i];
    }

    return ret;
}

bool
DI_Lib_ReadPinLevel(unsigned long pinId, unsigned int* outVal)
{
    unsigned char sendMessage[256];
    DI_DriverMsg* msg = (DI_DriverMsg*)sendMessage;
    unsigned char val[4] = { 0 };
    int msgSize;
    bool ret = false;

    memset(msg, 0, sizeof(DI_DriverMsg));
    msg->header.requestCode = DI_READ_PIN_LEVEL;
    msg->header.messageLen = sizeof(DI_MsgPinId);
    msg->body.pinId.pinId = pinId;
    msgSize = (int)(sizeof(msg->header) + msg->header.messageLen);
    ret = SendRTApp_SendMessageToRTCoreAndReadMessage((const unsigned char*)msg, msgSize,
        val, sizeof(val));
    memcpy(outVal, val, sizeof(int));

    return ret;
}

bool
DI_Lib_ReadRTAppVersion(char* rtAppVersion)
{
    unsigned char sendMessage[256];
    unsigned char readMessage[272];
    DI_DriverMsg* msg = (DI_DriverMsg*)sendMessage;
    DI_ReturnMsg* retMsg = (DI_ReturnMsg*)readMessage;
    int msgSize;
    bool ret = false;

    memset(msg, 0, sizeof(DI_DriverMsg));
    msg->header.requestCode = DI_READ_VERSION;
    msg->header.messageLen = 0;
    msgSize = (int)(sizeof(msg->header) + msg->header.messageLen);
    ret = SendRTApp_SendMessageToRTCoreAndReadMessage((const unsigned char*)msg, msgSize,
        (unsigned char*)retMsg, sizeof(DI_ReturnMsg));
    strncpy(rtAppVersion, retMsg->message.version, retMsg->messageLen);

    return ret;
}
