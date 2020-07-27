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

#ifndef _UART_DRIVER_MSG_H_
#define _UART_DRIVER_MSG_H_

#ifndef _STDINT_H
#include <stdint.h>
#endif

// constants
#define MAX_UART_WRITE_LEN	256

// request code
enum {
    UART_REQ_WRITE_AND_READ = 1,  // send request and receive response aganist opposing device
    UART_REQ_SET_PARAMS     = 2,  // setting UART parameters
    UART_REQ_VERSION        = 255,// RTApp Version
};

//
// message structure
//
// header
typedef struct UART_DriverMsgHdr {
    uint32_t	requestCode;
    uint32_t	messageLen;
} UART_DriverMsgHdr;

// body
    // UART_REQ_WRITE_AND_READ
typedef struct UART_MsgWriteAndRead {
    uint16_t	writeLen;
    uint16_t	readLen;
    uint32_t	writeData[1];  // writeLen
//
// (sizeof(writeLen) + sizeof(readLen) + writeLen) == messageLen
// writeLen must (<= MAX_UART_WRITE_LEN)
//
} UART_MsgWriteAndRead;
    // UART_REQ_SET_PARAMS
typedef struct UART_MsgSetParams {
    uint32_t	baudRate;
//
// sizeof(UART_MsgSetParams) == messageLen
//
} UART_MsgSetParams;

// union of messages
typedef struct UART_DriverMsg {
    UART_DriverMsgHdr	header;
    union {
        UART_MsgWriteAndRead    writeAndReadReq;
        UART_MsgSetParams       setParams;
    } body;
} UART_DriverMsg;

// response message
typedef struct UART_ReturnMsg {
    uint32_t    returnCode;
    uint32_t    messageLen;
    union {
        char    version[256];
    } message;
} UART_ReturnMsg;

// macro for UART_REQ_WRITE_AND_READ
#define UART_MsgWriteAndRead_WriteDataPtr(msgBody) \
    (unsigned char*)&(msgBody->writeData)

#endif  // _UART_DRIVER_MSG_H_
