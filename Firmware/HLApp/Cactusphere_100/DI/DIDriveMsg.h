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

#ifndef _DI_DRIVER_MSG_H_
#define _DI_DRIVER_MSG_H_

#ifndef _STDINT_H
#include <stdint.h>
#endif

// request code
enum {
    DI_SET_CONFIG_AND_START = 1,  // setting pulse parameters and start pulse counter
    DI_PULSE_COUNT_RESET = 2,  // pulse count reset
    DI_READ_PULSE_COUNT = 3,  // read pulse count
    DI_READ_DUTY_SUM_TIME = 4, // resd pulse on time
    DI_READ_PULSE_LEVEL		= 5,  // read input levels
    DI_READ_PIN_LEVEL = 6,      // read pin level
    DI_READ_VERSION = 255,      // read the RTApp version
};

//
// message structure
//
// header
typedef struct DI_DriverMsgHdr {
    uint32_t	requestCode;
    uint32_t	messageLen;
} DI_DriverMsgHdr;

// body
// setting config
typedef struct DI_MsgSetConfig {
    uint32_t	pinId;
    uint32_t minPulseWidth;
    uint32_t maxPulseCount;
    bool isPulseHigh;
    // sizeof(DI_MsgSetConfig) == messageLen
}DI_MsgSetConfig;

// pulse reset
typedef struct DI_MsgResetPulseCount {
    uint32_t	pinId;
    uint32_t initVal;
// sizeof(DI_MsgResetPulseCount) == messageLen
}DI_MsgResetPulseCount;

// pinID
typedef struct DI_MsgPinId {
    uint32_t	pinId;
    // sizeof(DI_MsgPinId) == messageLen
}DI_MsgPinId;

// message
typedef struct DI_DriverMsg {
    DI_DriverMsgHdr	header;
    union {
        DI_MsgSetConfig    setConfig;
        DI_MsgResetPulseCount       resetPulseCount;
        DI_MsgPinId pinId;
    } body;
} DI_DriverMsg;

// return message
typedef struct DI_ReturnMsg {
    uint32_t	returnCode;
    uint32_t	messageLen;
    union {
        bool		levels[4];
        char        version[256];
    } message;
}DI_ReturnMsg;

#endif  // _DI_DRIVER_MSG_H_
