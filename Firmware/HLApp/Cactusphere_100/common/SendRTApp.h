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

#ifndef _SEND_RTAPP_H_
#define _SEND_RTAPP_H_

#ifndef _STDBOOL_H
#include <stdbool.h>
#endif

// Initialization and cleanup
extern bool SendRTApp_InitHandlers(void);
extern void SendRTApp_CloseHandlers(void);

// Send request message to RTApp (and receve response)
extern bool SendRTApp_SendMessageToRTCore(
    const unsigned char* txMessage, long txMessageSize);
extern bool SendRTApp_SendMessageToRTCoreAndReadMessage(
    const unsigned char* txMessage, long txMessageSize,
    unsigned char* rxMessage, long rxMessageSize);

#endif  // _TELEMETRYITEMS_H_
