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

#include "InterCoreComm.h"

#include <stddef.h>  // for NULL
#include <string.h>

#include "mt3620-intercore.h"

#include "TimerUtil.h"

static BufferHeader*	sOutboundBuf = NULL;
static BufferHeader*	sInboundBuf  = NULL;
static uint32_t	sRingBufSize;
static unsigned char	sRecvBuf[MAX_UART_WRITE_LEN * 2];  // 512
static UART_DriverMsg*	sDriverMsgBuf = NULL;

static bool
InterCoreComm_SendData(const uint8_t* data, uint16_t len)
{
    memcpy(sRecvBuf + 20, data, len);

    return (0 == EnqueueData(sInboundBuf, sOutboundBuf, sRingBufSize,
        sRecvBuf, 20 + len));
}

// Initialization
bool
InterCoreComm_Initialize()
{
    if (0 != GetIntercoreBuffers(
            &sOutboundBuf, &sInboundBuf, &sRingBufSize)) {
        return false;
    }
    sDriverMsgBuf = (UART_DriverMsg*)(sRecvBuf + 20);  // GUID(16[Byte]) + reserved(4[Byte]) prefix

    return true;
}

// Wait and receive request from HLApp
const UART_DriverMsg*
InterCoreComm_WaitAndRecvRequest()
{
    uint32_t	dataSize;
    UART_DriverMsgHdr*	msgHdr;

    // wait request message arrives while sleep
    while (true) {
        dataSize = sizeof(sRecvBuf);
        if (0 == DequeueData(sOutboundBuf, sInboundBuf,
                sRingBufSize, sRecvBuf, &dataSize)) {
            break;
        }
        TimerUtil_SleepUntilIntr();
    }
    msgHdr = &sDriverMsgBuf->header;

    // check the received message's integrity
    if (dataSize <= sizeof(UART_DriverMsgHdr)) {
        return NULL;  // too short message
    }
    switch (msgHdr->requestCode) {
    case UART_REQ_WRITE_AND_READ:
        if (msgHdr->messageLen != ((sizeof(uint16_t) * 2) +
                sDriverMsgBuf->body.writeAndReadReq.writeLen)) {
            return NULL;  // invalid length
        }
        break;
    case UART_REQ_SET_PARAMS:
        if (msgHdr->messageLen != sizeof(UART_MsgSetParams)) {
            return NULL;  // invalid length
        }
        break;
    default:
        return NULL;  // unknown messagse
    }

    return sDriverMsgBuf;
}

// Send UART received data to HLApp
bool
InterCoreComm_SendReadData(const uint8_t* data, uint16_t len)
{
    if (len > sizeof(sRecvBuf) - 20) {
        return false;
    }
    return InterCoreComm_SendData(data, len);
}

bool
InterCoreComm_SendIntValue(int val)
{
    return InterCoreComm_SendData((uint8_t*)&val, sizeof(val));
}
