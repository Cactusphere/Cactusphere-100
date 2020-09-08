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

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "ModbusDevRTU.h"
#include "UartDriveMsg.h"
#include "SendRTApp.h"
#include "vector.h"

#define MODBUS_RTU_HEADER_LENGTH 1
#define MODBUS_RTU_CHECKSUM_LENGTH 2

#define MIN_REQ_LENGTH 12
#define MAX_MESSAGE_LENGTH 256

#define MODBUS_RTU_PRESET_REQ_LENGTH 6

// function
#define FC_READ_HOLDING_REGISTER 0x03
#define FC_READ_INPUT_REGISTERS 0x04
#define FC_WRITE_SINGLE_REGISTER 0x06

// ModbusCtx structure
typedef struct ModbusCtx {
    int devId;
    int baud;
    int header_length;
    int checksum_length;
}ModbusCtx;

static uint16_t 
ModbusRTU_CalcCRC(uint8_t* req, int req_length) {
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < req_length; i++) {
        crc = (uint16_t)(crc ^ req[i]);
        for (int j = 0; j < 8; j++) {
            if ((crc & 1) == 1) {
                crc = crc >> 1;
                crc ^= 0xA001;
            }
            else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

static int 
ModbusRTU_AddCRCRequestMsg(uint8_t* req, int req_length) {
    uint16_t crc = ModbusRTU_CalcCRC(req, req_length);

    req[req_length++] = (uint8_t)crc;
    req[req_length++] = (uint8_t)(crc >> 8);

    return req_length;
}

static int
ModbusRTU_CreateRequestMsg(ModbusCtx* me, int function, int addr, int length, uint8_t *req) {
    req[0] = (uint8_t)me->devId;
    req[1] = (uint8_t)function;
    req[2] = (uint8_t)(addr >> 8);
    req[3] = (uint8_t)(addr & 0x00ff);
    req[4] = (uint8_t)(length >> 8);
    req[5] = (uint8_t)(length & 0x00ff);

    return ModbusRTU_AddCRCRequestMsg(req, MODBUS_RTU_PRESET_REQ_LENGTH);
}

static int 
ModbusRTU_CheckResponseMsg(ModbusCtx* me, uint8_t* req, uint8_t* rsp){
    int rc = 0;
    const int offset = me->header_length;
    const int function = rsp[offset];
    int req_calc_length = 0;
    int rsp_calc_length = 0;

    if (req[0] != rsp[0]) {
        return -1;
    }

    if (function >= 0x80) {
        return -1;
    }

    switch (function) {
    case FC_READ_HOLDING_REGISTER:
    case FC_READ_INPUT_REGISTERS:
        req_calc_length = (req[offset + 3] << 8) + req[offset + 4];
        rsp_calc_length = (rsp[offset + 1] / 2);
        break;
    default:
        req_calc_length = rsp_calc_length = 1;
        break;
    }

    if (req_calc_length == rsp_calc_length) {
        rc = rsp_calc_length;
    }

    return rc;
}

static bool
ModbusDevRTU_ReadRegister(ModbusCtx* me, int regAddr, int function, unsigned short* dst) {
    int rc;
    int req_length;
    uint8_t req[MIN_REQ_LENGTH];
    uint8_t rsp[MAX_MESSAGE_LENGTH];
    unsigned char sendMessage[MAX_MESSAGE_LENGTH];
    UART_DriverMsg* msg = (UART_DriverMsg*)sendMessage;
    int length = 1;

    req_length = ModbusRTU_CreateRequestMsg(me, function, regAddr, length, req);

    msg->header.requestCode = UART_REQ_WRITE_AND_READ;

    memcpy(msg->body.writeAndReadReq.writeData, req, (size_t)req_length);
    msg->body.writeAndReadReq.writeLen = (uint16_t)req_length;
    msg->body.writeAndReadReq.readLen = 7;

    msg->header.messageLen = sizeof(msg->body.writeAndReadReq.writeLen)
        + sizeof(msg->body.writeAndReadReq.readLen)
        + msg->body.writeAndReadReq.writeLen;

    rc = SendRTApp_SendMessageToRTCoreAndReadMessage((const unsigned char*)msg, 
        (long)(sizeof(msg->header) + msg->header.messageLen),
        rsp, 
        msg->body.writeAndReadReq.readLen);

    if (rc > 0) {
        int offset;
        int i;

        rc = ModbusRTU_CheckResponseMsg(me, req, rsp);
        if (rc == -1)
            return false;

        offset = me->header_length;

        for (i = 0; i < rc; i++) {
            dst[i] = (unsigned short)((rsp[offset + 2 + (i << 1)] << 8) |
                rsp[offset + 3 + (i << 1)]);
        }
    }

    return rc;
}

// Initialization and cleanup
ModbusCtx* 
ModbusDevRTU_Initialize(int devId, int baud) {
    ModbusCtx* newObj;

    newObj = (ModbusCtx*)malloc(sizeof(ModbusCtx));

    newObj->devId = devId;
    newObj->baud = baud;

    newObj->header_length = MODBUS_RTU_HEADER_LENGTH;
    newObj->checksum_length = MODBUS_RTU_CHECKSUM_LENGTH;

    return newObj;
}

void
ModbusDevRTU_Destroy(ModbusCtx* me) {
    free(me);
}

// Connect
bool 
ModbusDevRTU_Connect(ModbusCtx* me) {
    unsigned char sendMessage[256];
    unsigned char readMessage;
    UART_DriverMsg* msg = (UART_DriverMsg*)sendMessage;
    int msgSize;

    msg->header.requestCode = UART_REQ_SET_PARAMS;
    msg->header.messageLen = sizeof(UART_MsgSetParams);
    msg->body.setParams.baudRate = (uint32_t)me->baud;
    msgSize = (int)(sizeof(msg->header) + msg->header.messageLen);

    SendRTApp_SendMessageToRTCoreAndReadMessage((const unsigned char*)msg, (long)msgSize, &readMessage, sizeof(readMessage));
    if (readMessage != 1) {
        return false;
    }
    return true;
}

// Read single register
bool 
ModbusDevRTU_ReadSingleRegister(ModbusCtx* me, int regAddr, unsigned short* dst) {
    return ModbusDevRTU_ReadRegister(me, regAddr, FC_READ_HOLDING_REGISTER, dst);
}

bool
ModbusDevRTU_ReadSingleInputRegister(ModbusCtx* me, int regAddr, unsigned short* dst) {
    return ModbusDevRTU_ReadRegister(me, regAddr, FC_READ_INPUT_REGISTERS, dst);
}

// Write single register
bool
ModbusDevRTU_WriteSingleRegister(ModbusCtx* me, int regAddr, unsigned short value) {
    int function = FC_WRITE_SINGLE_REGISTER;
    int rc;
    int req_length;
    uint8_t req[MIN_REQ_LENGTH];
    uint8_t rsp[MAX_MESSAGE_LENGTH];
    unsigned char sendMessage[MAX_MESSAGE_LENGTH];
    UART_DriverMsg* msg = (UART_DriverMsg*)sendMessage;

    req_length = ModbusRTU_CreateRequestMsg(me, function, regAddr, (int)value, req);

    msg->header.requestCode = UART_REQ_WRITE_AND_READ;

    memcpy(msg->body.writeAndReadReq.writeData, req, (size_t)req_length);
    msg->body.writeAndReadReq.writeLen = (uint16_t)req_length;
    msg->body.writeAndReadReq.readLen = 8;
    msg->header.messageLen = sizeof(msg->body.writeAndReadReq.writeLen)
        + sizeof(msg->body.writeAndReadReq.readLen)
        + msg->body.writeAndReadReq.writeLen;

    rc = SendRTApp_SendMessageToRTCoreAndReadMessage((const unsigned char*)msg, 
        (long)(sizeof(msg->header) + msg->header.messageLen),
        rsp, 
        msg->body.writeAndReadReq.readLen);

    if (rc > 0) {
        rc = ModbusRTU_CheckResponseMsg(me, req, rsp);
        if (rc == -1)
            return false;
    }
    return rc;
}
