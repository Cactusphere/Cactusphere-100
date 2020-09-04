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

#include "SendRTApp.h"

#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <applibs/application.h>
#include <applibs/log.h>

#include "cactusphere_product.h"

static const char rtAppComponentId[] = "b3b448f7-535f-bb7d-ce9d-7c90ad45a2b7";  // for RS485

static int sSockFd = -1;

// Initialization and cleanup
bool
SendRTApp_InitHandlers(void)
{
    // open connection to RTApp and set receive timeout time
    static const struct timeval recvTimeout =
        { .tv_sec = 5 * 10,.tv_usec = 0 };
    int result;

    sSockFd = Application_Connect(rtAppComponentId);
    if (sSockFd == -1) {
        Log_Debug("ERROR: Unable to create socket: %d (%s)\n", errno, strerror(errno));
        return false;
    }
    result = setsockopt(sSockFd,
        SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout));
    if (result == -1) {
        Log_Debug("ERROR: Unable to set socket timeout: %d (%s)\n", errno, strerror(errno));
        return false;
    }

    return true;
}

void SendRTApp_CloseHandlers(void)
{
    Log_Debug("Closing file descriptors.\n");
    if (sSockFd >= 0) {
        if (close(sSockFd) != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", "Socket", strerror(errno), errno);
        }
    }
}

// Send request message to RTApp (and receve response)
bool
SendRTApp_SendMessageToRTCore(
    const unsigned char* txMessage, long txMessageSize)
{
    int bytesSent = send(sSockFd, txMessage, (size_t)txMessageSize, 0);

    if (bytesSent == -1) {
        Log_Debug("ERROR: Unable to send message: %d (%s)\n", errno, strerror(errno));
        SendRTApp_CloseHandlers();
        return false;
    }

    return true;
}

bool
SendRTApp_SendMessageToRTCoreAndReadMessage(
    const unsigned char* txMessage, long txMessageSize,
    unsigned char* rxMessage, long rxMessageSize)
{
    int bytesReceived;

    if (! SendRTApp_SendMessageToRTCore(txMessage, txMessageSize)) {
        return false;
    }
    bytesReceived = recv(sSockFd, rxMessage, (size_t)rxMessageSize, 0);
    if (bytesReceived == -1) {
        Log_Debug("ERROR: Unable to receive message: %d (%s)\n", errno, strerror(errno));
        SendRTApp_CloseHandlers();
        return false;
    }

    return true;
}
